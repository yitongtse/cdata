///  Copyright (c) 2011-2012, 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      video_ca.c
///  @brief     Video encryption support
///  @author    Yi Tong Tse

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <assert.h>
//#include <binos_svcs/services.h>
#include "common.h"
#include "video_def.h"
#include "video.h"
#include "crc.h"
#include "video_util.h"
#include "video_cli.h"
#include "video_ca.h"
#include "generic_cli.h"
#include "scs_messages.h"


// Predefined constants to specify descriptor location (for SID scrambling)
// Ugly workaround to temporarily prevent duplicate definition errors
#ifndef __DO_NOT_DEFINE_FRAGMENT_HEADER__
const unsigned long FRAGMENT_HEADER = 0xFFFFFFFE;
const unsigned long FRAGMENT_ALL = 0xFFFFFFFD;
#endif //  __DO_NOT_DEFINE_FRAGMENT_HEADER__

#define ECM_INDEX_PATH          "/ecm_info_table_ptr_indexes"
#define ECM_INFO_PATH           "/ecm_info_tables"
#define ECM_PKT_PATH            "/ecm_pkts"

#define VEMAN_PROC_NAME "LC-VEMAN"

typedef struct {
    uint32 ok_cnt;
    uint32 timeout_cnt;
} ipc_stat_t;

//ipc_stat_t scs_ipc_stat;

// Shared memory descriptors
int ecm_index_fd = -1;
int ecm_info_fd = -1;
int ecm_pkt_fd = -1;

uint64 *ecm_index_buf = MAP_FAILED;
uint8 *ecm_info_buf = MAP_FAILED;
uint8 *ecm_pkt_buf = MAP_FAILED;

extern void *pclc_cvmx_bootmem_alloc_named(uint64_t size, uint64_t alignment, const char *name);
extern void *pclc_cvmx_bootmem_alloc_named(uint64_t size, uint64_t alignment, const char *name);
extern void  pclc_cvmx_bootmem_free_named(void *ptr, const char *name);
extern void *pclc_cvmx_bootmem_find_named(uint64_t *size, const char *name);
extern void  pclc_cvmx_bootmem_print_named(void);


// Open shared memory for ECM info buffer
int video_ca_init_shm (void)
{
#if 0
    uint32 size;

    // Set up shared buffer for ECM indexes
    //
    if (ecm_index_fd == -1) {
        ecm_index_fd = shm_open(ECM_INDEX_PATH, O_RDONLY, 0666);
    }
    if (ecm_index_fd == -1) {
        VIDEO_DEBUG("Failed to open shared memory for ECM index\n");
        return errno;
    }
    size = lseek(ecm_index_fd, 0L, SEEK_END);
    lseek(ecm_index_fd, 0L, SEEK_SET);

    if (ecm_index_buf == MAP_FAILED) {
        ecm_index_buf = mmap(0, size, PROT_READ, MAP_SHARED, ecm_index_fd, 0);
    }
    if (ecm_index_buf == MAP_FAILED) {
        VIDEO_DEBUG("Failed to memory map ECM index\n");
        return errno;
    }
    printf("ECM index buffer %016x, size %u\n",
           (uintptr_t)ecm_index_buf, size);


    // Set up shared buffer for ECM infos
    //
    if (ecm_info_fd == -1) {
        ecm_info_fd = shm_open(ECM_INFO_PATH, O_RDWR, 0666);
    }
    if (ecm_info_fd == -1) {
        VIDEO_DEBUG("Failed to open shared memory for ECM info\n");
        return errno;
    }
    size = lseek(ecm_info_fd, 0L, SEEK_END);
    lseek(ecm_info_fd, 0L, SEEK_SET);

    if (ecm_info_buf == MAP_FAILED) {
        ecm_info_buf = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, ecm_info_fd, 0);
    }
    if (ecm_info_buf == MAP_FAILED) {
        VIDEO_DEBUG("Failed to memory map ECM info\n");
        return errno;
    }
    printf("ECM info buffer %016x, size %u\n",
           (uintptr_t)ecm_info_buf, size);


    // Set up shared (named) bootmem buffer for ECM packets
    //
    if (ecm_pkt_buf == MAP_FAILED) {

        const char *mem_name = "g_ecm_pkt_bootmem_buf";
        uint64_t    mem_size;

        ecm_pkt_buf = (char *) pclc_cvmx_bootmem_find_named(&mem_size, mem_name);

        if (ecm_pkt_buf == NULL) {
            mem_size = MAX_ECM_PKTS * MAX_MPEG_PKT_SIZE;
            ecm_pkt_buf = (char *) pclc_cvmx_bootmem_alloc_named(mem_size, 1, mem_name);

        } else {
            assert(mem_size);
            if (mem_size != MAX_ECM_PKTS * MAX_MPEG_PKT_SIZE) {
                pclc_cvmx_bootmem_free_named(ecm_pkt_buf, mem_name);

                mem_size = MAX_ECM_PKTS * MAX_MPEG_PKT_SIZE;
                ecm_pkt_buf = (char *) pclc_cvmx_bootmem_alloc_named(mem_size, 1, mem_name);

            } else {
                return EOK;
            }
        }
    }

    if (ecm_pkt_buf == NULL)
        ecm_pkt_buf = MAP_FAILED; // Make sure it's set to MAP_FAILED on failure

    if (ecm_pkt_buf == MAP_FAILED) {
        VIDEO_DEBUG("Failed to allocate boot memory for ECM packets\n");
        return -1;
    }
    memset(ecm_pkt_buf, 0, MAX_ECM_PKTS * MAX_MPEG_PKT_SIZE);
#endif

    return EOK;
}


// Encode ES stream type according SCS enum definition in en_stream_type_t
static
en_stream_type_t video_ca_get_stream_type(uint8 es_type)
{
    switch (es_type) {
    case STREAM_TYPE_VIDEO_MPEG1:
    case STREAM_TYPE_VIDEO_MPEG2:
    case STREAM_TYPE_VIDEO_H264:
    case STREAM_TYPE_VIDEO_UNKNOWN:
        return esVideoComponent;

    case STREAM_TYPE_AUDIO_MPEG1:
    case STREAM_TYPE_AUDIO_MPEG2:
    case STREAM_TYPE_AUDIO_AAC:
    case STREAM_TYPE_AUDIO_AC3:
        return esAudioComponent;

    default:
        // All other stream types are considered Data
        return esDataComponent;
    }
}

static
const char* video_ca_decode_stream_type (en_stream_type_t m_en_stream_type)
{
    switch (m_en_stream_type) {
    case esVideoComponent: return "Video";
    case esAudioComponent: return "Audio";
    case esDataComponent: return "Data";
    case esUnknownComponent: return "Unkown";
    default: break;
    }
    return "Unknown";
}


// Append CA descriptor to program info loop
static
int pmt_add_ca_desc_prog (pmt_info_t *pmt, ca_descriptor_header_t *desc,
                          psi_section_t *new_pmt_sect)
{
    uint8* in_ptr = (uint8*)pmt->sect->sect_hdr;
    uint8* out_ptr = (uint8*)new_pmt_sect->sect_hdr;
    pmt_header_t* pmt_hdr = (pmt_header_t*)in_ptr;
    int sect_len = pmt_get_section_size(pmt_hdr);
    int remain_len = sect_len - CRC_SIZE;       // without CRC

    // Sanity check
    if ((pmt->ca_desc_cnt >= MAX_CA_DESC_PER_PROG) ||
        (pmt_get_section_size(pmt_hdr) + sizeof(descriptor_header_t)
         + desc->len > MAX_PSI_SECT_SIZE)) {
        return E2BIG;
    }

    // Copy section header and program info loop
    int prog_info_len = pmt_get_prog_info_len(pmt_hdr);
    int len = PMT_SECT_HDR_SIZE + prog_info_len;
    memcpy(out_ptr, in_ptr, len);
    in_ptr += len;
    out_ptr += len;
    remain_len -= len;

    // Append CA descriptor
    len = sizeof(descriptor_header_t) + desc->len;
    memcpy(out_ptr, desc, len);
    out_ptr += len;

    // Adjust section length
    pmt_header_t* new_pmt_hdr = (pmt_header_t*)new_pmt_sect->sect_hdr;
    pmt_set_section_size(new_pmt_hdr, sect_len + len);
    pmt_set_prog_info_len(new_pmt_hdr, prog_info_len + len);

    // Copy the rest of the PMT section
    memcpy(out_ptr, in_ptr, remain_len);

    return EOK;
}


#define PRIV_DATA_IN_CA_DESCR     7

