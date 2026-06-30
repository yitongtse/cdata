/*
 * Copyright (c) 2005-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __POOL_H__
#define __POOL_H__


#include <pthread.h>
#include "que.h"


#define POOLNAME_LEN   16

typedef struct pool_t_ {
    void*           buf;
    que_elem_t      head;
    pthread_mutex_t mutex;
    int             count;
    boolean         self_allocated;
    char            name[POOLNAME_LEN];
} pool_t;


// Create a pool of fixed sized buffers
//   Arguments:
//     pool: points to pool to be created
//     elem_size: size in bytes of each buffer element
//     num_elem: number of buffer elements
//     ptr: points to pre-allocated buffer for pool elements
//          Elements in the pool should have que_elem_t as their first field.
//          (If ptr is NULL, the pool will allocate its own buffer.)
//
//   Returns:
//     EXIT_SUCCESS if succeeds.
//     EXIT_FAILURE if fails.
//
int pool_create(pool_t *pool, int elem_size, int num_elem, void *ptr);


// Initialize a pool with a given list of buffers
//
int pool_init(pool_t *pool, que_elem_t* buf_list);


// Destroy the pool
//   If the pool owns the buffer, will also free it.
//
void pool_destroy(pool_t *pool);


// Check if the pool is empty
//
static inline
boolean pool_is_empty (pool_t *pool)
{
    return que_is_empty(&pool->mutex, &pool->head);
}


// Allocate a buffer element from the pool
//
que_elem_t* pool_alloc(pool_t *pool);


// Get a selected buffer element from the pool
// Note: the selected element must be linked to the pool!
//
void pool_get(pool_t *pool, que_elem_t *elem);


// Return a buffer element to the pool
//
void pool_free(pool_t *pool, que_elem_t *elem);


// Return the list of buffer elements pointed to by list to the pool
//
void pool_free_list(pool_t *pool, que_elem_t *list);


static inline
int pool_get_elem_id (pool_t *pool, que_elem_t *elem, int elem_sz)
{
    return (((uintptr_t)elem) - ((uintptr_t)pool->buf)) / elem_sz;
}


#endif /* __POOL_H__ */
