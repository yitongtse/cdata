#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>


timer_t timer_id;


void timer_thread (union sigval value)
{
    static int count = 0;
    printf("Timer expires %d, value %d\n", ++count, (int)value.sival_ptr);

    if (count == 10) {
        int rc;
        struct itimerspec its;

        // Start the timer
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;

        rc = timer_settime(timer_id, 0, &its, NULL);
        if (rc == -1) {
            perror("timer_settime");
            exit(-1);
        }
    }

}


int main (int argc, char *argv[])
{
    int rc;
    struct sigevent se;
    struct itimerspec its;

    // Create the timer
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_int = 1234;
    se.sigev_notify_function = timer_thread;
    se.sigev_notify_attributes = NULL;
    rc = timer_create(CLOCK_REALTIME, &se, &timer_id);
    if (rc == -1) {
        perror("timer_create");
        exit(-1);
    }

    // Start the timer
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;
    rc = timer_settime(timer_id, 0, &its, NULL);
    if (rc == -1) {
        perror("timer_settime");
        exit(-1);
    }

    while (1) {
        sleep(10);
    }

    // Create the timer
    exit(EXIT_SUCCESS);
}
