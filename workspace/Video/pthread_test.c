#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"


void* th1_main (void *not_used)
{
    printf("Thread 1 started\n");
    int i;
    for (i=0; i<10; i++) {
        printf("Thread 1: Hello %d\n", i);
        sleep(2);
    }
    return NULL;
}


void* th2_main (void *not_used)
{
    printf("Thread 2 started\n");
    int i;
    for (i=0; i<10; i++) {
        printf("     Thread 2: Hello %d\n", i);
        sleep(3);
    }
    return NULL;
}


int main (int argc, char *argv[])
{
    pthread_t th1, th2;

    pthread_create(&th1, NULL, th1_main, NULL);
    pthread_create(&th2, NULL, th2_main, NULL);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}
