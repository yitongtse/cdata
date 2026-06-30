///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "cli_common.h"
#include "generic_cli.h"
#include "video_cli.h"
#include "sim_video.h"
#include "sim_cli_video.h"
#include "sim_cli_video_cmd.h"
#include "video_messages.h"


int video_msg_fd;         // socket to video message server

extern int neg_flag;

int cli_exit(cli_control_block *ccb);
int cli_echo(cli_control_block *ccb);

////////////////////////////////////////////////////////////////
CLI_ARR_START(cli_elem)
CLI_ARR_CMD (0, exit,
             CLI_FUN(cli_exit, 0),
             0, "Exit")
CLI_ARR_END
////////////////////////////////////////////////////////////////


int cli_exit (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cli_main_loop_end();
    return 0;
}


static
void* cli_loop (void *arg)
{
    cli_init();
    cli_set_prompt("sim_cli> ");
    cli_add_elems(cli_elem);
    cli_add_elems(cli_video_elem);
    cli_main_loop();

    return (void*)NULL;
}


static
int connect_socket (char *addr, int port)
{
    // Create socket to the VDM CLI port
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        printf("socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock_fd);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}


char default_server[] = "127.0.0.1";


int main (int argc, char **argv)
{
    if (argc > 3) {
        printf("Usage: %s [-n] [ip_addr]\n", argv[0]);
        exit(-1);
    }

    char* server = NULL;

    /* option processing */
    int opt_ret = getopt(argc, argv, "n::");
    if (opt_ret == 'n') {
        printf("No Range checking for numbers\n");
        neg_flag = 1;
        server = (argc == 2) ?default_server : argv[2];
    } else {
        server = (argc == 1)? default_server : argv[1];
    }

    cli_peer_init();

    // store the server ip address for lazy opening use
    strncpy(cli_server, server, INET6_ADDRSTRLEN);

    // set up sock port for cli
    cli_sock_port[0] = VIDEO_CLI_PORT;

    // Set up socket to sim_video
    cli_sock_fd[0] = connect_socket(server, VIDEO_CLI_PORT);
    if (cli_sock_fd[0] == -1) {
        printf("Failed to open CLI test server port\n");
    }

    // Set up socket to the VDM message port
    video_msg_fd = connect_socket(server, VIDEO_MSG_PORT);
    if (video_msg_fd == -1) {
        printf("Failed to open video message port\n");
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, cli_loop, NULL) != 0) {
        perror("pthread_create");
        exit(-1);
    }

    pthread_join(tid, NULL);

    return 0;
}

