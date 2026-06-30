/*
 *  Copyright (c) 2005-2012 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __QUE_H__
#define __QUE_H__

#include <stdio.h>
#include  <pthread.h>
#include  <stdarg.h>
#include "common.h"


// Queue element
//
typedef struct que_elem_ {
  struct que_elem_   *next;
  struct que_elem_   *prev;
} que_elem_t;


// Initialize a queue
//
void que_init( pthread_mutex_t *mutex, que_elem_t *link );


// Insert an element to the end of the queue
//
void que_put( pthread_mutex_t *mutex, que_elem_t *link, que_elem_t *elem );


// Remove the first element from the queue and return it
//
que_elem_t* que_get( pthread_mutex_t *mutex, que_elem_t *link );


// Dequeue the element from its queue
//
void que_deque( pthread_mutex_t *mutex, que_elem_t *elem );


// Append the entire queue pointed to by list to the end of the que
// pointed to by link
//
void que_append(pthread_mutex_t *mutex, que_elem_t *link, que_elem_t* list);


// Check if the queue is empty
//
boolean que_is_empty( pthread_mutex_t *mutex, que_elem_t *link );


// Count the number elements on a queue
//
int que_get_size(pthread_mutex_t *mutex, que_elem_t *que);


// Iterate on all elements on the queue
//     FOR_ALL_ELEMENTS_IN_QUE(que_elem_t *que, que_elem_t *elem) {
//         ....
//     }
//
// Note: Care must be taken if the queue is modified when using this macro.
//       elem should be adjusted to point to the element before the next one
//       to be processed when continuing with the loop.
//
#define FOR_ALL_ELEMENTS_IN_QUE(que, elem)      \
        for ((elem) = (void*)((que)->next); (que_elem_t*)(elem) != (que); \
             (elem) = (void*)(((que_elem_t*)(elem))->next))


static inline
void print_que_elem (que_elem_t *elem)
{
    printf("link %" PRIxPTR ", prev %" PRIxPTR ", next %" PRIxPTR,
           (uintptr_t)elem, (uintptr_t)elem->prev, (uintptr_t)elem->next);
}

#endif /* __QUE_H__ */
