/*
 * Copyright (c) 2005-2013, 2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "common.h"
#include "util.h"


void fprint_hex (FILE* fp, void* ptr, int nbytes)
{
    int i;
    uint8 *temp = ptr;

    for (i=0; nbytes>0; nbytes--) {
        fprintf(fp, "%02x ", *temp++);
        if (++i==16) {
            i = 0;
            fprintf(fp, "\n");
        }
    }
    if (i) {
        fprintf(fp, "\n");
    }
}


#if DEBUG_MUTEX
void mutex_lock (pthread_mutex_t *mutex)
{
    int rc = pthread_mutex_lock(mutex);
    if (rc != EOK) {
        printf("mutex_lock: error %d\n", rc);
        bugtrace();
    }
}


void mutex_unlock (pthread_mutex_t *mutex)
{
    int rc = pthread_mutex_unlock(mutex);
    if (rc != EOK) {
        printf("mutex_lock: error %d\n", rc);
        bugtrace();
    }
}
#endif  // DEBUG_MUTEX


void print_elapsed_time (FILE *fp, uint64 sec)
{
    uint64 min = sec / 60;
    uint64 hr = min / 60;
    uint32 day = hr / 24;
    sec -= min * 60;
    min -= hr * 60;
    hr -= day * 24;
    fprintf(fp, "%d days %02d:%02d:%02d\n",
            day, (uint32)hr, (uint32)min, (uint32)sec);
}


