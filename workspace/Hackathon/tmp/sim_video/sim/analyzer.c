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


int cur_stat_id = 0;
int prev_stat_id = 1;
pid_stat_t pid_stat_tab[2][NUM_PIDS];

int time_id = 0;
int prog_tp_cnt[MAX_PROGS_PER_TS][MAX_TIME_ID];

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

// Prototype
void sim_demux_ip_pkt(uintptr_t ip_pkt_addr, int num_bytes, 
                      hash_table_t *ses_hash_tab, video_ip_stat_t *my_stat);

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


void analyzer_process_ip_pkt (uintptr_t ip_pkt, int num_bytes)
{
    // Dummy demux
    sim_demux_ip_pkt(ip_pkt, num_bytes, video_ctx->ses_hash_tab[0], &ip_stat);
    ip_buf_rd = ring_mod32(ip_buf_rd + 1, NUM_IP_BUFS);

    // Input processing
    in_session_t* ses = get_in_session(0);
    if (!session_is_used(ses->state))  return;
    ses_hash_item_t* hash_item = &video_ctx->ses_hash_item[0];
    if (hash_item->flow_rd != hash_item->flow_wr) {
        process_input_session(ses, &hash_item->flow_rd, hash_item->flow_wr);
    }

    // TODO: input monitoring
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

        // Analyzer: trigger packet processing directly here!
        analyzer_process_ip_pkt((uintptr_t)(buf + ip_buf_wr * IP_BUF_SZ), rc);
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


void sim_demux_ip_pkt (uintptr_t ip_pkt_addr, int num_bytes,
                       hash_table_t *ses_hash_tab, video_ip_stat_t *my_stat)
{
    // Analyzer: Hard code to the first session
    ses_hash_item_t* ses_item = &video_ctx->ses_hash_item[0];

    ses_item->info->addr = ip_pkt_addr;
    ses_item->info->num_bytes = num_bytes;
    ses_item->info++;

    if (++ses_item->flow_wr == IP_FLOW_BUF_SIZE) {
        ses_item->flow_wr = 0;
        ses_item->info -= IP_FLOW_BUF_SIZE;
    }

    // Check for IP flow buffer overrun
    if (ses_item->flow_wr == ses_item->flow_rd) {
        my_stat->overrun_cnt++;
    }

    //check_capture(hdr, CLI_VIDEO_CAPTURE_ANY);
}


static
void pid_stat_collect (pid_stat_t* pid_stat_tab, tp_header_t* tp_hdr)
{
    af_header_t* af_hdr = (af_header_t*)(tp_hdr + 1);
    int pid = tp_get_pid(tp_hdr);
    pid_stat_t* pid_stat = &pid_stat_tab[pid];

    pid_stat->tp_cnt++;
    if (pid == NULL_PID)  return;

    // Check CC error
    if (!(tp_hdr->af_ctrl & 2) || (af_hdr->len == 0)
            || !af_hdr->discontinuity) {
        uint8 exp_cc = (pid_stat->prev_cc + (tp_hdr->af_ctrl & 1)) & 0xf;
        if (tp_hdr->cc != exp_cc && pid_stat->prev_cc != CC_DISC) {
            pid_stat->cc_err_cnt++;
        }
    }
    pid_stat->prev_cc = tp_hdr->cc;
}


void process_input_ip_pkt (ip_pkt_info_t *ip_pkt_info, in_session_t *ses)
{
    uintptr_t tp_addr = (uintptr_t)ip_pkt_info->addr;
    assert(tp_addr != 0);

    ses->perf.now = 0;		// TODO: use system time instead

    int num_tp = ip_pkt_info->num_bytes / TP_SIZE;
    if (num_tp > MAX_TP_PER_IP) {
        // Drop jumble packet
        ses->stat.drop_ip_cnt++;
        return;
    }

    ses->arvl_time = 0;		// TODO: use sys time instead

    // Process all TPs in IP
    int i;
    for (i=0; i<num_tp; i++) {
        tp_header_t* tp_hdr = (tp_header_t*)tp_addr;
        pid_stat_collect(pid_stat_tab[cur_stat_id], tp_hdr);
        process_input_tp(*tp_hdr, tp_addr, ses);
        tp_addr += TP_SIZE;
    }

    ses->stat.tp_cnt += num_tp;
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


// In Analyzer, this thread is used to update the stat index
// TODO: need to protect pid_stat_tab with mutex!
//
static void* dummy_server_main (void* arg)
{
    void analyzer_stat_update(ipc_message *ipc_msg, ipc_message *rsp_msg);

    while (1) {
        sleep(1);
        analyzer_stat_update(NULL, NULL);
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

printf("connection recevied: port %d\n", cfg->port);

#if 1
        pthread_t tid;
        if (pthread_create(&tid, NULL, cfg->server_loop, arg) != 0) {
            perror("pthread_create");
            return (void*)NULL;
        }
#else
        msg_handler_arg_t hdl_arg = {NULL, (ipc_message*)video_rsp_buf};
        int n = read(cfg->sock_fd, video_msg_buf, sizeof(video_msg_buf));
        if (n > 0) {
            hdl_arg.ipc_msg = (ipc_message*)video_msg_buf;
            uint32 msg_size = sizeof(ipc_message_header)
                                      + IPC_HEADER(ipc_msg)->size;
            cfg->handler(&hdl_arg);
            if (hdl_arg.rsp_flag) {
                sim_send_video_response(cfg->sock_fd, video_rsp_buf);
            }
        }
#endif
    }
}


void sim_video_config (void)
{
    // Analyzer: hard code session 0 for the input

    uint8* msg_buf = malloc(1024);
    memset(msg_buf, 0, sizeof(msg_buf));
    ipc_message_header* ipc_hdr = IPC_HEADER(msg_buf);
    video_add_session_msg_t* cmd = IPC_DATA(msg_buf);

    ipc_hdr->type = MSG_TYPE_VIDEO_ADD_SESSION;
    ipc_hdr->size = sizeof(*cmd);

    cmd->primary_id = MY_SLOT_ID;
    cmd->owner_id = 0;
    cmd->transaction_id = 0;
    cmd->resource_id = 0;
    cmd->ip_addr_len = IPV4_ADDR_LEN;
    cmd->input_type = INPUT_TYPE_UNICAST;
    cmd->dst_ip_addr[0] = 0x01010101;
    cmd->dst_udp_port = 50000;

    cmd->cbr = VIDEO_CR_MODE_UNIFIED_VBR;
    cmd->jitter_size = 100;
    cmd->delay_target = 70;
    cmd->idle_threshold = 500;
    cmd->init_threshold = 2000;
    cmd->off_timer = 30;
    cmd->encrypt_flag = 0;

    cmd->bitrate_alloc = 38800000;
    cmd->qam_id = 0;

    cmd->stream_type = STREAM_TYPE_MPTS;
    cmd->pid_remap = 0;
    cmd->parse_psi = 1;
    cmd->dejitter = 1;
    cmd->prog_num = 0;

    cmd->enable = 1;

    sim_process_video_add_session((ipc_message*)msg_buf);

    free(msg_buf);
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

    // Analyzer: set up default passthru session
    sim_video_config();
    memset(pid_stat_tab, 0, sizeof(pid_stat_tab));
    memset(prog_tp_cnt, 0, sizeof(prog_tp_cnt));

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
