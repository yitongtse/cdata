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
#include "param.h"


#define EOK    0

int num_worker;

typedef struct {
    int data;
} worker_message_t;



static void* sender_main (void *arg)
{
    int n = (int)arg;
    mqd_t mqfd;
    struct mq_attr attr;
    int msg_len;
    worker_message_t msg;
    int i;
    int rc;

    printf("Sender starting...\n");

    // Set up attributes for message queue
    attr.mq_flags = 0;
    attr.mq_maxmsg = 1;
    attr.mq_msgsize = 1024;

    mqfd = mq_open("/test", O_CREAT | O_WRONLY, 0666, &attr);
    if (mqfd == -1) {
        perror("mq_open");
        exit(-1);
    }

    for (i=0; i<10; i++) {
        msg.data = i;
        rc = mq_send(mqfd, (char*)&msg, sizeof(worker_message_t), 1);
        if (rc == -1) {
            perror("mq_send");
        }
        printf("Msg %d sent\n", i);
    }

sleep(10);

    if (mq_close(mqfd) == -1) {
        perror("mq_close");
    }
    mq_unlink("/myipc");

    return NULL;
}


static void* worker_main (void *arg)
{
    int i;
    int rc;
    mqd_t mqfd;
    struct mq_attr attr;
    char msg_buf[1024];
    worker_message_t *msg = (worker_message_t*)msg_buf;
    int msg_len;
    int msg_prio;

    int n = (int)arg;
    printf("Worker %d starting...\n", n);

    // Set up attributes for message queue
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;

    mqfd = mq_open("/test", O_CREAT | O_RDONLY, 0666, &attr);
    if (mqfd == -1) {
        perror("mq_open");
        mq_close(mqfd);
        mq_unlink("/myipc");
        exit(-1);
    }

    while (1) {
        msg_len = mq_receive(mqfd, msg_buf, 1024, 0);
        if (msg_len == -1) {
            perror("mq_receive");
            return NULL;
        }
        printf("Worker %d: got msg %d, size %d\n", n, msg->data, msg_len);
//        sleep(1);
    }

    printf("Worker %d ending...\n", n);
    return NULL;
}


int main (int argc, char** argv)
{
    int rc;
    int i;
    param_t par;
    pthread_t sender_pid;
    pthread_t *worker_pid;
    void *res;

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "lnx_test.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "Linux Simulation");
    param_get_int(&par, "Number of worker threads", &num_worker, 4);
    param_end(&par);

    worker_pid = calloc(num_worker, sizeof(pthread_t));

    // Create the sender thread
    rc = pthread_create(&sender_pid, NULL, sender_main, (void*)i);
    if (rc != EOK) {
        perror("pthread_create");
        return -1;
    }
    sleep(1);

    for (i=0; i<num_worker; i++) {
        rc = pthread_create(&worker_pid[i], NULL, worker_main, (void*)i);
        if (rc != EOK) {
            perror("pthread_create");
            return -1;
        }
    }

    rc = pthread_join(sender_pid, &res);
    if (rc != EOK) {
        perror("pthread_join");
        return -1;
    }
    for (i=0; i<num_worker; i++) {
        rc = pthread_join(worker_pid[i], &res);
        if (rc != EOK) {
            perror("pthread_join");
            return -1;
        }
    }

    return 0;
}