// Append CA descriptor to all ES in PMT
static
int pmt_add_ca_desc_all_es (pmt_info_t *pmt, ca_descriptor_header_t *desc,
                            psi_section_t *new_pmt_sect)
{
    uint8* in_ptr = (uint8*)pmt->sect->sect_hdr;
    uint8* out_ptr = (uint8*)new_pmt_sect->sect_hdr;
    pmt_header_t* pmt_hdr = (pmt_header_t*)in_ptr;
    int sect_len = pmt_get_section_size(pmt_hdr);
    uint8* esinfo_end_addr = in_ptr + sect_len - CRC_SIZE;
    int desc_len = sizeof(descriptor_header_t) + desc->len;
    int insert_cnt = 0;
    int len;
    int priv_data = (desc->len == PRIV_DATA_IN_CA_DESCR) ? 1:0;

    // Sanity check
    if (pmt->ca_desc_cnt + pmt->es_cnt > MAX_CA_DESC_PER_PROG
        || sect_len + (sizeof(descriptor_header_t) + desc_len) * pmt->es_cnt
           > MAX_PSI_SECT_SIZE) {
        return E2BIG;
    }

    // Copy section header and program info loop
    len = PMT_SECT_HDR_SIZE + pmt_get_prog_info_len(pmt_hdr);
    memcpy(out_ptr, in_ptr, len);
    in_ptr += len;
    out_ptr += len;

    // ES loop
    while (in_ptr < esinfo_end_addr) {
        pmt_es_info_t* es_info = (pmt_es_info_t*)in_ptr;

        // Copy the original ES info
        len = pmt_get_es_info_len(es_info) + sizeof(pmt_es_info_t);
        memcpy(out_ptr, in_ptr, len);

        in_ptr += len;

        if (video_ca_get_stream_type(es_info->es_type) == esDataComponent) {
            out_ptr += len;
            continue;
        }

        // Set the output ES info length
        pmt_set_es_info_len((pmt_es_info_t*)out_ptr,
                             len + desc_len - sizeof(pmt_es_info_t));

        // Append CA descriptor
        out_ptr += len;
        memcpy(out_ptr, desc, desc_len);
        out_ptr += desc_len;

        // Increment 3rd byte of CA descriptor private data
        if (priv_data) { 
            ((ext_ca_descr_header_t *)desc)->priv_data3++;
        }

        insert_cnt++;
    }

    // Adjust new PMT section size
    pmt_header_t* new_pmt_hdr = (pmt_header_t*)new_pmt_sect->sect_hdr;
    pmt_set_section_size(new_pmt_hdr, sect_len + desc_len * insert_cnt);


    return EOK;
}


// Append CA descriptor to specified ES in PMT
static
int pmt_add_ca_desc_es (pmt_info_t *pmt, uint16 pid,
                        ca_descriptor_header_t *desc,
                        psi_section_t *new_pmt_sect)
{
    uint8* in_ptr = (uint8*)pmt->sect->sect_hdr;
    uint8* out_ptr = (uint8*)new_pmt_sect->sect_hdr;
    pmt_header_t* pmt_hdr = (pmt_header_t*)out_ptr;
    pmt_es_info_t* es_info = NULL;
    int orig_es_len;
    int len = 0;
    int i;

    // Find ES with matching PID
    for (i=0; i<pmt->es_cnt; i++) {
        es_info = pmt_info_get_es(pmt, i);
        if (pmt_get_es_pid(es_info) == pid) {
            len = pmt->es[i].offset + sizeof(pmt_es_info_t)
                  + pmt_get_es_info_len(es_info);
            break;
        }
    }

    if (!len) {
        VIDEO_DEBUG("Pid %d not found in PMT\n", pid);
        return ENOENT;
    }

    // Sanity check
    if (pmt->ca_desc_cnt >= MAX_CA_DESC_PER_PROG ||
       (pmt_get_section_size(pmt_hdr) + sizeof(descriptor_header_t)
                                      + desc->len > MAX_PSI_SECT_SIZE)) {
        return E2BIG;
    }

    orig_es_len = pmt_get_es_info_len(es_info);
    es_info = (pmt_es_info_t*)(out_ptr + pmt->es[i].offset);

    // Copy everything before the insert point from old to new PMT sections
    memcpy(out_ptr, in_ptr, len);
    in_ptr += len;
    out_ptr += len;

    // Append CA descriptor
    len = sizeof(descriptor_header_t) + desc->len;
    memcpy(out_ptr, desc, len);
    out_ptr += len;
    pmt_set_es_info_len(es_info, orig_es_len + len);

    // Copy everything after the insert point from old to new PMT sections
    int sect_len = pmt_get_section_size(pmt_hdr);
    len = sect_len - CRC_SIZE - len;
    memcpy(out_ptr, in_ptr, len);

    // Adjust new PMT section size
    pmt_set_section_size(pmt_hdr,
                         sect_len + sizeof(descriptor_header_t) + desc->len);

    return EOK;
}


// CA Descriptor Insertion / Deletion Routines
//
#define CA_DESC_LOC_PROG        0
#define CA_DESC_LOC_ALL_ES      0xFFFF


// Insert CA descriptor
//   location is either:
//     - the specified PID inner loop,
//     - CA_DESC_LOC_PROG for program outer loop,
//     - CA_DESC_LOC_ALL_ES for all ES inner loop
static
int pmt_add_ca_desc (pmt_info_t *pmt, ca_descriptor_header_t *desc,
                     uint16 location)
{
    int rc;

    // Allocate new PSI section for the new PMT with CA descriptor inserted
    psi_section_t* new_pmt_sect = (psi_section_t*)pool_alloc(&psi_section_pool);
    if (!new_pmt_sect) {
        VIDEO_DEBUG("Failed to allocate PSI section\n");
        return ENOMEM;
    }

    switch (location) {

    case CA_DESC_LOC_PROG:
        rc = pmt_add_ca_desc_prog(pmt, desc, new_pmt_sect);
        break;

    case CA_DESC_LOC_ALL_ES:
        rc = pmt_add_ca_desc_all_es(pmt, desc, new_pmt_sect);
        break;

    default:
        if (location < NULL_PID) {
            rc = pmt_add_ca_desc_es(pmt, location, desc, new_pmt_sect);
        } else {
            VIDEO_DEBUG("Bad CA pid %d\n", location);
            rc = EINVAL;
        }
    }

    if (rc != EOK) {
        VIDEO_DEBUG("Failed to insert CA descriptor to PMT: %s\n",
                    strerror(rc));
        pool_free(&psi_section_pool, &new_pmt_sect->link);
        return rc;
    }

    pool_free(&psi_section_pool, &pmt->sect->link);

    parse_pmt(pmt, new_pmt_sect);

    return EOK;
}


// Lookup CA descriptor with specified System ID
static
boolean ca_desc_lookup_by_sys_id (pmt_info_t *pmt, uint16 begin_loc,
                                  uint16 end_loc, uint16 sys_id,
                                  int *ca_desc_idx)
{
    int i;
    boolean found = FALSE;

    for (i = *ca_desc_idx; i < pmt->ca_desc_cnt; i++) {
        if (pmt->ca_desc[i].offset < begin_loc) {
            continue;
        }

        if (pmt->ca_desc[i].offset >= end_loc) {
            break;
        }

        if (ca_desc_get_sys_id(pmt_info_get_ca_desc(pmt, i)) == sys_id) {
            found = TRUE;
            break;
        }
    }

    *ca_desc_idx = i;
    return found;
}


// Delete specified CA descriptor in program info loop
static
int pmt_delete_ca_desc_prog (pmt_info_t *pmt, ca_descriptor_header_t *desc,
                             psi_section_t *new_pmt_sect)
{
    int sys_id = ca_desc_get_sys_id(desc);
    uint8* in_ptr = (uint8*)pmt->sect->sect_hdr;
    uint8* out_ptr = (uint8*)new_pmt_sect->sect_hdr;
    pmt_header_t* pmt_hdr = (pmt_header_t*)in_ptr;
    int sect_len = pmt_get_section_size(pmt_hdr);
    uint32 remain_len = sect_len - CRC_SIZE;
    descriptor_header_t *fnd_desc;
    int len;
    int idx = 0;

    int prog_info_len = pmt_get_prog_info_len(pmt_hdr);
    if (!ca_desc_lookup_by_sys_id(pmt, 0, PMT_SECT_HDR_SIZE + prog_info_len,
                                  sys_id, &idx)) {
        return ENOENT;
    }

    // Copy everything before the matched CA descriptor
    len = pmt->ca_desc[idx].offset;
    memcpy(out_ptr, in_ptr, len);
    in_ptr += len;
    out_ptr += len;
    remain_len -= len;

    // Skip the found CA descriptor
    fnd_desc = (descriptor_header_t*)pmt_info_get_ca_desc(pmt, idx);
    len = sizeof(descriptor_header_t) + fnd_desc->len;
    in_ptr += len;
    remain_len -= len;

    // Adjust section length
    pmt_hdr = (pmt_header_t*)new_pmt_sect->sect_hdr;
    pmt_set_section_size(pmt_hdr, sect_len - len);
    pmt_set_prog_info_len(pmt_hdr, prog_info_len - len);

    // Copy the rest of the PMT section
    memcpy(out_ptr, in_ptr, remain_len);

    return EOK;
}


