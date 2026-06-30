/*
 * Copyright (c) 2005-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *
 * Set ERRMSG=0 will disable errmsg() and alarmsg().
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <unistd.h>
#include <pthread.h>
#include "common.h"


// Print memory range in hexadecimal
void fprint_hex(FILE *fp, void *ptr, int nbytes);

static inline
void print_hex(void *ptr, int nbytes)
{
    fprint_hex(stdout, ptr, nbytes);
}


// Routines for getting or setting slot ID of a linecard
int get_slot_id(void);
int set_slot_id(int slot);

// Routing for determining the process name given its process id
static inline void get_process_name(int pid, char * name) { if(!pid) {name[0] = '\0';}}


// Routine to force bus error
static inline
void bus_error (void)
{
//    *((int*)0) = 0;
    __builtin_trap();
}

// Routine for stack trace back
#if BUGTRACE == 1
void bugtrace(void);
#else
#define bugtrace()
#endif


// Routine for saving debugging information to the system log
#if BUGINF == 1
void buginf(const char *format, ...)
                __attribute__ ((format ( __printf__, 1, 2)));
#else
#define buginf(format, str...)    printf(format "\n", ##str)
#endif  // BUGINF == 1

#define trcinf(args...)  buginf(args)


// Utility for data prefetch to cache
#if defined(__QNX__)
#define data_hint(addr) \
    ({ __asm__ __volatile__( "dcbt 0,%0" :: "r" (addr) ); })

#elif defined(CYLONS_MIPS_TARGET)
#define data_hint(addr)   asm volatile ("pref %[type], %[off](%[rbase])" : : [rbase] "d" (addr), [off] "I" (0), [type] "n" (28))

#else
#define data_hint(addr)       NULL
#endif


// Utility to flush data to memory
#ifdef __QNX__
#define data_flush(addr) \
((void)0)
/*    ({ __asm__ __volatile__( "dcbf 0,%0" :: "r" (addr) ); }) */
#endif

#if defined(CYLONS_MIPS_TARGET)
// same as OCTEON II - CVMX_SYNCW
#define sync_readwrite()        asm volatile ("syncw\n" : : :"memory")
#else
#define sync_readwrite()        NULL
#endif

// Routine for mutex lock and unlock with error checking
#if DEBUG_MUTEX
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);
#else
#define mutex_lock      pthread_mutex_lock
#define mutex_unlock    pthread_mutex_unlock
#endif  // DEBUG_MUTEX


// For programmer-assist branch prediction
// (Only available on latest GNU compiler!)
//
#if 0
#define LIKELY(x)       __builtin_expect((x), 1)
#define UNLIKELY(x)     __builtin_expect((x), 0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif


void print_elapsed_time(FILE *fp, uint64 sec);

#endif /* __UTIL_H__ */
