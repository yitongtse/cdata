/*
 *  Copyright (c) 2013, 2013 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "common.h"
#include "prio_que.h"


void prio_que_create (prio_que_t *pq, prio_que_item_compare_func_t cmp)
{
    pq->count = 0;
    pq->compare = cmp;
    memset(pq->data, 0, sizeof(void*) * PRIO_QUE_MAX_SIZE);
}


void prio_que_percolate_down (prio_que_t *pq, int idx)
{
    void* tmp = pq->data[idx];
    int child = (idx << 1) + 1;

    while (child < pq->count) {
        if (child < pq->count - 1
                && pq->compare(pq->data[child + 1], pq->data[child])) {
            child++;
        }
        if (!pq->compare(pq->data[child], tmp))  break;
        pq->data[idx] = pq->data[child];
        idx = child;
        child = (idx << 1) + 1;
    }
    pq->data[idx] = tmp;
}


void prio_que_percolate_up (prio_que_t *pq, int idx)
{
    void* tmp = pq->data[idx];

    while (idx > 0) {
        int parent = (idx - 1) >> 1;
        if (!pq->compare(tmp, pq->data[parent]))  break;
        pq->data[idx] = pq->data[parent];
        idx = parent;
    }
    pq->data[idx] = tmp;
}


int prio_que_enqueue (prio_que_t *pq, void *item)
{
    int idx = pq->count++;
    if (pq->count > PRIO_QUE_MAX_SIZE)  return E2BIG;
    pq->data[idx] = item;
    prio_que_percolate_up(pq, idx);
    return EOK;
}


void* prio_que_dequeue (prio_que_t *pq)
{
    void* item = pq->data[0];
    if (prio_que_is_empty(pq))  return NULL;
    pq->data[0] = pq->data[--pq->count];
    prio_que_percolate_down(pq, 0);
    return item;
}