// Delete specified CA descriptor in all ES info loops
static
int pmt_delete_ca_desc_all_es (pmt_info_t *pmt, ca_descriptor_header_t *desc,
                               psi_section_t *new_pmt_sect)
{
    int match_cnt = 0;
    int sys_id = ca_desc_get_sys_id(desc);
    uint8* in_ptr = (uint8*)pmt->sect->sect_hdr;
    uint8* out_ptr = (uint8*)new_pmt_sect->sect_hdr;
    pmt_header_t* pmt_hdr = (pmt_header_t*)in_ptr;
    descriptor_header_t *fnd_desc;
    int len;
    int delete_len = 0;
    int i;
    int idx = 0;
    int last_offset = 0;

    // Loop through all ESs
    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt_info_get_es(pmt, i);
        int es_info_len = pmt_get_es_info_len(es_info);
        int begin_loc = pmt->es[i].offset;
        int end_loc = begin_loc + sizeof(pmt_es_info_t) + es_info_len;

        if (ca_desc_lookup_by_sys_id(pmt, begin_loc, end_loc, sys_id, &idx)) {
            match_cnt++;

            // Copy till the matched CA descriptor
            len = pmt->ca_desc[idx].offset - last_offset;
            memcpy(out_ptr, in_ptr, len);
            in_ptr += len;
            out_ptr += len;
            last_offset += len;

            // Skip the found CA descriptor
            fnd_desc = (descriptor_header_t*)pmt_info_get_ca_desc(pmt, idx);
            len = sizeof(descriptor_header_t) + fnd_desc->len;
            in_ptr += len;
            last_offset += len;

            // Adjust output ES info length
            es_info = (pmt_es_info_t*)(((uintptr_t)new_pmt_sect->sect_hdr)
                                       + begin_loc - delete_len);
            pmt_set_es_info_len(es_info, es_info_len - len);

            delete_len += len;
        }
    }

    // Copy the rest of the PMT
    len = pmt_get_section_size(pmt_hdr) - CRC_SIZE - last_offset;
    memcpy(out_ptr, in_ptr, len);
    out_ptr += len;

    // Adjust output PMT section length
    pmt_hdr = (pmt_header_t*)new_pmt_sect->sect_hdr;
    pmt_set_section_size(pmt_hdr,
                         out_ptr - (uint8*)new_pmt_sect->sect_hdr + CRC_SIZE);

    return EOK;
}


// Delete specified CA descriptor in ES info loop of specified PID
static
int pmt_delete_ca_desc_es (pmt_info_t *pmt, uint16 pid,
                           ca_descriptor_header_t *desc,
                           psi_section_t *new_pmt_sect)
{
    int sys_id = ca_desc_get_sys_id(desc);
    uint8* in_ptr = (uint8*)pmt->sect->sect_hdr;
    uint8* out_ptr = (uint8*)new_pmt_sect->sect_hdr;
    pmt_header_t* pmt_hdr = (pmt_header_t*)in_ptr;
    int sect_len = pmt_get_section_size(pmt_hdr);
    uint32 remain_len = sect_len - CRC_SIZE;
    descriptor_header_t *fnd_desc;
    pmt_es_info_t* es_info;
    int begin_loc, end_loc;
    int len;
    int i;
    int idx = 0;

    // Find ES with matching PID in PMT
    for (i=0; i<pmt->es_cnt; i++) {
        es_info = pmt_info_get_es(pmt, i);
        if (pmt_get_es_pid(es_info) == pid) {
            break;
        }
    }

    if (i >= pmt->es_cnt) {
        // PID not found
        return ENOENT;
    }

    begin_loc = pmt->es[i].offset;
    end_loc = begin_loc + sizeof(pmt_es_info_t) + pmt_get_es_info_len(es_info);

    if (!ca_desc_lookup_by_sys_id(pmt, begin_loc, end_loc, sys_id, &idx)) {
        // CA_DESC not found
        return ENOENT;
    }

    // Copy everything before the matched CA descriptor
    len = pmt->ca_desc[idx].offset;
    memcpy(out_ptr, in_ptr, len);
    in_ptr += len;
    out_ptr += len;
    remain_len -= len;

    // Skip the found CA descriptor
    fnd_desc = (descriptor_header_t*)pmt_info_get_ca_desc(pmt, idx);
    len = sizeof(descriptor_header_t) + fnd_desc->len;
    in_ptr += len;
    remain_len -= len;

    // Adjust output PMT section length
    pmt_hdr = (pmt_header_t*)new_pmt_sect->sect_hdr;
    pmt_set_section_size(pmt_hdr, sect_len - len);

    // Adjust output ES info length
    es_info = (pmt_es_info_t*)(((uintptr_t)pmt_hdr) + begin_loc);
    pmt_set_es_info_len(es_info, pmt_get_es_info_len(es_info) - len);

    // Copy the rest of the PMT section
    memcpy(out_ptr, in_ptr, remain_len);

    return EOK;
}


// Delete specified CA descriptor from PMT
static
int pmt_delete_ca_desc (pmt_info_t *pmt, ca_descriptor_header_t *desc,
                        uint16 location)
{
    int rc;

    // Allocate new PSI section for the new PMT with CA descriptor removed
    psi_section_t* new_pmt_sect = (psi_section_t*)pool_alloc(&psi_section_pool);
    if (!new_pmt_sect) {
        VIDEO_DEBUG("Failed to allocate PSI section\n");
        return ENOMEM;
    }

    switch (location) {

    case CA_DESC_LOC_PROG:
        rc = pmt_delete_ca_desc_prog(pmt, desc, new_pmt_sect);
        break;

    case CA_DESC_LOC_ALL_ES:
        rc = pmt_delete_ca_desc_all_es(pmt, desc, new_pmt_sect);
        break;

    default:
        if (location >= NULL_PID) {
            VIDEO_DEBUG("Bad CA pid %d\n", location);
            rc = EINVAL;
            goto error;
        }
        rc = pmt_delete_ca_desc_es(pmt, location, desc, new_pmt_sect);
    }

    if (rc != EOK) {
        VIDEO_DEBUG("Failed to remove CA descriptor to PMT: %s\n",
                    strerror(rc));
        goto error;
    }

    pool_free(&psi_section_pool, &pmt->sect->link);
    parse_pmt(pmt, new_pmt_sect);
    return EOK;

error:
    pool_free(&psi_section_pool, &new_pmt_sect->link);
    return rc;
}


// Build pid_list from PMT (used in UPDATE_SERVICE and SID_EXISTS)
static
void video_ca_make_pid_list (pmt_info_t *pmt, uint16 *num_pids,
                             es_pid_t *pid_list)
{
    int i;
    *num_pids = pmt->es_cnt;
    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt_info_get_es(pmt, i);
        pid_list[i].pid = pmt_get_es_pid(es_info);
        pid_list[i].m_en_stream_type
                = video_ca_get_stream_type(es_info->es_type);
    }
}


// CA-Related Messages Senders and Handlers
//


// Send MSG_TYPE_NEW_TS message
static
int video_ca_send_add_ts (uint16 qam_id)
{
#if 0
    msg_new_onid_tsid_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    qam_config_t* cfg = get_qam_config(video_ctx, qam_id);
    VIDEO_CA_DEBUG("TX NEW_TS: qam %d, onid %d, tsid %d",
                   qam_id, cfg->onid, cfg->tsid);

    memset(&msg, 0, sizeof(msg));
    msg.qam_id = qam_id;
    msg.onid = cfg->onid;
    msg.tsid = cfg->tsid;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_NEW_TS, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "NEW_TS",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif
    return EOK;
}

// Send MSG_TYPE_ONID_TSID_CHANGE message
static
int video_ca_send_change_ts (uint16 qam_id, uint16 old_onid, uint16 old_tsid)
{
#if 0
    msg_change_on_onid_tsid_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    qam_config_t* new_cfg = get_qam_config(video_ctx, qam_id);
    VIDEO_CA_DEBUG("TX CHANGE_TS: qam %d, onid %d -> %d, tsid %d -> %d",
                   qam_id, old_onid, new_cfg->onid, old_tsid, new_cfg->tsid);

    memset(&msg, 0, sizeof(msg));
    msg.qam_id = qam_id;
    msg.old_onid = old_onid;
    msg.new_onid = new_cfg->onid;
    msg.old_tsid = old_tsid;
    msg.new_tsid = new_cfg->tsid;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_ONID_TSID_CHANGE, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "ONID_TSID_CHANGE",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif
    return EOK;
}

// Send MSG_TYPE_REMOVE_TS message
static
int video_ca_send_delete_ts (uint16 qam_id)
{
#if 0
    msg_remove_onid_tsid_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    qam_config_t* cfg = get_qam_config(video_ctx, qam_id);
    VIDEO_CA_DEBUG("TX REMOVE_TS: qam %d, onid %d, tsid %d",
                   qam_id, cfg->onid, cfg->tsid);

    memset(&msg, 0, sizeof(msg));
    msg.qam_id = qam_id;
    msg.onid = cfg->onid;
    msg.tsid = cfg->tsid;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_REMOVE_TS, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "REMOVE_TS",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif
    return EOK;
}

