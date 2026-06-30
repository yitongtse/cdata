#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include "common.h"

// THIS CODE DOESN'T WORK !!

sem_t sem1, sem2;


void* th1_main (void *not_used)
{
    printf("Thread 1 started\n");
    sem_wait(&sem1);

    int i;
    for (i=0; i<10; i++) {
        printf("Thread 1: Hello %d\n", i);
        sleep(2);
    }

    sem_post(&sem1);
    return NULL;
}


void* th2_main (void *not_used)
{
    printf("Thread 2 started\n");
    sem_wait(&sem2);

    int i;
    for (i=0; i<10; i++) {
        printf("     Thread 2: Hello %d\n", i);
        sleep(3);
    }

    sem_post(&sem2);
    return NULL;
}


int main (int argc, char *argv[])
{
    int rc;
    pthread_t th1, th2;

    rc = sem_init(&sem1, 1, 0);
    assert(rc == 0);
    rc = sem_init(&sem2, 1, 0);
    assert(rc == 0);

    pthread_create(&th1, NULL, th1_main, NULL);
    pthread_create(&th2, NULL, th2_main, NULL);

    sleep(1);
    printf("Posting sem1\n");
    rc = sem_post(&sem1);
    assert(rc == 0);
    printf("Waiting sem1\n");
    rc = sem_wait(&sem1);
    assert(rc == 0);
    printf("Posting sem2\n");
    rc = sem_post(&sem2);
    assert(rc == 0);
    printf("Waiting sem2\n");
    rc = sem_wait(&sem2);
    assert(rc == 0);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}
