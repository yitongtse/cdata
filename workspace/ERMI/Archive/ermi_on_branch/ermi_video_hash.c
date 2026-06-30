/*
 *-----------------------------------------------------------------------------
 * ermi_video_hash.c - hash table APIs  (Utility File)
 *                   - Ported from vcp_hash.c
 *
 * January 2004, Xiaomei Liu
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *
 *
 *-----------------------------------------------------------------------------
 */
#include <string.h>
#include <os/list_private.h>
#include <assert.h>
#include "logger.h"

#include "ermi_video_hash.h"
#include "msg_ermi_c_vcp.c"

/*
 * vcp_create_hash_table() will create a hash table
 * return: pointer to hash table or NULL
 *
 * Input:
 *   hash_func,     function pointer that specifies how to get hash_value 
 *   lookup_func,   function pointer that checks if two objs are same
 *   id_func,       function pointer that returns id from object
 *   type,          enum that can be VCP_HASH_TYPE_INT or VCP_HASH_TYPE_STRING
 *   table_len,     int value < VCP_MAX_HASH_TABLE_LEN
 * Output:
 *   return         pointer to hash table if success
 *                  NULL if failed
 */
vcp_hash_table_t *vcp_create_hash_table (uint16 table_len, 
    vcp_hash_type_t type, vcp_hash_func_t hash_func, 
    vcp_hash_id_func_t id_func, list_lookup_func_t lookup_func)
    
{
    vcp_hash_table_t *table;
    list_header *list;
    uint16  n_entry;   

    /*validate parameter*/
    assert(table_len <= VCP_MAX_HASH_TABLE_LEN);
    assert(hash_func);
    assert(lookup_func);

    /*allocate mem*/
    table = malloc(sizeof(vcp_hash_table_t));
    if (!table) {
        errmsg(&msgsym(RSCERR, VCP), "VCP can't malloc hash table");
        return (NULL);
    }

    table->type = type;

    /*allocate space to store pointers of list_header*/
    table->entry = malloc(sizeof(list_header)*table_len);
    if (!table->entry) {
        errmsg(&msgsym(RSCERR, VCP), "VCP can't malloc hash entry");
        free(table);
        return (NULL);
    }

    /*initialization*/
    table->len = table_len; 
    table->hash_func = hash_func;
    table->lookup_func = lookup_func;
    if (!id_func) {
        table->id_func = vcp_default_id_func;
    } else {
        table->id_func = id_func;
    }
    table->count = 0;
   
    /* create list for each entry */
    for (n_entry = 0; n_entry < table_len; n_entry++) {
        list = list_create(&table->entry[n_entry], 0, NULL,
                           LIST_FLAGS_AUTOMATIC);
        if (!list) {
            errmsg(&msgsym(RSCERR, VCP), "VCP can't create hash list");
            vcp_destroy_hash_table(table);
            return (NULL);
        }

    }
    return (table);
}

/* vcp_destroy_hash_table destroy the empty lists , 
 *   and free hash table 
 * Input:
 *   table, pointer to hash table which has vcp_hash_table_t* type
 * Output:
 *   No output, always succeed
 * NOTE: 
 *   Table must be valid table and the entry must be 0 to be destroyed
 *   Otherwise, there will be assertion error
 */
void vcp_destroy_hash_table (vcp_hash_table_t *table)
{
    uint16 nslot;
    list_header *list;

    /*is table valid?*/
    assert(table);
    assert(table->entry);
        
    /* free the lists */
    nslot = table->len;
    while (nslot > 0) {
        list = &table->entry[nslot - 1];

        /* make sure the list is empty */
        assert(list->count == 0);
        list_destroy(list);
        nslot--;
    }
    free(table->entry);    
    free(table);
}

/* vcp_find_hash_obj() find the object with this id 
 *   id must be casted to void* before calling this func
 *
 * Input:
 *   table,     pointer to hash table of type vcp_hash_table_t*
 *   id,        void* pointer that point to the key of an object
 * Output:
 *   return     pointer to the hash object if success
 *              NULL if failed
 */