// Send MSG_TYPE_UPDATE_SERVICE message
int video_ca_send_update_service (uint16 qam_id, uint16 prog_num)
{
#if 0
    msg_update_service_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;
    int i;

    qam_prog_info_t* info = &get_qam_info(qam_id)->prog_tab[prog_num];
    VIDEO_CA_DEBUG("TX UPDATE_SID: qam %d, sid %d", qam_id, prog_num);

    memset(&msg, 0, sizeof(msg));
    msg.qam_id = qam_id;
    msg.sid = prog_num;

    if (info->used) {
        video_ca_make_pid_list(&get_out_prog(info->out_prog_id)->pmt,
                               &msg.num_pids, msg.pid_list);
    } else {
        msg.num_pids = 0;
    }

    for (i=0; i<msg.num_pids; i++) {
        VIDEO_CA_DEBUG("    pid %d, type %s", msg.pid_list[i].pid,
                video_ca_decode_stream_type(msg.pid_list[i].m_en_stream_type));
    }

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_UPDATE_SERVICE, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "UPDATE_SERVICE",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif

    return EOK;
}

// Send MSG_TYPE_NEW_PID message
int video_ca_send_add_pid (uint16 qam_id, uint16 pid)
{
#if 0
    msg_new_pid_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    VIDEO_CA_DEBUG("TX ADD_PID: qam %d, pid %d", qam_id, pid);


    memset(&msg, 0, sizeof(msg));
    msg.qam_id = qam_id;
    msg.pid = pid;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_NEW_PID, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "NEW_PID",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif
    return EOK;
}

// Send MSG_TYPE_PID_GONE message
int video_ca_send_delete_pid (uint16 qam_id, uint16 pid)
{
#if 0
    msg_pid_gone_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    VIDEO_CA_DEBUG("TX PID_GONE: qam %d, pid %d", qam_id, pid);

    memset(&msg, 0, sizeof(msg));
    msg.qam_id = qam_id;
    msg.pid = pid;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_PID_GONE, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "PID_GONE",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif

    return EOK;
}

// Send MSG_TYPE_SID_EXISTS message
int video_ca_send_sid_exists (uint16 qam_id, uint16 prog_num, uint64 p_object)
{
#if 0
    msg_sid_exists_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;
    int i;

    VIDEO_CA_DEBUG("TX SID_EXIST: sid %d, p_obj %016lx", prog_num, p_object);

    memset(&msg, 0, sizeof(msg));
    msg.sid = prog_num;
    msg.p_object = p_object;

    qam_prog_info_t* info = &get_qam_info(qam_id)->prog_tab[prog_num];
    video_ca_make_pid_list(&get_out_prog(info->out_prog_id)->pmt,
                           &msg.num_pids, msg.pid_list);

    for (i=0; i<msg.num_pids; i++) {
        VIDEO_CA_DEBUG("    pid %d, type %s", msg.pid_list[i].pid,
                video_ca_decode_stream_type(msg.pid_list[i].m_en_stream_type));
    }

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_SID_EXISTS, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "SID_EXISTS",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif

    return EOK;
}

// Send MSG_TYPE_NO_SID message
int video_ca_send_no_sid (uint16 qam_id __UNUSED, uint16 prog_num, uint64 p_object)
{
#if 0
    msg_no_sid_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    VIDEO_CA_DEBUG("TX NO_SID: sid %d, p_obj %016lx", prog_num, p_object);

    memset(&msg, 0, sizeof(msg));
    msg.sid = prog_num;
    msg.p_object = p_object;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_NO_SID, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "NO_SID",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif

    return EOK;
}

// Send MSG_TYPE_PID_EXISTS message
int video_ca_send_pid_exists (uint16 qam_id __UNUSED, uint16 pid, uint64 p_object)
{
#if 0
    msg_pid_exists_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    VIDEO_CA_DEBUG("TX PID_EXIST: pid %d, p_obj %016lx", pid, p_object);

    memset(&msg, 0, sizeof(msg));
    msg.pid = pid;
    msg.p_object = p_object;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_PID_EXISTS, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "PID_EXISTS",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif

    return EOK;
}

// Send MSG_TYPE_NO_PID message
int video_ca_send_no_pid (uint16 qam_id __UNUSED, uint16 pid, uint64 p_object)
{
#if 0
    msg_no_pid_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    VIDEO_CA_DEBUG("TX NO_PID: pid %d, p_obj %016lx", pid, p_object);

    memset(&msg, 0, sizeof(msg));
    msg.pid = pid;
    msg.p_object = p_object;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_NO_PID, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "NO_PID",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif

    return EOK;
}

// Send MSG_TYPE_PID_PROVISIONED message
int video_ca_send_pid_provisioned (uint16 qam_id, uint16 pid, uint64 p_object)
{
#if 0
    msg_pid_provisioned_t msg;
    vdman_ipc_rc_t rc;
    vipc_rc_t ipc_rc;

    VIDEO_CA_DEBUG("TX PID_PROV: qam %d, pid %d, p_obj %016llx",
                   qam_id, pid, p_object);

    memset(&msg, 0, sizeof(msg));
    msg.qam_id = qam_id;
    msg.pid = pid;
    msg.p_object = p_object;

    rc = video_ca_ipc_send_to_scs(MSG_TYPE_PID_PROVISIONED, &msg, &ipc_rc);
    if (rc != VDMAN_IPC_OK) {
        em_vidman_IPC_SEND_FAILED(VEMAN_PROC_NAME, "PID_PROVISIONED",
                                  vdman_ipc_rc_string(rc),
                                  vipc_rc_string(ipc_rc));
    }
#endif
    return EOK;
}

// MSG_TYPE_SET_CW_INDEX handler
void video_ca_set_cw_index_handler (ipc_message *ipc_msg)
{
    msg_scs_cw_index_t* msg = IPC_DATA(ipc_msg);

    VIDEO_CA_DEBUG("RX SET_CWI: qam %d, pid %d, cwi %d",
                   msg->qam_id, msg->pid, msg->p_cw);

    // TODO: Sanity checks needed
    get_qam_info(msg->qam_id)->pid_tab[msg->pid].cw_idx = msg->p_cw;
}


// MSG_TYPE_START_ECM_PLAYOUT handler
void video_ca_add_ecm_handler (ipc_message *ipc_msg)
{
    msg_start_ecm_playout_t* msg = IPC_DATA(ipc_msg);
    crsl_insert_t *insert;
    que_elem_t* insert_list;
    uint32 ecm_info_index;

    VIDEO_CA_DEBUG("RX ADD_ECM: qam %d, ecm_idx %d",
                   msg->qam_id, msg->ecm_info_index);

    // Allocate carousel insert
    insert = (crsl_insert_t*)pool_alloc(&video_ctx->crsl_insert_pool);
    if (!insert) {
        VIDEO_DEBUG("Run out of carousel insert\n");
        em_vidman_VIDEO_CAROUSEL_ALLOC_FAILED("ADD_ECM", msg->ecm_info_index);
        return;
    }

    // Set up carousel insert
    insert->ecm_flag = 1;
    ecm_info_index = msg->ecm_info_index;
    insert->crsl = (void*)((uintptr_t)ecm_info_index);
    insert->next_time = sys_clk.base;
    insert->target_cnt = CRSL_INSERT_CONTINUOUS;
    insert->period = ECM_PERIOD_DEFAULT;
#if 0
    VIDEO_CA_DEBUG("ecm_addr %08x",
                   (uint32)((ecm_info_t*)insert->crsl)->ecm_addr);
    VIDEO_CA_DEBUG("period %d", insert->period);
#endif

    // Put crsl insert to qam's queue
    qam_channel_lock(msg->qam_id);
    insert_list = get_qam_crsl_insert_list(video_ctx, msg->qam_id);
    que_put(NULL, insert_list, &insert->link);
    qam_channel_unlock(msg->qam_id);
}


// Search for ECM insert with matching ecm_info_index
static
crsl_insert_t* ecm_insert_lookup (que_elem_t *insert_list,
                                  uint32 ecm_info_index)
{
    crsl_insert_t *insert;

    FOR_ALL_ELEMENTS_IN_QUE(insert_list, insert) {
        if (insert->ecm_flag && (uint32)((uintptr_t)insert->crsl) == ecm_info_index) {
            return insert;
        }
    }
    return NULL;
}


