///  Copyright (c) 2011-2015 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      hash.c
///  @brief     Hash utilities
///  @author    Yi Tong Tse


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include "common.h"
#include "que.h"
#include "hash.h"


hash_table_t* hash_table_create (int num_slot,
                                 hash_code_func_t hash_func,
                                 hash_match_func_t match_func,
                                 hash_print_func_t print_func)
{
    int i;
    int rc;

    hash_table_t* tab = calloc(1, sizeof(hash_table_t)
                                          + num_slot * sizeof(que_elem_t));
    if (!tab)  return NULL;

    tab->item_cnt = 0;
    tab->num_slot = num_slot;
    tab->hash_func = hash_func;
    tab->match_func = match_func;
    tab->print_func = print_func;

    rc = pthread_mutex_init(&tab->mutex, NULL);
    assert(rc == EOK);
    for (i=0; i<num_slot; i++) {
        que_init(NULL, tab->slot + i);
    }
    return tab;
}


void hash_table_free (hash_table_t *hash_tab)
{
    assert(hash_tab);
    pthread_mutex_destroy(&hash_tab->mutex);
    free(hash_tab);
}


hash_item_t* hash_table_lookup (hash_table_t *hash_tab, void *key)
{
    uint32 hash_code;
    que_elem_t *elem;

    assert(hash_tab);
    assert(key);

    hash_code = hash_tab->hash_func(key);
    FOR_ALL_ELEMENTS_IN_QUE(&hash_tab->slot[hash_code % hash_tab->num_slot],
                            elem) {
        hash_item_t* item = (hash_item_t*)elem;
        if (item->hash_code == hash_code && hash_tab->match_func(key, item)) {
            return item;
        }
    }
    return NULL;
}


void hash_table_add (hash_table_t *hash_tab, void *key, hash_item_t *item)
{
    assert(hash_tab);
    assert(key);

    item->hash_code = hash_tab->hash_func(key);
    que_put(&hash_tab->mutex,
            &hash_tab->slot[item->hash_code % hash_tab->num_slot],
            &item->link);
    hash_tab->item_cnt++;
}


hash_item_t* hash_table_delete (hash_table_t *hash_tab, void *key)
{
    hash_item_t* item;

    assert(hash_tab);
    assert(key);
    
    item = hash_table_lookup(hash_tab, key);
    if (item) {
        que_deque(&hash_tab->mutex, &item->link);
        hash_tab->item_cnt--;
    }
    return item;
}


#define MAX_HIST_BINS 6
void hash_table_print_summary (FILE *fp, hash_table_t *hash_tab)
{

    int i;
    int bin_size;
    int hash_histogram[MAX_HIST_BINS];
    uint32 tot = 0;

    memset(hash_histogram, 0, sizeof(hash_histogram));

    fprintf(fp, "Number of items: %d\n", hash_tab->item_cnt);
    // prepare histogram for all bin sizes
    for (i=0; i<hash_tab->num_slot; i++) {
        bin_size = que_get_size(NULL, hash_tab->slot + i);
        // level out maximum bin size
        if (bin_size >= MAX_HIST_BINS -1) {
            bin_size = MAX_HIST_BINS -1;
        }
        hash_histogram[bin_size]++;
    }
    // now print this histogram
    fprintf(fp, "Session histogram:\n");
    // loop till 1 less than max bins since
    // last bin holds item count of (MAX_HIST_BINS -1) or more items
    for (i=0;i<MAX_HIST_BINS ; i++) {
        if (hash_histogram[i] > 0) { 
            //check for last bin
            if (i == MAX_HIST_BINS -1) {
                fprintf(fp, "Bin >= %d: Items %d\n",i, hash_histogram[i]);
            } else { 
                fprintf(fp, "Bin with %d: Items %d\n", i, hash_histogram[i]);
            }
            tot += hash_histogram[i];
        }
    }
    fprintf(fp, "\nTotal Slots: %d\n", tot);
}


void hash_table_print (FILE *fp, hash_table_t *hash_tab)
{
    int i;
    que_elem_t *elem;

    fprintf(fp, "Number of items: %d\n", hash_tab->item_cnt);

    for (i=0; i<hash_tab->num_slot; i++) {
        if (!que_is_empty(NULL, hash_tab->slot + i)) {
            fprintf(fp, "Slot %d\n", i);
            FOR_ALL_ELEMENTS_IN_QUE(hash_tab->slot + i, elem) {
                hash_tab->print_func(fp, (hash_item_t*)elem);
            }
        }
    }

}

