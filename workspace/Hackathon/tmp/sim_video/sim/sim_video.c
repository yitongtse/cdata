///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
//#include "bench_posix.h"
#include "video.h"
#include "video_util.h"
#include "video_cli.h"
#include "sim_video.h"


#define TIME_WIN_MS             20

#define MY_SLOT_ID              0
#define IP_BUF_SZ               1536

#define LISTENQ                 1024


uint32 slot_id;
uint32 wake_cnt = 0;
int srv_cnt = 0;
int pending_srv_cnt = 0;

int ctrl_sock_fd = -1;

pthread_mutex_t timer_mutex;
pthread_cond_t tx_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t rx_cond = PTHREAD_COND_INITIALIZER;

thread_stat_t rx_thread_stat;
thread_stat_t demux_thread_stat;
thread_stat_t input_thread_stat;
thread_stat_t output_thread_stat;

logger_t *video_log = NULL;

int rx_ip_cnt = 0;
uint8 video_ip_buf[IP_BUF_SZ * NUM_IP_BUFS];

int ip_buf_rd = 0;             // IP buffer read index
int ip_buf_wr = 0;             // IP buffer write index

video_context_t *video_context[NUM_SLOTS];
video_ip_stat_t ip_stat;

out_command_t* out_cmd_buf[NUM_QAMS][FLOWS_PER_QAM];
ring_buf_t out_cmd_ring[NUM_QAMS][FLOWS_PER_QAM];

uint64 mux_tp_cnt = 0;
out_command_t* mux_cmd_buf[NUM_QAMS];
ring_buf_t mux_cmd_ring[NUM_QAMS];

// Server configs
server_config_t sim_cli_cfg = {VIDEO_CLI_PORT, 0, cli_server_loop,
                               cli_handler};
server_config_t sim_msg_cfg = {VIDEO_MSG_PORT, 0, msg_server_loop,
                               sim_video_msg_handler};


void signal_handler (int signum)
{
    if (ctrl_sock_fd != -1) {
        close(ctrl_sock_fd); 
    }
    exit(-1);
}


void advance_mpeg_time (void)
{
    static mpeg_time_t PCR_RANGE
            = ((uint64)SCALE_BASE_EXT) << (FRAC_BITS_TIME + 32);
    static int64 clk_inc = (((int64)EXT_FREQ) << FRAC_BITS_TIME) * TIME_WIN_MS
                               / 1000;

    sys_time += clk_inc;
    if (sys_time > PCR_RANGE) {
        sys_time -= PCR_RANGE;
        sys_time_hi++;
    }
}


static void* data_packet_handler (void* arg)
{
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        perror("socket");
        exit(-1);
    }

    int so_reuseaddr = TRUE;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
               sizeof(so_reuseaddr));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(VIDEO_DATA_PORT);

    int rc = bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc == -1) {
        perror("bind");
        exit(-1);
    }

    uint8* buf = video_ip_buf;

    // Wait for video packets
    while (1) {
        rc = recvfrom(sock_fd, buf + ip_buf_wr * IP_BUF_SZ, IP_BUF_SZ, 0,
                      NULL, NULL);
        if (rc == -1) {
            perror("recvfrom");
            exit(-1);
        }

        // Update IP buffer write pointer
        ip_buf_wr = ring_mod32(ip_buf_wr + 1, NUM_IP_BUFS);

        rx_ip_cnt++;
    }
}


static int video_init (void)
{
    memset(video_context, 0, sizeof(video_context));
    memset(&ip_stat, 0, sizeof(ip_stat));

    video_state_init();

    // Set up output command buffer
    out_command_t* buf
       = (out_command_t*)calloc(OUTCMDS_PER_FLOW * FLOWS_PER_QAM * NUM_QAMS,
                                sizeof(out_command_t));
    assert(buf != NULL);

    int i, j;
    for (i=0; i<NUM_QAMS; i++) {
        for (j=0; j<FLOWS_PER_QAM; j++, buf+=OUTCMDS_PER_FLOW) {
            out_cmd_buf[i][j] = buf;
        }
    }

    // Set up output command ring
    memset(&out_cmd_ring, 0, sizeof(out_cmd_ring));

    // Set up mux command buffer
    buf = (out_command_t*)calloc(OUTCMDS_PER_FLOW * NUM_QAMS,
                                 sizeof(out_command_t));
    assert(buf != NULL);
    for (i=0; i<NUM_QAMS; i++, buf+=OUTCMDS_PER_FLOW) {
        mux_cmd_buf[i] = buf;
    }

    // Set up mux command ring
    memset(&mux_cmd_ring, 0, sizeof(mux_cmd_ring));
    memset(&rx_thread_stat, 0, sizeof(thread_stat_t));
    memset(&demux_thread_stat, 0, sizeof(thread_stat_t));
    memset(&input_thread_stat, 0, sizeof(thread_stat_t));
    memset(&output_thread_stat, 0, sizeof(thread_stat_t));

    return EOK;
}