// MSG_TYPE_REMOVE_ECM_PLAYOUT handler
void video_ca_delete_ecm_handler (ipc_message *ipc_msg)
{
    msg_start_ecm_playout_t* msg = IPC_DATA(ipc_msg);
    crsl_insert_t *insert;

    VIDEO_CA_DEBUG("RX DELETE_ECM: qam %d, ecm_idx %d",
                   msg->qam_id, msg->ecm_info_index);

    qam_channel_lock(msg->qam_id);
    insert = ecm_insert_lookup(get_qam_crsl_insert_list(video_ctx, msg->qam_id),
                               msg->ecm_info_index);
    if (!insert) {
        VIDEO_DEBUG("ECM with index %d not found\n", msg->ecm_info_index);
        em_vidman_VIDEO_CAROUSEL_NOT_FOUND("DELETE_ECM", msg->ecm_info_index);
    } else {
        que_deque(NULL, &insert->link);
        pool_free(&video_ctx->crsl_insert_pool, &insert->link);
    }
    qam_channel_unlock(msg->qam_id);
}


// MSG_TYPE_ADD_CA_DESCRIPTOR handler
//
//   CA-desc-mode               TableIdExt              Location
//   ------------------------------------------------------------------
//   CA_DESC_LOC_PROG           SID                     FRAGMENT_HEADER
//   CA_DESC_LOG_ALL_ES         SID                     FRAGMENT_ALL
//   PID-mode                   FRAGMENT_HEADER         PID
//
void video_ca_add_ca_descriptor_handler (ipc_message *ipc_msg)
{
    out_prog_t *prog;
    psi_section_t *pmt_sect;
    pmt_header_t *pmt_hdr;
    msg_add_ca_descriptor_t* msg = IPC_DATA(ipc_msg);
    ca_descriptor_header_t *desc = (ca_descriptor_header_t*) 
                                       msg->descriptor.m_pDescriptor; 
    ext_ca_descr_header_t *ext_desc = (ext_ca_descr_header_t *)desc;

    qam_info_t* qam = get_qam_info(msg->qam_id);
    uint16 ecm_pid = ca_desc_get_pid(desc);
    qam_pid_info_t *ecm_pid_info = &qam->pid_tab[ecm_pid];

    VIDEO_CA_DEBUG("RX ADD_CA_DSC: qam %d, tbl_id %d, location %d, size %d",
                   msg->qam_id, msg->descriptor.m_ulTableIdExt,
                   msg->descriptor.m_ulLocation, msg->descriptor.descr_size);
    VIDEO_CA_DEBUG("   tag %d, len %d, sysid %x, pid %x, priv %d,%d,%d\n",
                   desc->tag, desc->len, ca_desc_get_sys_id(desc), ecm_pid,
                   ext_desc->priv_data1, ext_desc->priv_data2,
                   ext_desc->priv_data3);

    if (!ecm_pid_info->used &&
         ecm_pid_info->out_ses_id!=MAX_SESSIONS) {
        VIDEO_DEBUG("ECM pid %d not allocated, used(%d) qam %d out_ses_id(%d)\n", 
                    ecm_pid, ecm_pid_info->used, msg->qam_id, ecm_pid_info->out_ses_id);
        return;
    }

    if (msg->descriptor.m_ulTableIdExt == FRAGMENT_HEADER) {
        // Insert CA desc to specified PID
        // Note: this does not work for PID shared among differnet programs!

        // Experimental (need sanity check)
        uint16 pid = (uint16)msg->descriptor.m_ulLocation;
        qam_pid_info_t *pid_info = &qam->pid_tab[pid];
        out_session_t *ses = get_out_session(pid_info->out_ses_id);

        prog = out_prog_lookup_by_pid(&ses->prog_list, pid);
        if (!prog->pmt_built) {
            VIDEO_DEBUG("Add CA-DSC: Qam %d Prog %d PMT not built",
                        msg->qam_id, prog->prog_num);
            return;
        }

        // Check and set out_ses_id for ECM pid
        uint16 out_ses_id = qam->pid_tab[pid].out_ses_id;
        if (ecm_pid_info->out_ses_id != MAX_SESSIONS &&
            ecm_pid_info->out_ses_id != out_ses_id) {
            VIDEO_DEBUG("PID %d already used by out session %d\n",
                        ecm_pid, ecm_pid_info->out_ses_id);
        } else {
            ecm_pid_info->out_ses_id = qam->pid_tab[pid].out_ses_id;
            pmt_add_ca_desc(&prog->pmt, desc, pid);
            prog->ca_desc_data = *desc;
            prog->ca_desc_loc = pid;
            VIDEO_CA_DEBUG("\n%s %d: prog_num %d desc:sysid %d desc:ecm_pid %d pid %d\n",
                        __FUNCTION__, __LINE__, prog->prog_num, ca_desc_get_sys_id(desc), 
                        ecm_pid, pid);
        }

    } else {
        uint16 prog_num = (uint16)msg->descriptor.m_ulTableIdExt;
        qam_prog_info_t* info = &get_qam_info(msg->qam_id)->prog_tab[prog_num];
        if (!info->used) {
            VIDEO_DEBUG("Add CA-DSC: Program %d not found in QAM %d",
                        prog_num, msg->qam_id);
            return;
        }

        prog = get_out_prog(info->out_prog_id);
        if (!prog->pmt_built) {
            VIDEO_DEBUG("Add CA-DSC: Qam %d Prog %d PMT not built",
                        msg->qam_id, prog->prog_num);
            return;
        }

        // Check and set out_ses_id for ECM pid
        if (ecm_pid_info->out_ses_id != MAX_SESSIONS) {
            VIDEO_DEBUG("PID %d already used by out session %d\n",
                        ecm_pid, ecm_pid_info->out_ses_id);
        } else {
            ecm_pid_info->out_ses_id = prog->out_ses_id;
            uint16 loc = msg->descriptor.m_ulLocation == FRAGMENT_HEADER ?
                             CA_DESC_LOC_PROG : CA_DESC_LOC_ALL_ES;
            pmt_add_ca_desc(&prog->pmt, desc, loc);
            prog->ca_desc_data = *desc;
            prog->ca_desc_loc = loc;
            VIDEO_CA_DEBUG("\n%s %d: prog_num %d desc:sysid %d desc:ecm_pid %d loc %d\n",
                        __FUNCTION__, __LINE__, prog->prog_num, ca_desc_get_sys_id(desc), 
                        ecm_pid, loc);
        }
    }

    // CA block check
    if (prog->in_prog->pmt.ca_desc_cnt > 0) {
        // block the output prog
        prog->blocked = 1;
        VIDEO_CA_DEBUG("Add CA-DSC: Blocked Input Program %d, CA desc already present, QAM %d",
                  prog->in_prog->prog_num, msg->qam_id);
        em_vidman_VIDEO_PROG_CA_BLOCKED("CA_BLOCKED", prog->in_prog->prog_num, msg->qam_id);
        qam->pat_rebuild = 1;
        return;
    }

    // Update PMT version
    pmt_sect = (psi_section_t*)prog->pmt.sect;
    pmt_hdr = (pmt_header_t*)pmt_sect->sect_hdr;
    prog->pmt_ver = (prog->pmt_ver + 1) % 32;
    pmt_hdr->ver = prog->pmt_ver;

    // Compute CRC
    pmt_sect->crc = crc32((uint8*)pmt_sect->sect_hdr,
                          pmt_get_section_size(pmt_hdr) - CRC_SIZE);
    psi_section_set_crc(pmt_sect, pmt_sect->crc);

    // Free TP buffers used by PMT
    pool_free_list(&psi_tp_buf_pool, &prog->pmt_crsl.tp_list);

    // Generate PMT carousel
    prog->pmt_crsl.num_tp = psi_section_packetize(prog->pmt.sect, prog->pmt_pid,
                                                  &prog->pmt_crsl.tp_list);

    // Mark ca_pid as used in the QAM
    ecm_pid_info->used = 1; 
    if (is_PME_session(prog)) {
        VIDEO_DEBUG("Add C0 to PMT \n");
        add_C0_table_to_pmt_carousel(prog, prog->pmt_pid);
    }
}


