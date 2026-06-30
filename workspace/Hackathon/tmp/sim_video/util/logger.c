/*
 * Copyright (c) 2005-2012, 2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include "common.h"
#include "logger.h"
#include "util.h"


#define LOGGER_MAX_MSG_SIZE     128


#if LOGGER == 1

logger_t* logger_init (const char *name, uint32 log_size)
{
    int logger_fd;
    logger_t *logger;
    pthread_mutexattr_t mutex_attr;
    boolean logger_exist = FALSE;
    int shm_size = sizeof(logger_t) + log_size + LOGGER_MAX_MSG_SIZE;

    // Create or open shared memory for logger buffer
    logger_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (logger_fd == -1 && errno == EEXIST) {
        // Shared memory already exists
        logger_fd = shm_open(name, O_RDWR, 0666);
        logger_exist = TRUE;
    }
    if (logger_fd == -1) {
        fprintf(stderr, "Failed to open shared memory %s for logger: %s\n",
                name, strerror(errno));
        return NULL;
    }

    /* Set the memory object's size */
    if (!logger_exist) {
        if (ftruncate(logger_fd, shm_size) == -1 ) {
            fprintf(stderr, "ftruncate: %s\n", strerror(errno));
            return NULL;
        }
    }

    /* Map the memory object */
    logger = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                  logger_fd, 0);
    if (logger == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        return NULL;
    }

    if (logger_exist) {
        if (log_size < logger->log_size) {
            // Mapped size smalled than log size.  Need to redo mmap.
            log_size = logger->log_size;
            munmap(logger, shm_size);
            shm_size = sizeof(logger_t) + log_size + LOGGER_MAX_MSG_SIZE;

            logger = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                          logger_fd, 0);
            if (logger == MAP_FAILED) {
                fprintf(stderr, "mmap failed: %s\n", strerror(errno));
                return NULL;
            }
        }
    } else {
        // Initialize mutex for logger
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        if (pthread_mutex_init(&logger->mutex, &mutex_attr) != EOK) {
            fprintf(stderr, "Failed to initialize mutex for logger: %s\n",
                    strerror(errno));
            return NULL;
        }

        pthread_mutex_lock(&logger->mutex);
        logger->log_size = log_size;
        logger->size = 0;
        logger->idx = 0;
        logger->lock_flag = 0;
        pthread_mutex_unlock(&logger->mutex);
    }

    return logger;
}


// Reset the logger
//   Will fail and return FALSE if logger does not exist, or is locked
//
boolean logger_reset (logger_t* logger)
{
    if (!logger || logger->lock_flag) {
        return FALSE;
    }

    pthread_mutex_lock(&logger->mutex);
    logger->size = 0;
    logger->idx = 0;
    pthread_mutex_unlock(&logger->mutex);
    return TRUE;
}


void logger_printf (logger_t* logger, const char* format, ... )
{
    int msg_size, tail_size;
    va_list arg_list;
    char* logger_buf;

    if (!logger || logger->lock_flag) {
        return;
    }

    logger_buf = logger->log_buf;

    pthread_mutex_lock(&logger->mutex);
    va_start(arg_list, format);
    msg_size = vsnprintf(logger_buf + logger->idx, LOGGER_MAX_MSG_SIZE,
                         format, arg_list);
    va_end(arg_list);

    logger->size += msg_size;
    logger->idx += msg_size;

    // Check for wrap around
    tail_size = logger->idx - logger->log_size;
    if (tail_size >= 0) {
        // Copy tail to beginning of buffer
        memcpy(logger_buf, logger_buf + logger->log_size, tail_size);
        logger->idx = tail_size;
    }

    pthread_mutex_unlock(&logger->mutex);
}


void logger_print_hex (logger_t* logger, void* ptr, int nbytes)
{
    int i;
    uint8 *temp = ptr;

    if (!logger || logger->lock_flag) {
        return;
    }

    pthread_mutex_lock(&logger->mutex);
    for (i=0; nbytes>0; nbytes--) {
        logger_printf(logger, "%02x ", *temp++);
        if (++i==16) {
            i = 0;
            logger_printf(logger, "\n");
        }
    }
    if (i) {
        logger_printf(logger, "\n");
    }
    pthread_mutex_unlock(&logger->mutex);
}


void logger_show (logger_t* logger, uint32 size)
{
    int msg_size, tail_size, idx;
    char* logger_buf;
    int lock_flag = logger->lock_flag;

    if (!logger) {
        return;
    }

    logger_lock(logger);        // Temporarily lock the logger

    if (size > logger->size) {
        size = logger->size;
    }
    if (size > logger->log_size) {
        size = logger->log_size;
    }

    idx = logger->idx - size;
    if (idx < 0) {
        idx += logger->log_size;
    }

    logger_buf = logger->log_buf;

    printf("-------- Log: size %d --------\n", size);
    while (size > 0) {
        msg_size = printf("%s", (char*)(logger_buf + idx));
        if (msg_size <= 0) {
            printf("\nLogger dump failed: %d\n", msg_size);
            return;
        }

        size -= msg_size;
        idx += msg_size;

        // Check for wrap around
        tail_size = idx - logger->log_size;
        if (tail_size >= 0) {
            idx = tail_size;
        }

//        delay(100);    // to slow down so text won't get dropped
    }

    if (!lock_flag) {
        logger_unlock(logger);
        printf("\n-------- End of log --------\n");
    } else {
        printf("\n-------- End of log (locked) --------\n");
    }
}

#endif  // LOGGER == 1

