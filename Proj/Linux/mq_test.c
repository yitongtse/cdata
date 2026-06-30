#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <mqueue.h>
#include <errno.h>
#include <pthread.h>


#define MAX_MSG_SIZE         1024


static void* sender_main (void *arg)
{
    int i;
    int rc;
    char msg_buf[MAX_MSG_SIZE];
    struct mq_attr attr;
    int msg_len;
    int msg_prio;
    mqd_t mqfd;

    printf("Hello from sender\n");

    // Set up attributes for message queue
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;

    mqfd = mq_open("/myipc", O_CREAT | O_WRONLY, 0666, &attr);
    if (mqfd == -1) {
        perror("mq_open");
        exit(-1);
    }

    sprintf(msg_buf, "HELLO");

    msg_len = 6;
    msg_prio = 1;

    for (i=0; i<4; i++) {
        printf("Send: %s\n", msg_buf);
        rc = mq_send(mqfd, msg_buf, msg_len, msg_prio);
        if (rc == -1) {
            perror("mq_send");
        }
    }

    if (mq_close(mqfd) == -1) {
        perror("mq_close");
    }
    mq_unlink("/myipc");

    return NULL;
}


static void* receiver_main (void *arg)
{
    int i;
    int rc;
    mqd_t mqfd;
    char msg_buf[MAX_MSG_SIZE];
    struct mq_attr attr;
    int msg_len;
    int msg_prio;

    printf("Hello from receiver\n");

    // Set up attributes for message queue
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;

    mqfd = mq_open("/myipc", O_CREAT | O_RDONLY, 0666, &attr);
    if (mqfd == -1) {
        perror("mq_open");
        mq_close(mqfd);
        mq_unlink("/myipc");
        exit(-1);
    }

    while (1) {
        msg_len = mq_receive(mqfd, msg_buf, MAX_MSG_SIZE, 0);
        if (rc == -1) {
            perror("mq_receive");
        }
        printf("Recv: %s\n", msg_buf);
    }
}


main ()
{
    int rc;
    pthread_t sender_pid, receiver_pid;
    void *res;

    // Create sender thread
    pthread_create(&sender_pid, NULL, sender_main, NULL);

    // Create receiver thread
    pthread_create(&receiver_pid, NULL, receiver_main, NULL);

    rc = pthread_join(sender_pid, &res);
    rc = pthread_join(receiver_pid, &res);
}
