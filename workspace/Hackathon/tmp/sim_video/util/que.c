/*****************************************************************************
 * Copyright (c) 2003, 2005-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : que.c
 *
 *    Date      Who     Modification
 *    --------  ------  ------------------------------------------------------
 *    02-27-03  YWL     Created.
 *
 *****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include "common.h"
#include "util.h"
#include "que.h"


void que_init (pthread_mutex_t *mutex, que_elem_t *local_link)
{
    if (mutex != (pthread_mutex_t*)NULL) {
        mutex_lock(mutex);
    }

    local_link->next = local_link;
    local_link->prev = local_link;

    if (mutex != (pthread_mutex_t*)NULL) {
        mutex_unlock(mutex);
    }
}


void que_put (pthread_mutex_t *mutex, que_elem_t *local_link, que_elem_t *elem)
{
    if (mutex != (pthread_mutex_t*)NULL) {
        mutex_lock(mutex);
    }

#if DEBUG_QUE
    if (elem->prev != NULL || elem->next != NULL) {
        bugtrace();
        bus_error();
    }
#endif

    elem->prev = local_link->prev;
    elem->next = local_link;
    local_link->prev->next = elem;
    local_link->prev = elem;

    if (mutex != (pthread_mutex_t *) NULL) {
        mutex_unlock(mutex);
    }
}


que_elem_t *que_get (pthread_mutex_t *mutex, que_elem_t *local_link)
{
    if (mutex != (pthread_mutex_t *) NULL) {
        mutex_lock(mutex);
    }

#if DEBUG_QUE
    if (local_link->next == local_link) {
        // Que empty
        bugtrace();
        bus_error();
    }
#endif

    que_elem_t* elem = local_link->next;
    local_link->next = elem->next;
    local_link->next->prev = local_link;

#if DEBUG_QUE
    elem->prev = elem->next = NULL;
#endif

    if (mutex != (pthread_mutex_t *) NULL) {
        mutex_unlock(mutex);
    }
    return elem;
}


void que_deque (pthread_mutex_t *mutex, que_elem_t *elem)
{
    if (mutex != (pthread_mutex_t *) NULL) {
        mutex_lock(mutex);
    }

#if DEBUG_QUE
    if (elem->prev == NULL || elem->next == NULL) {
        bugtrace();
        bus_error();
    }
#endif

    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;

#if DEBUG_QUE
    elem->prev = elem->next = NULL;
#endif

    if (mutex != (pthread_mutex_t *) NULL) {
        mutex_unlock(mutex);
    }
}


void que_append (pthread_mutex_t *mutex, que_elem_t *local_link, 
                 que_elem_t* list)
{
    if (mutex != (pthread_mutex_t*)NULL) {
        mutex_lock(mutex);
    }

    if (!que_is_empty(NULL, list)) {
        local_link->prev->next = list->next;
        list->next->prev = local_link->prev;
        list->prev->next = local_link;
        local_link->prev = list->prev;
        list->next = list;
        list->prev = list;
    }

    if (mutex != (pthread_mutex_t *) NULL) {
        mutex_unlock(mutex);
    }
}


boolean que_is_empty (pthread_mutex_t *mutex, que_elem_t *local_link)
{
    if (mutex != (pthread_mutex_t *) NULL) {
	mutex_lock(mutex);
    }

    boolean is_empty = (local_link->next == local_link);
    
    if (mutex != (pthread_mutex_t *) NULL) {
	mutex_unlock(mutex);
    }
    return is_empty;
}


#define MAX_QUE_SIZE	200000

int que_get_size (pthread_mutex_t *mutex, que_elem_t *que)
{
    if (mutex != (pthread_mutex_t*)NULL) {
        mutex_lock(mutex);
    }

    que_elem_t *elem;
    int size = 0;

    FOR_ALL_ELEMENTS_IN_QUE(que, elem) {
        size++;
        assert(size <= MAX_QUE_SIZE);
        if (size > MAX_QUE_SIZE) {
            printf("Error: Que seems corrupted! Size = %d\n", size);
            if (mutex != (pthread_mutex_t*)NULL) {
                mutex_unlock(mutex);
            }
            return -1;
        }
    }

    if (mutex != (pthread_mutex_t*)NULL) {
        mutex_unlock(mutex);
    }

    return size;
}

