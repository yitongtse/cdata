/*
 * Copyright (c) 2005-2008, 2010-2013, 2015 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * NOTE:
 *   LOGGER = 0 : compile out all logger APIs
 *   LOGGER = 1 : normal logger behavior
 *   LOGGER = 2 : logger_printf() is same as printf()
 *
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "common.h"
#include <pthread.h>


// Logger utility
//
typedef struct logger_ {
    pthread_mutex_t mutex;
    uint32 log_size;            // logger size
    uint32 size;                // total message size
    int32  idx;                 // index for write pointer
    uint32 lock_flag;           // whether the log is locked
    char  log_buf[0];           // beginning of log buffer
} logger_t;




#if LOGGER == 0

#define logger_init(...)                NULL
#define logger_reset(name)              NULL
#define logger_lock(...)                NULL
#define logger_unlock(...)              NULL
#define logger_show(...)                NULL
#define logger_printf(...)              NULL
#define logger_print_hex(...)           NULL

#endif  // LOGGER == 0




#if LOGGER == 1

logger_t* logger_init(const char *name, uint32 log_size);

boolean logger_reset(logger_t* logger);

static inline
void logger_lock(logger_t* logger)
{
    logger->lock_flag = 1;
}

static inline
void logger_unlock(logger_t* logger)
{
    logger->lock_flag = 0;
}

void logger_show(logger_t* logger, uint32 size);

void logger_printf(logger_t* logger, const char* format, ...)
                  __attribute__ ((format ( __printf__, 2, 3)));

void logger_print_hex(logger_t* log, void* ptr, int nbytes);

#endif  // LOGGER == 1




#if LOGGER == 2

#define logger_init(name, log_size)             NULL
#define logger_reset(...)                       NULL
#define logger_lock(...)                        NULL
#define logger_unlock(...)                      NULL
#define logger_show(...)                        NULL
#define logger_printf(logger, args...)          printf(args)
#define logger_print_hex(logger, args...)       print_hex(args)

#endif  // LOGGER == 2



#endif /* __LOGGER_H__ */