static void sim_demux_ip_pkt (uintptr_t ip_pkt_addr, hash_table_t *ses_hash_tab,
                              video_ip_stat_t *my_stat)
{
    sim_video_header_t* sim_hdr = (sim_video_header_t*)ip_pkt_addr;
    ip_header_t* hdr = &sim_hdr->ip_hdr;

    boolean mcast_flag;
    if (hdr->ver == IPV4_VERSION) {
        my_stat->ipv4_cnt++;
        mcast_flag = is_multicast_v4(hdr->dst_ip);
    } else {
        my_stat->ipv6_cnt++;
        mcast_flag = is_multicast_v6(((ipv6_header_t*)hdr)->dst_ip);
    }

    if (mcast_flag) {
        my_stat->mcast_cnt++;
    } else {
        my_stat->ucast_cnt++;
    }

    // Look up the session
    ses_hash_key_t ses_key;
    get_in_session_key(hdr, mcast_flag, &ses_key);
    ses_hash_item_t* ses_item
            = (ses_hash_item_t*)hash_table_lookup(ses_hash_tab, &ses_key);

    if (!ses_item) {
        // Not such session
        my_stat->unref_cnt++;
        check_capture(hdr, CLI_VIDEO_CAPTURE_DROP);
        return;
    }

    (ses_item->info++)->addr = ip_pkt_addr;
    if (++ses_item->flow_wr == IP_FLOW_BUF_SIZE) {
        ses_item->flow_wr = 0;
        ses_item->info -= IP_FLOW_BUF_SIZE;
    }

    // Check for IP flow buffer overrun
    if (ses_item->flow_wr == ses_item->flow_rd) {
        my_stat->overrun_cnt++;
    }

    check_capture(hdr, CLI_VIDEO_CAPTURE_ANY);
}


static void sim_video_demux (void)
{
    thread_stat_start(&demux_thread_stat, get_time());

    while (ip_buf_rd != ip_buf_wr) {
        sim_demux_ip_pkt(((uintptr_t)video_ip_buf) + ip_buf_rd * IP_BUF_SZ,
                         video_ctx->ses_hash_tab[0], &ip_stat);

        // Update IP buffer read pointer
        ip_buf_rd = ring_mod32(ip_buf_rd + 1, NUM_IP_BUFS);
    }

    thread_stat_stop(&demux_thread_stat, get_time());
}


void process_input_ip_pkt (ip_pkt_info_t *ip_pkt_info, in_session_t *ses)
{
    sim_video_header_t* sim_hdr = (sim_video_header_t*)ip_pkt_info->addr;
    ses->perf.now = sim_hdr->arvl_time;

    ip_header_t* ip_hdr = &sim_hdr->ip_hdr;
    ipv6_header_t* ipv6_hdr = (ipv6_header_t*)ip_hdr;
    assert(ip_hdr != 0);

    // perfrom ip version check to set up
    // address pointer for udp header
    udp_header_t* udp_hdr;
    if (ip_hdr->ver == IPV4_VERSION) {
        udp_hdr = (udp_header_t*)(ip_hdr + 1);
    } else {
       udp_hdr = (udp_header_t*)(ipv6_hdr + 1);
    }

    uintptr_t addr = (uintptr_t)(udp_hdr + 1);

    // RTP-tolerant support
    int rtp_hdr_size = get_rtp_header_size(addr);
    if (rtp_hdr_size) {
        // RTP header present
        ses->stat.rtp_cnt++;

        if (rtp_hdr_size != RTP_HDR_LEN) {
            // Drop RTP with extended header
            ses->stat.drop_ip_cnt++;
            return;
        }
        addr += RTP_HDR_LEN;
    }

    int num_tp = (udp_hdr->length - rtp_hdr_size) / TP_SIZE;
    if (num_tp > MAX_TP_PER_IP) {
        // Drop jumble packet
        ses->stat.drop_ip_cnt++;
        return;
    }

    // Check for source IP change
    if (ip_hdr->ver == IPV4_VERSION) {
        if (ip_hdr->src_ip != ses->src_ip[0]) {
            ses->src_ip_change_cnt++;
            ses->src_ip[0] = ip_hdr->src_ip;
            VIDEO_IN_DEBUG("\nIn %d: new src IP %08x", ses->id, ses->src_ip[0]);
        }
    } else {
        if (!memcmp(ipv6_hdr->src_ip, ses->src_ip, sizeof(ipv6_hdr->src_ip))) {
            ses->src_ip_change_cnt++;
            memcpy(ses->src_ip,ipv6_hdr->src_ip, sizeof(ipv6_hdr->src_ip));
            VIDEO_IN_DEBUG("\nIn %d: new src IP %08x", ses->id, ses->src_ip[3]);
        }
    }

    ses->ip_hdr = ip_hdr;
    ses->arvl_time = sim_hdr->arvl_time;

    // Process all TPs in IP
    int i;
    for (i=0; i<num_tp; i++) {
        process_input_tp(*(tp_header_t*)addr, addr, ses);
        addr += TP_SIZE;
    }

    ses->stat.tp_cnt += num_tp;
}


