/*
 * Copyright (c) 2005-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include "common.h"
#include "util.h"
#include "que.h"
#include "pool.h"


int pool_create (pool_t *pool, int elem_size, int num_elem, void *ptr)
{
    if (!ptr) {
        ptr = pool->buf = calloc(num_elem, elem_size);
        pool->self_allocated = TRUE;
        if (!ptr) {
            //buginf("pool_create: Failed to allocate buffer pool");
            printf("pool_create: Failed to allocate buffer pool: %d x %d\n",
                   elem_size, num_elem);
            return ENOMEM;;
        }
    } else {
        pool->self_allocated = FALSE;
    }

    pool->buf = ptr;
    pool->count = num_elem;

    int rc = pthread_mutex_init(&pool->mutex, NULL);
    if (rc != EOK) {
        printf("pool_create: Failed to init mutex: error %d\n", rc);
        bugtrace();
    }

    que_init(&pool->mutex, &pool->head);

    uint8 *ptr2;
    for (ptr2 = ptr; num_elem > 0; num_elem--, ptr2 += elem_size) {
        que_put(&pool->mutex, &pool->head, (que_elem_t*)ptr2);
    }

    return EOK;
}


int pool_init (pool_t *pool, que_elem_t* buf_list)
{
    int rc = pthread_mutex_init(&pool->mutex, NULL);
    if (rc != EOK) {
        printf("pool_init: Failed to init mutex: error %d\n", rc);
        bugtrace();
    }

    que_init(&pool->mutex, &pool->head);
    que_append(NULL, &pool->head, buf_list);
    pool->count = que_get_size(NULL, &pool->head);
    pool->self_allocated = FALSE;
    pool->name[0] = '\0';

    return EOK;
}


void pool_destroy (pool_t *pool)
{
    if (pool->self_allocated && pool->buf) {
        free(pool->buf);
    }

    int rc = pthread_mutex_destroy(&pool->mutex);
    if (rc != EOK) {
        printf("Failed to destroy mutex: error %d\n", rc);
        bugtrace();
    }
}


que_elem_t* pool_alloc (pool_t *pool)
{
    mutex_lock(&pool->mutex);

    if (que_is_empty(NULL, &pool->head)) {
        mutex_unlock(&pool->mutex);
//        bugtrace();
        return NULL;
    }

    pool->count--;
    que_elem_t* elem = que_get(NULL, &pool->head);

    mutex_unlock(&pool->mutex);
    return elem;
}


void pool_get (pool_t *pool, que_elem_t *elem)
{
    mutex_lock(&pool->mutex);
    pool->count--;
    que_deque(NULL, elem);
    mutex_unlock(&pool->mutex);
}


void pool_free (pool_t *pool, que_elem_t* elem)
{
    mutex_lock(&pool->mutex);
    que_put(NULL, &pool->head, elem);
    pool->count++;
    mutex_unlock(&pool->mutex);
}


void pool_free_list (pool_t *pool, que_elem_t* list)
{
    mutex_lock(&pool->mutex);
    int num_elem = que_get_size(NULL, list);
    que_append(NULL, &pool->head, list);
    pool->count += num_elem;
    mutex_unlock(&pool->mutex);
}

