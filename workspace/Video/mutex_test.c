#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <assert.h>
#include "common.h"


pthread_mutex_t mutex0 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;


void* th1_main (void *not_used)
{
    pthread_mutex_lock(&mutex1);

    int i;
    for (i=0; i<5; i++) {
        printf("Thread 1: Hello %d\n", i);
        sleep(1);
    }

    pthread_mutex_unlock(&mutex1);
    return NULL;
}


void* th2_main (void *not_used)
{
    pthread_mutex_lock(&mutex2);

    int i;
    for (i=0; i<5; i++) {
        printf("Thread 2: Hello %d\n", i);
        sleep(2);
    }

    pthread_mutex_unlock(&mutex2);
    return NULL;
}


int main (int argc, char *argv[])
{
    pthread_t th1, th2;

    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);

    pthread_create(&th1, NULL, th1_main, NULL);
    pthread_create(&th2, NULL, th2_main, NULL);

    pthread_mutex_unlock(&mutex1);
    sched_yield();
    pthread_mutex_lock(&mutex1);

    pthread_mutex_unlock(&mutex2);
    sched_yield();
    pthread_mutex_lock(&mutex2);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}