// MSG_TYPE_DELETE_CA_DESCRIPTOR handler
void video_ca_delete_ca_descriptor_handler (ipc_message *ipc_msg)
{
    out_prog_t *prog;
    psi_section_t *pmt_sect;
    pmt_header_t *pmt_hdr;
    msg_remove_ca_descriptor_t* msg = IPC_DATA(ipc_msg);
    ca_descriptor_header_t* desc
            = (ca_descriptor_header_t*)msg->descriptor.m_pDescriptor;

    VIDEO_CA_DEBUG("RX DELETE_CA_DSC: qam %d, tbl_id %d, location %d, size %d",
                   msg->qam_id, msg->descriptor.m_ulTableIdExt,
                   msg->descriptor.m_ulLocation, msg->descriptor.descr_size);

    if (msg->descriptor.m_ulTableIdExt == FRAGMENT_HEADER) {
        // Delete CA desc from specified PID
        // Note: this does not work for PID shared among differnet programs!

        // Experimental (need sanity check)
        qam_info_t* qam = get_qam_info(msg->qam_id);
        uint16 pid = (uint16)msg->descriptor.m_ulLocation;
        qam_pid_info_t* pid_info = &qam->pid_tab[pid];
        out_session_t* ses = get_out_session(pid_info->out_ses_id);
        prog = out_prog_lookup_by_pid(&ses->prog_list, pid);
        if (!prog->pmt_built) {
            VIDEO_DEBUG("Delete CA-DSC: Qam %d Prog %d PMT not built",
                        msg->qam_id, prog->prog_num);
            return;
        }

        pmt_delete_ca_desc(&prog->pmt, desc, pid);

        // clear ca-desc store
        ca_desc_set_sys_id (&prog->ca_desc_data, 0);
        prog->ca_desc_loc = 0;
    } else {
        uint16 prog_num = (uint16)msg->descriptor.m_ulTableIdExt;
        qam_prog_info_t* info = &get_qam_info(msg->qam_id)->prog_tab[prog_num];

        if (!info->used) {
            VIDEO_LOG("Delete CA descriptor: Program %d not found in QAM %d",
                      prog_num, msg->qam_id);
            return;
        }

        prog = get_out_prog(info->out_prog_id);
        if (!prog->pmt_built) {
            VIDEO_DEBUG("Delete CA-DSC: Qam %d Prog %d PMT not built",
                        msg->qam_id, prog->prog_num);
            return;
        }

        // Delete CA desc at program loop
        pmt_delete_ca_desc(&prog->pmt, desc,
                (msg->descriptor.m_ulLocation == FRAGMENT_HEADER)?
                CA_DESC_LOC_PROG : CA_DESC_LOC_ALL_ES);

        // clear ca-desc store
        ca_desc_set_sys_id (&prog->ca_desc_data, 0);
        prog->ca_desc_loc = 0;
    }

    // Update PMT version
    pmt_sect = (psi_section_t*)prog->pmt.sect;
    pmt_hdr = (pmt_header_t*)pmt_sect->sect_hdr;
    prog->pmt_ver = (prog->pmt_ver + 1) % 32;
    pmt_hdr->ver = prog->pmt_ver;

    // Compute CRC
    pmt_sect->crc = crc32((uint8*)pmt_sect->sect_hdr,
                          pmt_get_section_size(pmt_hdr) - CRC_SIZE);
    psi_section_set_crc(pmt_sect, pmt_sect->crc);

    // Free TP buffers used by PMT
    pool_free_list(&psi_tp_buf_pool, &prog->pmt_crsl.tp_list);

    // Generate PMT carousel
    prog->pmt_crsl.num_tp = psi_section_packetize(prog->pmt.sect, prog->pmt_pid,
                                                  &prog->pmt_crsl.tp_list);

    // TODO: Mark ca_pid as used in the QAM
}


// MSG_TYPE_GET_SID handler
void video_ca_get_sid_handler (ipc_message *ipc_msg)
{
    msg_get_sid_t* msg = IPC_DATA(ipc_msg);
    VIDEO_CA_DEBUG("RX GET_SID: qam %d, sid %d, p_obj %016llx",
                   msg->qam_id, msg->sid, msg->p_object);

    if (get_qam_info(msg->qam_id)->prog_tab[msg->sid].used) {
        video_ca_send_sid_exists(msg->qam_id, msg->sid, msg->p_object);
    } else {
        video_ca_send_no_sid(msg->qam_id, msg->sid, msg->p_object);
    }
}


// MSG_TYPE_GET_PID handler
void video_ca_get_pid_handler (ipc_message *ipc_msg)
{
    msg_get_pid_t* msg = IPC_DATA(ipc_msg);
    VIDEO_CA_DEBUG("RX GET_PID: qam %d, pid %d, p_obj %016llx",
                   msg->qam_id, msg->pid, msg->p_object);

    if (get_qam_info(msg->qam_id)->pid_tab[msg->pid].used) {
        video_ca_send_pid_exists(msg->qam_id, msg->pid, msg->p_object);
    } else {
        video_ca_send_no_pid(msg->qam_id, msg->pid, msg->p_object);
    }
}


#define MIN_REQUEST_PID         8048
#define MAX_REQUEST_PID         8175

// MSG_TYPE_REQUEST_PID handler
void video_ca_request_pid_handler (ipc_message *ipc_msg)
{
    msg_request_pid_t* msg = IPC_DATA(ipc_msg);
    VIDEO_CA_DEBUG("RX REQUEST_PID: qam %d, p_obj %016llx",
                   msg->qam_id, msg->p_object);

    uint16 pid = qam_find_next_avail_pid(MIN_REQUEST_PID, MAX_REQUEST_PID,
                                         msg->qam_id);
    assert(pid != INVALID_PID);

    qam_pid_info_set(&get_qam_info(msg->qam_id)->pid_tab[pid], 1, 0,
                     MAX_SESSIONS, NO_CHANGE);
    video_ca_send_pid_provisioned(msg->qam_id, pid, msg->p_object);
}


// MSG_TYPE_RESERVE_PID handler
void video_ca_reserve_pid_handler (ipc_message *ipc_msg)
{
    msg_reserve_pid_t* msg = IPC_DATA(ipc_msg);
    uint16 pid = msg->pid;
    VIDEO_CA_DEBUG("RX RESERVE_PID: qam %d, pid %d, p_obj %016llx",
                   msg->qam_id, pid, msg->p_object);
 
    if (pid >= NUM_PIDS) {
        VIDEO_CA_DEBUG("Error: invalid PID %d", pid);
        pid = INVALID_PID;
        goto done;
    }
 
    // VDMAN is passive entity so that it does not check the PID range.
    // VEMAN is responsible to set an availabe PID that is not conflict
    // with the PID reserved by VCMAN
    qam_pid_info_t* info = &get_qam_info(msg->qam_id)->pid_tab[pid];
    if (!info->used) {
        qam_pid_info_set(info, 1, 0, MAX_SESSIONS, 0);
    } else {
        pid = INVALID_PID;
    }

done:
    video_ca_send_pid_provisioned(msg->qam_id, pid, msg->p_object);
}


// MSG_TYPE_RELEASE_PID handler
void video_ca_release_pid_handler (ipc_message *ipc_msg)
{
    msg_release_pid_t* msg = IPC_DATA(ipc_msg);
    VIDEO_CA_DEBUG("RX RELEASE_PID: qam %d, pid %d", msg->qam_id, msg->pid);

    // VDMAN is passive entity so that it does check the PID range.
    // VEMAN is responsible to release the PID properly.
    qam_pid_info_set(&get_qam_info(msg->qam_id)->pid_tab[msg->pid], 0, 0, 0, 0);
}


// MSG_TYPE_SID_ECM_PID handler
void video_ca_sid_ecm_pid_handler (ipc_message *ipc_msg)
{
    uint16 pid = INVALID_PID;
    msg_sid_ecm_pid_t* msg = IPC_DATA(ipc_msg);
    VIDEO_CA_DEBUG("RX SID_ECM_PID: qam %d, sid %d, p_obj %016llx",
                   msg->qam_id, msg->sid, msg->p_object);

    qam_info_t* qam = get_qam_info(msg->qam_id);
    qam_prog_info_t* qam_prog = &qam->prog_tab[msg->sid];
    if (qam_prog->used) {
        out_prog_t* prog = get_out_prog(qam_prog->out_prog_id);
        out_config_t* cfg = get_out_session_config(video_ctx, prog->out_ses_id);
        video_pid_range_t* pid_range = &cfg->pid_range[prog->cfg_idx];
        pid = qam_find_prev_avail_pid(pid_range->last, pid_range->first, 
                                      msg->qam_id);
    }

    if (pid != INVALID_PID) {
       qam_pid_info_set(&qam->pid_tab[pid], 1, 0, MAX_SESSIONS, NO_CHANGE);
    }
    video_ca_send_pid_provisioned(msg->qam_id, pid, msg->p_object);
}


// MSG_TYPE_SET_PID_SCRAMBLED handler
static
void video_ca_set_pid_scrambled_handler(ipc_message *ipc_msg)
{
    msg_set_pid_scrambled_t* msg = IPC_DATA(ipc_msg);
    VIDEO_CA_DEBUG("RX SET_PID_SCRAMBLED: qam %d, pid %d, scrambled %d",
                   msg->qam_id, msg->pid, msg->scrambled);
 
    if (msg->pid != INVALID_PID) {
        qam_pid_info_t* info = &get_qam_info(msg->qam_id)->pid_tab[msg->pid];
        if (info->used) {
            // Pid scrambling status from SCS to VIOP necessary
            // for user display.
            info->scrambled = msg->scrambled;
        }
    }
}