static void sim_video_input (void)
{
    thread_stat_start(&input_thread_stat, get_time());

    static int mon_cntdwn = 100 / TIME_WIN_MS;
    boolean monitor_flag = FALSE;
    if (--mon_cntdwn <= 0) {
        mon_cntdwn = 100 / TIME_WIN_MS;
        monitor_flag = TRUE;
    }

    int i;
    for (i=0; i<MAX_SESSIONS; i++) {
        in_session_t* ses = get_in_session(i);
        if (!session_is_used(ses->state))  continue;
        ses_hash_item_t* hash_item = &video_ctx->ses_hash_item[i];
        if (hash_item->flow_rd != hash_item->flow_wr) {
            process_input_session(ses, &hash_item->flow_rd, hash_item->flow_wr);
        }

        // Input monitoring
        if (monitor_flag) {
            video_monitor_in_session(ses);
        }
    }

    thread_stat_stop(&input_thread_stat, get_time());
}


static void sim_video_output (void)
{
    thread_stat_start(&output_thread_stat, get_time());

    static int mon_cntdwn = 100 / TIME_WIN_MS;
    boolean monitor_flag = FALSE;
    if (--mon_cntdwn <= 0) {
        mon_cntdwn = 100 / TIME_WIN_MS;
        monitor_flag = TRUE;
    }

    int i;
    for (i=0; i<NUM_QAMS; i++) {
        qam_info_t* qam = get_qam_info(i);
        process_qam_channel(qam);
        qam_mux(i, sys_clk.base + SCH_WIN);

        // Assume all muxed output packets can be sent, no backpressure!
        ring_buf_t* ring = &mux_cmd_ring[i];
        ring->rd = ring->wr;

        // QAM monitoring
        if (monitor_flag) {
            video_monitor_qam(qam);
        }
    }

    thread_stat_stop(&output_thread_stat, get_time());
}


static void* ctrl_handler (void* arg)
{
    signal(SIGPIPE, SIG_IGN);           // ignore SIGPIPE signal

    sim_ctrl_msg_t ctrl_msg;
    ctrl_msg.time_win_size = TIME_WIN_MS * SCALE_MS_BASE;

    int conn_fd = *((int*)arg);         // socket file descriptor
    uint32 my_wake_cnt;

    pthread_mutex_lock(&timer_mutex);
    srv_cnt++;
    my_wake_cnt = wake_cnt;
    pthread_mutex_unlock(&timer_mutex);

    boolean pending_change = FALSE;
    int rc;

    while (1) {
        pthread_mutex_lock(&timer_mutex);
        if (pending_change) {
            if (pending_srv_cnt > 0) {
                pending_srv_cnt--;
                pthread_cond_signal(&rx_cond);
            }
            pending_change = FALSE;
        }

        do {
            pthread_cond_wait(&tx_cond, &timer_mutex);
        } while (my_wake_cnt == wake_cnt);

        // Monitor potential race condition
        if (wake_cnt - my_wake_cnt > 1) {
            printf("Wake %d -> %d\n", my_wake_cnt, wake_cnt);
        }

        my_wake_cnt = wake_cnt;
        ctrl_msg.sys_time = mpeg_time_to_base(sys_time);
        pthread_mutex_unlock(&timer_mutex);

        int n = write(conn_fd, &ctrl_msg, sizeof(sim_ctrl_msg_t));
        if (n == 0 || n == -1)  break;

        n = read(conn_fd, &ctrl_msg, sizeof(sim_ctrl_msg_t));
        if (n == 0 || n == -1)  break;

        pending_change = TRUE;
    }

disconnect:
    printf("Client disconnected.\n");
    pthread_mutex_lock(&timer_mutex);
    srv_cnt--;
    if (pending_srv_cnt > 0) {
        pending_srv_cnt--;
        pthread_cond_signal(&rx_cond);
    }
    pthread_mutex_unlock(&timer_mutex);


    return NULL;
}


