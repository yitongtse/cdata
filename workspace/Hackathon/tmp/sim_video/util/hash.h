///  Copyright (c) 2011-2015 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      hash.h
///  @brief     Header file for Hash utilities
///  @author    Yi Tong Tse
///

#ifndef __HASH_H__
#define __HASH_H__

#include <pthread.h>
#include <assert.h>
#include "common.h"
#include "que.h"


/// Hash item
typedef struct {
    que_elem_t link;
    uint32 hash_code;
} hash_item_t;


/// Type of function to get hash code from a key
typedef uint32 (*hash_code_func_t)(void *key);

/// Type of function to determine if the item matches the key
typedef boolean (*hash_match_func_t)(void *key, hash_item_t *item);

/// Type of function to print content of an item
typedef void (*hash_print_func_t)(FILE *fp, hash_item_t *item);


/// Hash table
typedef struct {
    int num_slot;                       // number of slots
    hash_code_func_t hash_func;
    hash_match_func_t match_func;
    hash_print_func_t print_func;
    int32 item_cnt;                     // current number of items in the table
    pthread_mutex_t mutex;
    que_elem_t slot[0];                 // array for the slots
} hash_table_t;


/// Create hash table
///
/// @param hash_tab         Pointer to caller allocated hash table
/// @param num_slot         Number of slots in the hash table
/// @param hash_func        Fuction to get hash code from a key
/// @param match_func       Fuction to determine if the item has a matching key
/// @param print_func       Fuction to print content of an item
///
/// @return
///   - pointer to the hash table created if successful
///   - NULL in case of any errors
///
hash_table_t* hash_table_create(int num_slot,
                                hash_code_func_t hash_func,
                                hash_match_func_t match_func,
                                hash_print_func_t print_func);


/// Free hash table
///
/// @param hash_tab         Pointer to hash table to be freed
///
void hash_table_free(hash_table_t *hash_tab);


/// Look up the first matching item with a given key
///
/// @param hash_tab     Pointer to caller allocated hash table
/// @param key          Pointer to the key to be looked up
///
/// @return
///   - Pointer to the first matching item found
///   - NULL if no matching item found
///
hash_item_t* hash_table_lookup(hash_table_t *hash_tab, void *key);


/// Add a new item to hash table
///
/// @param hash_tab     Pointer to caller allocated hash table
/// @param key          Pointer to the key to be added
/// @param item         Pointer to the item to be added
///
/// @note
///   - will set hash_code field in item
///   - will not check for uniqueness of item
///
void hash_table_add(hash_table_t *hash_tab, void *key, hash_item_t *item);


/// Delete the first matchig item with a given key
///
/// @note
///   - in case there are multiple items with matching key, the item added
///     first will be deleted
///   - the item will NOT be linked after returned from API
///
/// @param hash_tab     Pointer to caller allocated hash table
/// @param key          Pointer to the key to be looked up
///
/// @return
///   - Pointer to item deleted
///   - NULL if no matching item found
///
hash_item_t* hash_table_delete(hash_table_t *hash_tab, void *key);


/// Print content of hash table
///
/// @param hash_tab     Pointer to caller allocated hash table
///
void hash_table_print(FILE *fp, hash_table_t *hash_tab);

/// Print summary of hash table in form of histogram of 
/// of number of sessions in a slot
///
/// @param hash_tab     Pointer to caller allocated hash table
///
void hash_table_print_summary(FILE *fp, hash_table_t *hash_tab);


/// compute crc 16 
/// @param  iv uint16
/// @param  p uint64
/// 
/// @return 
///   - calculated crc uint16
uint16_t hash_crc16(uint16_t iv, uint64_t p);

#endif    // __HASH_H__