// Handler for Encryption related messages
//   Note: this is only for testing purpose using ng_cli/sim_cli!!
//
boolean video_ca_msg_handler (ipc_message *ipc_msg,
                              ipc_message *rsp_msg __UNUSED, boolean *rsp_flag)
{
    *rsp_flag = FALSE;

    switch (IPC_HEADER(ipc_msg)->type) {

    case MSG_TYPE_VIDEO_CA_SET_CW_INDEX:
        video_ca_set_cw_index_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_START_ECM_PLAYOUT:
        video_ca_add_ecm_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_REMOVE_ECM_PLAYOUT:
        video_ca_delete_ecm_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_ADD_CA_DESCRIPTOR:
        video_ca_add_ca_descriptor_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_REMOVE_CA_DESCRIPTOR:
        video_ca_delete_ca_descriptor_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_GET_SID:
        video_ca_get_sid_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_GET_PID:
        video_ca_get_pid_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_REQUEST_PID:
        video_ca_request_pid_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_RESERVE_PID:
        video_ca_reserve_pid_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_RELEASE_PID:
        video_ca_release_pid_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_SID_ECM_PID:
        video_ca_sid_ecm_pid_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CA_SET_PID_SCRAMBLED:
        video_ca_set_pid_scrambled_handler(ipc_msg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


void video_ca_report_qam_change (uint16 qam_id, boolean old_mode,
                                 uint16 old_onid, uint16 old_tsid)
{
    qam_config_t *new_cfg = get_qam_config(video_ctx, qam_id);
    boolean new_mode = (new_cfg->enable_flag && new_cfg->encrypt_flag);
    int new_onid = new_cfg->onid;
    int new_tsid = new_cfg->tsid;
    boolean mode_changed = (new_mode != old_mode);
    boolean cfg_changed = ((new_onid != old_onid) || (new_tsid != old_tsid));

    VIDEO_CA_DEBUG("Report qam chg: qam %d, mode %d->%d, onid %d->%d, "\
                   "tsid %d->%d", qam_id, old_mode, new_mode, old_onid,
                   new_onid, old_tsid, new_tsid);
    VIDEO_CA_DEBUG("  enabled %d, encrypt %d",
                   new_cfg->enable_flag, new_cfg->encrypt_flag);

    if (mode_changed) {
        if (new_mode) {         // turning on
            video_ca_send_add_ts(qam_id);
        } else {                // turning off
            video_ca_send_delete_ts(qam_id);
        }
    } else {
        if (new_mode && cfg_changed) {    // config changed
            video_ca_send_change_ts(qam_id, old_onid, old_tsid);
        }
    }
}

void video_ca_report_add_ts(void)
{
    int qam_id = 0;
    qam_config_t* cfg = NULL;

    VIDEO_LOG("Report ADD TS to CA for all encrypted qams");
    
    for (qam_id=0; qam_id < NUM_QAMS; qam_id++) {
         cfg = get_qam_config(video_ctx, qam_id);
         if (cfg !=NULL) {
             if (cfg->enable_flag && cfg->encrypt_flag) {
                 video_ca_send_add_ts(qam_id);
             }
         }
    }
    
    VIDEO_LOG("Reported ADD TS to CA for all encrypted qams");
}

// Add PID delete action to PMT change table
//   Assume all PID deletes are recorded first before PID adds for a service
//
int video_ca_delete_pid (pmt_change_t *pmt_chg, uint16 pid, uint16 stream_type)
{
    es_change_t* es_chg;

    if (pmt_chg->count == MAX_UPDATES) {
        return E2BIG;
    }

    es_chg = &pmt_chg->table[pmt_chg->count++];
    es_chg->pid = pid;
    es_chg->stream_type = stream_type;
    es_chg->action = ES_ACTION_DELETE;
    return EOK;
}

   
// Record deletion action all ES PIDs in PMT
void video_ca_delete_pmt_es_pids (pmt_change_t *pmt_chg, pmt_info_t *pmt)
{
    int i;
    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt_info_get_es(pmt, i);
        video_ca_delete_pid(pmt_chg, pmt_get_es_pid(es_info), 
                            video_ca_get_stream_type(es_info->es_type));
    }
}


// Add PID add action to PMT change table
//   Assume all PID deletes are recorded first before PID adds for a service
//
int video_ca_add_pid (pmt_change_t *pmt_chg, uint16 pid, uint16 stream_type)
{
    int i;
    es_change_t* es_chg;
    es_change_t* es_not_used = NULL;

    for (i=0, es_chg=pmt_chg->table; i<pmt_chg->count; i++, es_chg++) {
        if (es_chg->pid == pid && es_chg->stream_type == stream_type) {
            if (es_chg->action != ES_ACTION_DELETE) {
                // Unexpected case!
                return EINVAL;
            }

            // Just cancel out the entries
            es_chg->pid = 0;
            return EOK;
        }

        if (!es_chg->pid && !es_not_used) {
            es_not_used = es_chg;   // entry that can be used
        }
    }

    if (es_not_used) {
        es_chg = es_not_used;
    } else {
        if (pmt_chg->count == MAX_UPDATES) {
            return E2BIG;
        }
        es_chg = &pmt_chg->table[pmt_chg->count++];
    }

    es_chg->pid = pid;
    es_chg->stream_type = stream_type;
    es_chg->action = ES_ACTION_ADD;
    return EOK;
}


// Record addition action all ES PIDs in PMT
void video_ca_add_pmt_es_pids (pmt_change_t *pmt_chg, pmt_info_t *pmt)
{
    int i;
    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt_info_get_es(pmt, i);
        video_ca_add_pid(pmt_chg, pmt_get_es_pid(es_info),
                            video_ca_get_stream_type(es_info->es_type));
    }
}


// Report all PID changes for a service
//   Returns the number of updates sent
//
int video_ca_report_pmt_change (uint16 qam_id, pmt_change_t *pmt_chg)
{
    int i;
    int chg_cnt = 0;
    es_change_t* es_chg;

    for (i=0, es_chg=pmt_chg->table; i<pmt_chg->count; i++, es_chg++) {
        if (!es_chg->pid)  continue;

        if (es_chg->action == ES_ACTION_ADD) {
            video_ca_send_add_pid(qam_id, es_chg->pid);
        } else {
            video_ca_send_delete_pid(qam_id, es_chg->pid);
        }
        chg_cnt++;
    }

    return chg_cnt;
}


#define ECM_PKTS_COPIED_BIT 0x80000000  // upper most bit in num_tp field is used to indicate whether the packets 
                                        // have been copied from shared mem to bootmem 
#define ECM_NUM_PKTS_BITS   0x7FFFFFFF  // all other bits in num_tp field are used for the number of packets 

int ecm_insert_schedule (crsl_insert_t *insert, uint32 qam_id,
                         uint32 flow_id, uint32 offset __UNUSED, uint16 out_pid)
{
    uint32 i;
    uintptr_t bootmem_tp_addr;
    boolean pid_remap = (out_pid != INVALID_PID);
    ecm_info_t *p_ecm_info;
    ecm_info_t ecm_info;
    uint32 num_ecm_tp;
    uint16* cmdbuf_avail = &get_qam_info(qam_id)->cmdbuf_avail[flow_id];
    assert(insert);
    assert(insert->ecm_flag);

    p_ecm_info = get_ecm_info((uintptr_t)insert->crsl);
    if (!p_ecm_info) {
        // Skip this turn
        insert->next_time += insert->period;
        return 0;
    }

    // Copy ecm_info from shared memory to local
    ecm_info = *p_ecm_info;
    // Mask only the num pkts bits in this field (top bit is the copy bit)
    num_ecm_tp = (ecm_info.num_tp & ECM_NUM_PKTS_BITS);
    if (*cmdbuf_avail <= num_ecm_tp) {
        *cmdbuf_avail = video_get_flow_avail_space(qam_id, flow_id);

        if (*cmdbuf_avail <= num_ecm_tp) {
            VIDEO_OUT_DEBUG(", NO-CMD %d", *cmdbuf_avail);
            VIDEO_LOG("QAM %d: Skipped ECM insertion due to output block",
                      qam_id);
            return 0;
        }
    }

    //use the shared bootmem address & offset to get bootmem tp addr
    //logical & physical addr are same.
    bootmem_tp_addr = (uintptr_t)(ecm_pkt_buf + ecm_info.ecm_pkt_offset);

    for (i=0; i<num_ecm_tp; i++) {
        out_command_t* cmd
            = setup_outcmd(qam_id, flow_id, sys_clk.base & OUT_TIME_MASK,
                           bootmem_tp_addr);

        if (pid_remap) {
            outcmd_set_pid_restamp(cmd, out_pid);
        }
        outcmd_set_cc_restamp(cmd, insert->next_cc++);
        insert->next_cc &= 0xF;
        bootmem_tp_addr += MAX_MPEG_PKT_SIZE;
    }

    (*cmdbuf_avail) -= num_ecm_tp;

    insert->cnt++;
    insert->period = ecm_info.insert_period;
    insert->next_time += insert->period;

    // Make sure that we are scheduling this in the future
    if (insert->next_time < sys_clk.base) {
        insert->next_time = sys_clk.base + insert->period;
    }

    return num_ecm_tp;
}


static
int exec_video_ca_init (cli_control_block *ccb __UNUSED)
{
    video_ca_init_shm();
    return EOK;
}


#if 0
static
void ca_info_print (FILE *fp, ca_info_t *ca_info)
{
    powerkey_ca_info_t* pk_ca_info = &ca_info->ca_info_hdr;
    fprintf(fp, "is_encrypted %d\n", pk_ca_info->is_encrypted);
    fprintf(fp, "start_immediate %d\n", pk_ca_info->start_immediate);
    fprintf(fp, "start_time %d\n", pk_ca_info->start_time);
    fprintf(fp, "ecm_update_period %d\n", pk_ca_info->ecm_update_period);
    fprintf(fp, "cw_update_period %d\n", pk_ca_info->cw_update_period);
    fprintf(fp, "cw_setup_period %d\n", pk_ca_info->cw_setup_period);
    fprintf(fp, "powerKeyCaData_len %d\n", pk_ca_info->powerKeyCaData_len);

    int i;
    uintptr_t ptr = (uintptr_t)pk_ca_info->powerKeyCaData_val;
    for (i=0; i<(int)pk_ca_info->powerKeyCaData_len; i++) {
        powerkey_ca_data *ca_data = (powerkey_ca_data*)ptr;
        fprintf(fp, "Table %d:\n", i);
        fprintf(fp, "  typeOfData %d\n", ca_data->typeOfData);
        fprintf(fp, "  deliveryId %d\n", ca_data->deliveryId);
        fprintf(fp, "  serialNumber %d\n", ca_data->serialNumber);
        fprintf(fp, "  caData_len %d\n", ca_data->caData_len);
        fprint_hex(fp, ca_data->caData_val, ca_data->caData_len);
        fprintf(fp, "\n");

        ptr += sizeof(powerkey_ca_data) + ca_data->caData_len;
        ptr = (ptr + 3) & 0xFFFFFFFFFFFFFFC;
    }
}


static void exec_video_show_out_session_ca_info (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    uint32 rid = ccb->data[3].i32;
    if (rid >= MAX_SESSIONS) {
        fprintf(ccb->fp, "Error: Session id %d out of range\n", rid);
        return;
    }

    out_config_t* cfg = get_out_session_config(video_ctx, rid);
    if (!cfg->used_flag) {
        fprintf(ccb->fp, "Output session config %d not used\n", rid);
        return;
    }

    fprintf(ccb->fp, "CA info:\n");
    ca_info_t* ca_info = get_ca_info(video_ctx, cfg->ca_info_idx);
    if (ca_info) {
        ca_info_print(ccb->fp, ca_info);
    } else {
        fprintf(ccb->fp, "  N/A\n");
    }
}
#endif /* 0 */


static boolean check_ecm_addr (uintptr_t ecm_addr)
{
    if (ecm_pkt_buf == MAP_FAILED) {
        VIDEO_DEBUG("ECM ADDR CHECK FAILED: mapping failed (0x%X vs 0x%X)",
                    (uintptr_t)ecm_pkt_buf,(uintptr_t)MAP_FAILED);
        return FALSE;
    }

    uintptr_t diff = ecm_addr - (uintptr_t)ecm_pkt_buf;
    if (diff % ECM_PKT_SIZE) {
        VIDEO_DEBUG("ECM ADDR CHECK FAILED:diff:0x%X, ecm_addr:0x%X pkt_size: 0x%X",
               diff, ecm_addr, ECM_PKT_SIZE);
        return FALSE;
    }
    if (diff / ECM_PKT_SIZE >= MAX_ECM) {
        VIDEO_DEBUG("ECM ADDR CHECK FAILED:diff:0x%X, pkt_size:0x%X, max_ecm:0x%X",
               diff, ECM_PKT_SIZE, MAX_ECM);
        return FALSE;
    }
    return TRUE;
}


static void exec_video_ca_show_ecm (cli_control_block *ccb)
{
    uint32 id = ccb->data[4].i32;
    ecm_info_t *ecm_info;
    uintptr_t ecm_addr;
    int num_tp;
    int i;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

#if 0
  {
    uintptr_t ecm_info_offset;
    fprintf(ccb->fp, "ECM info index: %d\n", id);
    ecm_info_offset = ecm_index_buf[id];
    fprintf(ccb->fp, "ECM info offset: %lu\n", ecm_info_offset);
    fprintf(ccb->fp, "ECM info buf: %016lx, => %016lx\n",
            (uintptr_t)ecm_info_buf,
            ((uintptr_t)ecm_info_buf) + ecm_info_offset);
    ecm_info = (ecm_info_t*)&ecm_info_buf[ecm_info_offset];
    fprintf(ccb->fp, "ECM pkt offset %lu, num TP %d, period %d\n",
            ecm_info->ecm_pkt_offset, ecm_info->num_tp, ecm_info->period);
  }
#endif

    ecm_info = get_ecm_info(id);
    if (!ecm_info) {
        fprintf(ccb->fp, "Bad ECM info\n");
        return;
    }

    num_tp = ecm_info->num_tp;
    ecm_addr = (uintptr_t)&ecm_pkt_buf[ecm_info->ecm_pkt_offset];

    fprintf(ccb->fp, "ECM TP address %016x\n", ecm_addr);
    fprintf(ccb->fp, "Number of TPs: %d\n", num_tp);
    fprintf(ccb->fp, "Insertion period: %d\n", ecm_info->insert_period);
    if (!check_ecm_addr(ecm_addr)) {
        fprintf(ccb->fp, "Invalid ECM address\n");
        return;
    }

    for (i=0; i<num_tp; i++) {
        fprintf(ccb->fp, "TP %d:\n", i);
        fprint_hex(ccb->fp, (void*)ecm_addr, TP_SIZE);
    }
}


#if 0
static
void exec_video_ca_show_ipc (cli_control_block *ccb)
{
    fprintf(ccb->fp, "Video CA IPC statistics:\n");
    /*
    fprintf(ccb->fp, "  SCS: ok %d, timeout %d\n",
           scs_ipc_stat.ok_cnt, scs_ipc_stat.timeout_cnt);
    */
}
#endif /* 0 */


// Video CA CLI handler 
boolean video_ca_cli_handler (cli_control_block *ccb)
{
    switch (ccb->fooid) {

    case CLI_VIDEO_CA_INIT:
        exec_video_ca_init(ccb);
        break;

    case CLI_VIDEO_CA_SHOW_ECM:
        exec_video_ca_show_ecm(ccb);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void video_ca_apply_desc (out_prog_t *prog, uint32 qam_id)
{
    if (!prog)  {
        VIDEO_CA_DEBUG("\n%s %d: NULL prog\n", __FUNCTION__, __LINE__);
        return;
    }
    // pre enc or resvd uninit ca data
    if (prog->pmt.ca_desc_cnt ||
       !ca_desc_get_sys_id(&prog->ca_desc_data))  {
        VIDEO_CA_DEBUG("\n%s %d: pmt ver %d, ca_desc_cnt %d, uninit ca_desc_data %d", 
                       __FUNCTION__, __LINE__, prog->pmt.ver, prog->pmt.ca_desc_cnt,
                       ca_desc_get_sys_id(&prog->ca_desc_data));
        // input src could change on off state
        video_ca_send_update_service(qam_id, prog->prog_num);
        return; 
    }

    pmt_add_ca_desc (&prog->pmt, &prog->ca_desc_data, prog->ca_desc_loc);
    VIDEO_CA_DEBUG("\n%s %d: prog_num %d added ca desc:sysid %d ca loc %d\n",
                    __FUNCTION__, __LINE__, prog->prog_num, ca_desc_get_sys_id(&prog->ca_desc_data), 
                    prog->ca_desc_loc);
}

void video_ca_block_update(out_session_t *ses, out_prog_t *prog) 
{
    qam_info_t* qam = get_qam_info(ses->qam_id);
    if (prog->blocked &&
       (prog->in_prog->pmt.ca_desc_cnt == 0)) {
        prog->blocked = 0;
        qam->pat_rebuild = 1;                 
        VIDEO_CA_DEBUG("\n%s %d: Unblocked for clear input prog %d, qam %d", 
                       __FUNCTION__, __LINE__, prog->in_prog->prog_num, qam->id);
        em_vidman_VIDEO_PROG_CA_BLOCKED("CA_UNBLOCKED", prog->in_prog->prog_num, qam->id);
        return;
    } 
    else if (!prog->blocked &&
        (prog->in_prog->pmt.ca_desc_cnt > 0)) {
        prog->blocked = 1;
        qam->pat_rebuild = 1;                 
        VIDEO_CA_DEBUG("\n%s %d: Blocked for ca-desc is present input prog %d, qam %d", 
                       __FUNCTION__, __LINE__, prog->in_prog->prog_num, qam->id);
        em_vidman_VIDEO_PROG_CA_BLOCKED("CA_BLOCKED", prog->in_prog->prog_num, qam->id);
        return;
    }
    //VIDEO_CA_DEBUG("\n%s %d:No block chg input prog %d, qam %d", __FUNCTION__, __LINE__, 
    //               prog->in_prog->prog_num, qam->id);
}
