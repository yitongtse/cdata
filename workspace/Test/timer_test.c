#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <assert.h>


#define SIG SIGRTMIN

static
void timer_handler (int sig, siginfo_t *si, void *uc)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("PID %x: time %d - %d\n", pthread_self(), ts.tv_sec, ts.tv_nsec);
}


int main (int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: %s <time_interval>\n", argv[0]);
        exit (-1);
    }

    int interval = atoi(argv[1]);

    // Set up signal handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    int rc = sigaction(SIG, &sa, NULL);
    if (rc == -1) {
        perror("sigactinon");
        exit(-1);
    }

    timer_t timer_id;
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timer_id;

    // Create timer
    rc = timer_create(CLOCK_REALTIME, &sev, &timer_id);
    if (rc == -1) {
        perror("timer_create");
        exit(-1);
    }

    struct itimerspec its;
    its.it_value.tv_sec = interval;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = interval;
    its.it_interval.tv_nsec = 0;

    // Start timer
    rc = timer_settime(timer_id, 0, &its, NULL);
    if (rc == -1) {
        perror("timer_settime");
        exit(-1);
    }

    while (1) {
        sleep(1);
    }
}

