/*
 *-----------------------------------------------------------------------------
 * ermi_video_hash.h video control hash_table structure 
 *
 * January 2004, Xiaomei Liu
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *
 *
 *-----------------------------------------------------------------------------
 */
#ifndef __ERMI_VIDEO_HASH_H__
#define __ERMI_VIDEO_HASH_H__
/* 
  hash_obj is used to typecast all sorts 
  of objects to be stored in hash_table 
*/
#define VCP_MAX_HASH_TABLE_LEN     1009
typedef enum {
    VCP_HASH_TYPE_INT,
    VCP_HASH_TYPE_STRING
}vcp_hash_type_t;

typedef uint16 (*vcp_hash_func_t)(uint16 len, vcp_hash_type_t, void* id);
typedef void (*vcp_hash_print_func_t)(void* obj);
typedef void* (*vcp_hash_id_func_t)(void* obj);

typedef struct vcp_hash_table_{
    uint16    len;     /*number of slots*/
    vcp_hash_type_t type;
    list_header *entry; /*table_len * sizeof(list_header)*/
    vcp_hash_func_t  hash_func; /* mandatory */
    vcp_hash_id_func_t id_func; /* default is the first field */
    list_lookup_func_t lookup_func; /* mandatory */
    uint16    count;
} vcp_hash_table_t;

/* create an hash table, specify hash function and data_free_function */
vcp_hash_table_t *vcp_create_hash_table(uint16 table_len, 
           vcp_hash_type_t type, vcp_hash_func_t hash_func,
           vcp_hash_id_func_t id_func, list_lookup_func_t lookup_func);

/* destroy a hash table,
 * NOTE: hash table must be empty before deletion
 */
void vcp_destroy_hash_table(vcp_hash_table_t *table);

/* find object with this id */
void *vcp_find_hash_obj(vcp_hash_table_t *table, void* id);

/* add or delete a hash object
 * NOTE: object need to be created before calling vcp_add_hash_obj()
 *       Caller need to check if object already exist before calling
 *       vcp_add_hash_obj()
 */
boolean vcp_add_hash_obj(vcp_hash_table_t *table, void* obj);

/*
 * NOTE: object is removed from hash table by vcp_del_hash_obj()
 *       but the object is not freed by the vcp_del_hash_obj()
 */
boolean vcp_del_hash_obj(vcp_hash_table_t *table, void* obj);

/* default vcp hash functions */
uint16 vcp_int_hash_func(uint16 len, vcp_hash_type_t type, void* id);
uint16 vcp_string_hash_func(uint16 len, vcp_hash_type_t type, void* id);

/* default vcp hash lookup functions */
int vcp_int_lookup_func(void* id1, void* id2);
int vcp_string_lookup_func(void* id1, void* id2);

/* default vcp id function
 * NOTE: to use this default id function
 *       the first element of hash object
 *       must be the id
 */
void* vcp_default_id_func(void* obj);

/* dispaly hash table content */
void show_vcp_hash_table(vcp_hash_table_t *hash_table);

/* just for testing string and int hash */
void vcp_test_hash(void);

#endif __ERMI_VIDEO_HASH_H__
