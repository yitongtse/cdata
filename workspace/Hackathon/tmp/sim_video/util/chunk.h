/*
 *  Copyright (c) 2013 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __CHUNK_H__
#define __CHUNK_H__


typedef struct chunk_ {
    struct chunk_ *next;        // pointer to next chunk (or NULL for last)
    uint8 owner;                // owner (for synchronization)
    uint16 size;                // usable buffer size
    uint16 thres;               // threshold (to tell if it is at end of chunk)
    uint16 wr;                  // write index
} chunk_t;


static inline
void chunk_init (chunk_t *chnk, int size, int thres)
{
    assert(thres <= size);
    chnk->next = NULL;
    chnk->size = size;
    chnk->thres = thres;
    chnk->wr = 0;
}


static inline
void* chunk_get_next (chunk_t *chnk, int nbytes, boolean *last_flag)
{
    assert(nbytes <= chnk->thres);
    uint64 addr = ((uint64)(chnk + 1)) + chnk->wr;
    chnk->wr += nbytes;
    *last_flag = (chnk->size - chnk->wr < chnk->thres);
    return (void*)addr;
}


#endif  // __CHUNK_H__
/*
 *  Copyright (c) 2013 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __CHUNK_H__
#define __CHUNK_H__


typedef struct chunk_ {
    struct chunk_ *next;        // pointer to next chunk (or NULL for last)
    uint8 owner;                // owner (for synchronization)
    uint16 size;                // usable buffer size
    uint16 thres;               // threshold (to tell if it is at end of chunk)
    uint16 wr;                  // write index
} chunk_t;


static inline
void chunk_init (chunk_t *chnk, int size, int thres)
{
    assert(thres <= size);
    chnk->next = NULL;
    chnk->size = size;
    chnk->thres = thres;
    chnk->wr = 0;
}


static inline
void* chunk_get_next (chunk_t *chnk, int nbytes, boolean *last_flag)
{
    assert(nbytes <= chnk->thres);
    uint64 addr = ((uint64)(chnk + 1)) + chnk->wr;
    chnk->wr += nbytes;
    *last_flag = (chnk->size - chnk->wr < chnk->thres);
    return (void*)addr;
}


#endif  // __CHUNK_H__
/*
 *  Copyright (c) 2013 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __CHUNK_H__
#define __CHUNK_H__


typedef struct chunk_ {
    struct chunk_ *next;        // pointer to next chunk (or NULL for last)
    uint8 owner;                // owner (for synchronization)
    uint16 size;                // usable buffer size
    uint16 thres;               // threshold (to tell if it is at end of chunk)
    uint16 wr;                  // write index
} chunk_t;


static inline
void chunk_init (chunk_t *chnk, int size, int thres)
{
    assert(thres <= size);
    chnk->next = NULL;
    chnk->size = size;
    chnk->thres = thres;
    chnk->wr = 0;
}


static inline
void* chunk_get_next (chunk_t *chnk, int nbytes, boolean *last_flag)
{
    assert(nbytes <= chnk->thres);
    uint64 addr = ((uint64)(chnk + 1)) + chnk->wr;
    chnk->wr += nbytes;
    *last_flag = (chnk->size - chnk->wr < chnk->thres);
    return (void*)addr;
}


#endif  // __CHUNK_H__
/*
 *  Copyright (c) 2013 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __CHUNK_H__
#define __CHUNK_H__


typedef struct chunk_ {
    struct chunk_ *next;        // pointer to next chunk (or NULL for last)
    uint8 owner;                // owner (for synchronization)
    uint16 size;                // usable buffer size
    uint16 thres;               // threshold (to tell if it is at end of chunk)
    uint16 wr;                  // write index
} chunk_t;


static inline
void chunk_init (chunk_t *chnk, int size, int thres)
{
    assert(thres <= size);
    chnk->next = NULL;
    chnk->size = size;
    chnk->thres = thres;
    chnk->wr = 0;
}


static inline
void* chunk_get_next (chunk_t *chnk, int nbytes, boolean *last_flag)
{
    assert(nbytes <= chnk->thres);
    uint64 addr = ((uint64)(chnk + 1)) + chnk->wr;
    chnk->wr += nbytes;
    *last_flag = (chnk->size - chnk->wr < chnk->thres);
    return (void*)addr;
}


#endif  // __CHUNK_H__