static void* video_process_main (void* arg)
{
    sleep(1);

    while (1) {
        pthread_mutex_lock(&timer_mutex);
        pending_srv_cnt = srv_cnt;
        wake_cnt++;
        pthread_cond_broadcast(&tx_cond);

        while (1) {
            pthread_cond_wait(&rx_cond, &timer_mutex);
            if (pending_srv_cnt == 0)  break;
        }
        pthread_mutex_unlock(&timer_mutex);

        // Video processing
        advance_mpeg_time();
        get_mpeg_time(&sys_clk);

        if (video_ctx) {
// TODO: should also process traffic even when LC is not hot!
            sim_video_demux();
            sim_video_input();
            sim_video_output();
        }
    }
}


static void* dummy_server_main (void* arg)
{
    // Setup socket for flow control
    ctrl_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctrl_sock_fd < 0) {
        perror("socket2");
        exit(-1);
    }

    int so_reuseaddr = TRUE;
    setsockopt(ctrl_sock_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
               sizeof(so_reuseaddr));

    struct sockaddr_in ctrl_serv_addr;
    memset(&ctrl_serv_addr, 0, sizeof(ctrl_serv_addr));
    ctrl_serv_addr.sin_family = AF_INET;
    ctrl_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ctrl_serv_addr.sin_port = htons(VIDEO_FLOWCTRL_PORT);
    int rc = connect(ctrl_sock_fd, (struct sockaddr *)&ctrl_serv_addr,
                     sizeof(ctrl_serv_addr));
    if (rc < 0) {
        perror("connect");
        exit(-1);
    }

    while (1) {
        // Wait for flow control
        sim_ctrl_msg_t ctrl_msg;
        int n = read(ctrl_sock_fd, &ctrl_msg, sizeof(sim_ctrl_msg_t));
        assert(n != -1);

        write(ctrl_sock_fd, &ctrl_msg, sizeof(sim_ctrl_msg_t));
    }
}


static void* server_setup (void* arg)
{
    server_config_t* cfg = (server_config_t*)arg;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(-1);
    }

    int so_reuseaddr = TRUE;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
               sizeof(so_reuseaddr));

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(cfg->port);

    int rc = bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc < 0) {
        perror("connect");
        exit(-1);
    }

    if (listen(listen_fd, LISTENQ) == -1) {
        perror("listen");
        exit(-1);
    }

    while (1) {
        cfg->sock_fd = accept(listen_fd, (struct sockaddr *)NULL, NULL);
        if (cfg->sock_fd < 0) {
            perror("accept");
            return (void*)NULL;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, cfg->server_loop, arg) != 0) {
            perror("pthread_create");
            return (void*)NULL;
        }
    }
}


int main (int argc, char **argv)
{
    signal(SIGINT, signal_handler);           // ctrl-C

    video_init();

    // Configure for non redundant mode
    // Default LCRED setting
    slot_id = MY_SLOT_ID;
    lcred_mode_t mode = LCRED_NON_REDUNDANT_MODE;
    lcred_role_t role = LCRED_ACTIVE_ROLE;

    int rc = video_context_init(slot_id);
    if (rc != EOK) {
        printf("Failed to initialize video context\n");
        return rc;
    }
    sim_video_lcred_change_mode_role(&mode, &role);

    // Set up recursive mutex
    pthread_mutexattr_t mta;
    pthread_mutexattr_init(&mta);
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&timer_mutex, &mta);

    // Start video processing thread
    pthread_t proc_tid;
    pthread_create(&proc_tid, NULL, video_process_main, NULL);

    // Start the input packet handler thread (for data traffic)
    pthread_t in_tid;
    pthread_create(&in_tid, NULL, data_packet_handler, NULL);

    // TCP socket for flow control
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(-1);
    }

    int so_reuseaddr = TRUE;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
               sizeof(so_reuseaddr));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(VIDEO_FLOWCTRL_PORT);

    rc = bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc == -1) {
        perror("bind2");
        exit(-1);
    }

    if (listen(listen_fd, LISTENQ) == -1) {
        perror("listen");
        exit(-1);
    }

    // Setup message server
    pthread_t msg_tid;
    if (pthread_create(&msg_tid, NULL, server_setup, &sim_msg_cfg) != 0) {
        perror("pthread_create MSG server");
        return -1;
    }

    // Setup CLI server
    sim_video_cli_init();
    pthread_t cli_tid;
    if (pthread_create(&cli_tid, NULL, server_setup, &sim_cli_cfg) != 0) {
        perror("pthread_create CLI server");
        return -1;
    }

    // Start dummy video server
    pthread_t srv_tid;
    if (pthread_create(&srv_tid, NULL, dummy_server_main, NULL) != 0) {
        perror("pthread_create dummy video server");
        return -1;
    }

    // Wait for new server connection
    while (1) {
        int conn_fd;
        if ((conn_fd = accept(listen_fd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept");
            exit(-1);
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, ctrl_handler, &conn_fd) != 0) {
            perror("pthread_create");
            exit(-1);
        }
    }

    close(listen_fd);
    exit(0);
}
