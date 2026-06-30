/*
 * Copyright (c) 2010-2011 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/dispatch.h>
#include <sys/mman.h>


#define PULSE_CODE_TIMER        (_PULSE_CODE_MINAVAIL)
#define PULSE_CODE_WORKER       (_PULSE_CODE_MINAVAIL + 1)

#define NUM_WORK                20


typedef struct {
    int count;
    int value;
} work_t;


int num_cores;
int* chid_worker;
int* coid_worker;
work_t* work;

pthread_barrier_t barrier;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int work_id = NUM_WORK;

dispatch_t *_dispatch_create(int chid, unsigned flags);


void do_work (work_t* work)
{
    volatile int i;

    for (i = 0; i < work->count; i++) {
        work->value = i;
        i = work->value;
    }
    work->value = 0;
}


int worker_pulse_handler (message_context_t *ctp, int code, unsigned flags,
                          void *handle)
{
    int core_id = (int)handle;
    int id;

//    printf("Core %d worker receives pulse\n", core_id);

    while (1) {
        pthread_mutex_lock(&mutex);
        id = work_id++;
        pthread_mutex_unlock(&mutex);

        if (id >= NUM_WORK) {
             break;
         }

       printf("Workder %d: work %d\n", core_id, id);
        if (work[id].value != 0) {
            printf("Error: Work %d is in progress!\n", id);
        }
        do_work(&work[id]);
    }

    return 0;
}


void* worker_main (void *arg)
{
    int rc;
    int core_id = (int)arg;
    int run_mask = 1 << core_id;
    int chid;
    dispatch_t *dpp;
    dispatch_context_t *ctp;

    // Set run mask
    rc = ThreadCtl(_NTO_TCTL_RUNMASK, &run_mask);
    if (rc == -1) {
        printf("Failed to set run mask %04x\n", run_mask);
    }

    // Create channel
    chid = ChannelCreate(0);
    if (chid == -1) {
        printf("Failed to create channel: %s\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }
    chid_worker[core_id] = chid;

    // Create dispatch
    dpp = _dispatch_create(chid, 0);
    if (dpp == NULL) {
        printf("Failed to create dispatch: %s\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    // Attach pulse handler
    rc = pulse_attach(dpp, NULL, PULSE_CODE_WORKER, &worker_pulse_handler,
                          (void*)core_id);
    if (rc == -1) {
        printf("Failed to attach pulse handler: %s\n", strerror(errno));
        return (void*)EXIT_FAILURE;
    }

    // Allocate dispatch context
    ctp = dispatch_context_alloc(dpp);
    if (ctp == NULL) {
        printf("Failed to allocate dispatch context: %s\n",
                       strerror(errno));
        return (void*)EXIT_FAILURE;
    }

//    printf("Core %d ready: Chan ID %d\n", core_id, chid);

    rc = pthread_barrier_wait(&barrier);
    if (rc != EOK && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Worker %d: Failed at barrier: %d\n", core_id, rc);
    }


    // Main loop
    while (1) {
        if ((ctp = dispatch_block(ctp))) {
            rc = dispatch_handler(ctp);
            assert(rc == 0);
        }
    }

    return (void*)EOK;
}


// Timer event handler
//
int timer_event_handler (message_context_t *ctp, int code, unsigned flags,
                         void *handle)
{
    int i;
    int rc;
    int id;
    int busy = 1;

    printf("\nReceived timer event\n");

    pthread_mutex_lock(&mutex);
    id = work_id;
    if (work_id >= NUM_WORK) {
        work_id = 0;
        busy = 0;
    }
    pthread_mutex_unlock(&mutex);

    if (busy) {
        printf("Workers still busy at work %d!  Skip this round.\n", id);
        return 0;
    }

    // Send pulse to all worker
    for (i=0; i<num_cores; i++) {
        rc = MsgSendPulse(coid_worker[i], 12, PULSE_CODE_WORKER, 0);
        if (rc == -1) {
            printf("Failed to send pulse to worker on core %d: %s\n",
                   i, strerror(errno));
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    int i;
    int rc;

    pthread_t *tids;
    int chid;
    int coid;
    dispatch_t *dpp;
    dispatch_context_t *ctp;
    struct sigevent timer_event;
    timer_t timer;
    struct itimerspec itimer;

    num_cores = _syspage_ptr->num_cpu;
     printf("No. of cores: %d\n", num_cores);

     chid_worker = calloc(sizeof(int), num_cores);
     coid_worker = calloc(sizeof(int), num_cores);
    tids = malloc(num_cores * sizeof(pthread_t));

#if 0
    work = mmap(NULL, sizeof(work_t) * NUM_WORK,
                PROT_NOCACHE | PROT_READ | PROT_WRITE, MAP_PHYS, NOFD, 0);
    if (work == MAP_FAILED) {
        printf("Failed to allocate memory: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
#endif
    work = calloc(sizeof(work_t), NUM_WORK);

    for (i=0; i<NUM_WORK; i++) {
        work[i].count = 1000000 + 1000 * i;
        work[i].value = 0;
    }

    pthread_barrier_init(&barrier, NULL, num_cores + 1);

    // Create worker threads, one on each core
    for (i=0; i<num_cores; i++) {
        pthread_create(&tids[i], NULL, worker_main, (void*)i);
    }

    // Make sure all worker threads have created their channels
    rc = pthread_barrier_wait(&barrier);
    if (rc != EOK && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Failed at barrier: %d\n", rc);
        return EXIT_FAILURE;
    }

    // Connect to each core
    for (i=0; i<num_cores; i++) {
        coid_worker[i] = ConnectAttach(0, 0, chid_worker[i],
                                       _NTO_SIDE_CHANNEL, 0);
//        printf("Connect to core %d: %d\n", i, coid_worker[i]);
    }

    // Create channel
    chid = ChannelCreate(0);
    if (chid == -1) {
        printf("Failed to create channel: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Create a connection to itself
    coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
    if (coid == -1) {
        printf("Failed to attach to channel: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Create dispatch
    dpp = _dispatch_create(chid, 0);
    if (dpp == NULL) {
        printf("Failed to create dispatch: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Set up pulse handler
    rc = pulse_attach(dpp, NULL, PULSE_CODE_TIMER, &timer_event_handler, NULL);
    if (dpp == NULL) {
        printf("Failed to attach pulse handler: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Allocate dispatch context
    ctp = dispatch_context_alloc(dpp);
    if (ctp == NULL) {
        printf("Failed to create dispatch context: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Set up a timer
    SIGEV_PULSE_INIT(&timer_event, coid, 10, PULSE_CODE_TIMER, 0);
    rc = timer_create(CLOCK_REALTIME, &timer_event, &timer);
    if (rc == -1) {
        printf("Failed to create timer: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    itimer.it_value.tv_sec = 1;
    itimer.it_value.tv_nsec = 0;
    itimer.it_interval.tv_sec = 2;
    itimer.it_interval.tv_nsec = 0;
    rc = timer_settime(timer, 0, &itimer, NULL);
    if (rc == -1) {
        printf("Failed to set timer: %s\n", strerror(rc));
        return EXIT_FAILURE;
    }


    // Main loop
    while (1) {
        if ((ctp = dispatch_block(ctp))) {
            rc = dispatch_handler(ctp);
            assert(rc == 0);
        }
    }

#if 0
    for (i=0; i<num_cores; i++) {
        pthread_join(tids[i], NULL);
    }

    printf("End of test\n");
#endif

    return EXIT_SUCCESS;
}