void *vcp_find_hash_obj (vcp_hash_table_t *table, void* id)
{
    uint16 hash_value;
    void *found_obj, *obj_id;
    list_header *list;
    list_element *runner;
   
    /* find hash entry first */
    hash_value = table->hash_func(table->len, table->type, id);
    if (hash_value >= table->len) {
        printf("Bad hash value, %s\n", __FUNCTION__);
        return (NULL);
    }
    list = &table->entry[hash_value];

    /* search through list for this object */
    FOR_ALL_ELEMENTS_IN_LIST(list, runner) {
        found_obj = (void*) runner->data;
        obj_id = table->id_func(found_obj);
        if ((table->lookup_func(obj_id, id)) == 0) {
            return (found_obj);
        }
    }

    return (NULL);
}


/*
 * vcp_add_hash_obj() add a hash object to hash table
 *   caller should check object existance before calling
 *
 * Input:
 *   table,     pointer to hash table with vcp_hash_table_t*
 *   obj,       pointer to hash object to be added to the table
 *
 * Output:
 *   return     TRUE if succeed
 *              FALSE if failed
 *
 * NOTE: It is the caller's responsibility to make sure that 
 *       hash object does not exist in hash table already
 */
boolean vcp_add_hash_obj (vcp_hash_table_t *table, void* obj)
{
    uint16 hash_value;
    list_header *list;
    boolean add_result;
    void* id;

    /* get the id first, and find an entry in hash table */
    id = table->id_func(obj);
    hash_value = table->hash_func(table->len, table->type, id);
    list = &table->entry[hash_value];

    /* assuming the object doesn't exist yet */
    add_result = list_enqueue(list, NULL, obj);
    table->count++;
    return (add_result);
}

/* vcp_del_hash_obj() delete an object from hash table 
 * Input:
 *   table,     pointer to hash table of type vcp_hash_table_t*
 *   obj,       pointer to hash object to be deleted
 * Output:
 *   return,    TRUE if succeed
 *              FALSE if failed
 */
boolean vcp_del_hash_obj (vcp_hash_table_t *table, void* obj)
{
    void *found_obj;
    uint16 hash_value;
    list_header *list;
    void* id;

    assert(table);

    /* find the object first */
    id = table->id_func(obj);
    found_obj = vcp_find_hash_obj(table, id);
    if (!found_obj) {
        return (FALSE);
    }

    /* remove the object from hash entry */
    hash_value = table->hash_func(table->len, table->type, id);
    list = &table->entry[hash_value];
    if (!list_remove(list, NULL, found_obj)) {
        return (FALSE);
    }

    table->count--;
    return (TRUE);
}

/* show_vcp_hash_table() display hash table content
 *   by displaying object id 
 *
 * Input:
 *   table,     pointer to valid hash table of vcp_hash_table_t* type
 * Output:
 *   display hash object and entry info 
 *
 * NOTE:
 *   This function is designed just for debugging purpose
 */
void show_vcp_hash_table (vcp_hash_table_t *table)
{
    list_header *list;
    list_element *runner;
    uint32 k;
    void* id;

    for (k=0; k<table->len; k++) {
        list = &table->entry[k];

        /* show id's of the non empty list */
        printf("Table entry %d\n", k);
        if (list->count > 0) {
            FOR_ALL_ELEMENTS_IN_LIST(list, runner) {
                id = table->id_func(runner->data);
                if (table->type == VCP_HASH_TYPE_STRING)
                    printf("obj %s", (char*)id);
                else if (table->type == VCP_HASH_TYPE_INT)
                    printf("obj %d", *((uint32*)id));
            }
            printf("\n");
        }
    }

}

/* vcp_string_hash_func() default string hash function
 *   map string to an uint16 hash value 
 *
 * Input:
 *   len,       hash table length < VCP_MAX_HASH_TABLE_LEN
 *   type,      enum that must be VCP_HASH_TYPE_STRING
 *   id,        pointer to the hash object key
 *
 * Output:
 *   return     16 bit integer < VCP_MAX_HASH_TABLE_LEN
 */
