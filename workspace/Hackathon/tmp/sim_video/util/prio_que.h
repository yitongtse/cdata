/*
 *  Copyright (c) 2013, 2014 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __PRIO_QUE_H__
#define __PRIO_QUE_H__

// max video flows and then a safety factor of around 1.5
#define PRIO_QUE_MAX_SIZE     120

// function that returns True if swapping is needed
typedef int (*prio_que_item_compare_func_t)(void *a, void *b);

typedef struct {
    int count;
    prio_que_item_compare_func_t compare;
    void *data[PRIO_QUE_MAX_SIZE];
} prio_que_t;


static inline
int prio_que_is_empty (prio_que_t *pq)
{
    return !pq->count;
}


void prio_que_create(prio_que_t *pq, prio_que_item_compare_func_t cmp);

int prio_que_enqueue(prio_que_t *pq, void *item);

void* prio_que_dequeue(prio_que_t *pq);

void prio_que_percolate_down(prio_que_t *pq, int idx);

void prio_que_percolate_up(prio_que_t *pq, int idx);


#endif /* __PRIO_QUE_H__ */
