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
#include "video.h"
#include "video_util.h"
#include "video_cli.h"
#include "sim_video.h"
#include "cgi_util.h"


#define TEMPLATE_DIR    "/home/pi/Hackathon/Site/cgi-bin/Templates"

#define MSG_TYPE_VIDEO_GET_PROG_STAT	903

#define VIDEO_SERVER	"127.0.0.1"

uint8 msg_buf[VIDEO_MSG_BUF_SIZE];


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


int main (int argc, char **argv)
{
    // Set up socket to the VDM message port
    int msg_fd = connect_socket(VIDEO_SERVER, 50002);
    if (msg_fd == -1) {
        printf("Failed to open video message port\n");
    }

    memset(msg_buf, 0, sizeof(msg_buf));
    ipc_message_header *ipc_hdr = IPC_HEADER(msg_buf);
    ipc_hdr->type = MSG_TYPE_VIDEO_GET_PROG_STAT;
    ipc_hdr->size = 0;
    int len = sizeof(ipc_message_header);
    int n = write(msg_fd, msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }

    n = read(msg_fd, msg_buf, VIDEO_MSG_BUF_SIZE);
    close(msg_fd);

    // Generate web-page
    char filename[100];
    printf("Context-type:text/html\n\n");
    sprintf(filename, "%s/prog_stat.html", TEMPLATE_DIR);
    return sub_template(filename, IPC_DATA(msg_buf));
}