uint16 vcp_string_hash_func(uint16 len, vcp_hash_type_t type, void* id)
{
    uint16 id_len, i, ret;
    char* string_id;
    uint32 hash_value=0;

    assert(type == VCP_HASH_TYPE_STRING);
    string_id = (char*)id;
    id_len = strlen(string_id);

    /* calculate hash value */
    for (i=0; i<id_len; i=i+2) {
        hash_value += (string_id[i]<<8) + string_id[i+1];
    }

    /* handle the last char */
    if ((id_len & 1)) {
        hash_value += string_id[id_len-1];
    }
    ret = hash_value % len;
    return ret;
}

/* vcp_default_id_func() assumes the first
 *   element of the hash object is the id
 *
 * Input:
 *   obj,       pointer to valid hash object
 * Output:
 *   return     pointer to the object key
 *
 * NOTE:  To use this default id function, the first element
 *        of the hash object must be the key
 */
void* vcp_default_id_func(void* obj)
{
    return obj;
}

/* vcp_string_lookup_func() is the default 
 *   string id comparison function
 * 
 * Input:
 *   id1,   pointer to the first id string
 *   id2,   pointer to the second id string
 *
 * Output:
 *   return 0 if the two strings are same
 *          != 0 if two strings are different
 */
int vcp_string_lookup_func(void* id1, void* id2)
{
    return strcmp(id1, id2);
}

/* vcp_int_lookup_func() is the default
 *   integer id comparison function
 *
 * Input:
 *   id1,   pointer to the first id integer
 *   id2,   pointer to the second id integer
 *
 * Output:
 *   return 0 if the two strings are same
 *          != 0 if two strings are different
 */
int vcp_int_lookup_func(void* id1, void* id2)
{
    uint32 val1, val2, ret;
    val1 = *((uint32*)id1);
    val2 = *((uint32*)id2);
    ret = (val1==val2)?0:1;
    return ret;
}

/* vcp_int_hash_func() is the default integer id
 *   hash function. It maps an int to uint16 hash
 *   value
 *
 * Input:
 *   len,       hash table length < VCP_MAX_HASH_TABLE_LEN
 *   type,      enum that must be VCP_HASH_TYPE_INT 
 *   id,        pointer to the hash object key
 *
 * Output:
 *   return     16 bit integer < VCP_MAX_HASH_TABLE_LEN
 */
uint16 vcp_int_hash_func(uint16 len, vcp_hash_type_t type, void* id)
{
    uint32 id_val, hash_val;
    assert(type==VCP_HASH_TYPE_INT);
    
    id_val = *((uint32*)id);
    hash_val = (id_val & 0xffff) + (id_val >>16);

    return (hash_val % len);
}

/* dummy function for testing */
void* vcp_test_string_id(void* obj)
{
    char* string;
    string = (char*)obj;

    return (void*)(string+3);
}

/* test simple string hash and int hash */
void vcp_test_hash(void)
{
    char* strings[8] = {"string1", "string2", "string3", "string4", "string5",
                        "string6", "string7", "string8"};
    uint32 datas[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    vcp_hash_table_t *string_table, *int_table;
    uint32 i;

    string_table = vcp_create_hash_table(8, VCP_HASH_TYPE_STRING,
                                         vcp_string_hash_func,
                                         vcp_test_string_id,
                                         vcp_string_lookup_func);
    int_table = vcp_create_hash_table(8, VCP_HASH_TYPE_INT,
                                      vcp_int_hash_func,
                                      vcp_default_id_func,
                                      vcp_int_lookup_func);

    for (i=0; i<8; i++) {
        vcp_add_hash_obj(string_table, strings[i]);
        vcp_add_hash_obj(int_table, &datas[i]);
    }
    
    show_vcp_hash_table(string_table);
    show_vcp_hash_table(int_table);

    vcp_del_hash_obj(string_table, strings[1]);
    vcp_del_hash_obj(int_table, &datas[1]);

    show_vcp_hash_table(string_table);
    show_vcp_hash_table(int_table);
}
