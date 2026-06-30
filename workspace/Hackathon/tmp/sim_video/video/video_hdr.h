/*
 * Copyright (c) 2006-2012, 2014-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __VIDEO_HDR_H__
#define __VIDEO_HDR_H__


#define TP_SIZE                         188
#define TP_SIZE_BIT                     (TP_SIZE * 8)

#define MAX_PSI_SECT_SIZE               1024
#define MAX_PRIVATE_SECT_SIZE           4096

#define SYNC_BYTE                       0x47

// Well known PIDs
#define PAT_PID                         0
#define NULL_PID                        0x1fff

#define NUM_PIDS                        8192

#define PAT_SECT_HDR_SIZE               8
#define PMT_SECT_HDR_SIZE               12
#define PMT_ES_HDR_SIZE                 5

#define PAT_TABLE_ID                    0
#define PMT_TABLE_ID                    2

#define MAX_PROGS_PER_PAT_SECT          ((MAX_PSI_SECT_SIZE-PAT_SECT_HDR_SIZE-4) / 4)

#define STUFF_BYTE                      0xff

// Important descriptor tags
#define DESC_TAG_CA                     9
#define DESC_TAG_LANG                   10

// Encryption Session and Shared Memory constants (must match encryption's scrambling_itf.h)
#define ECM_PKTS_PER_GRP         4

// Support encryption on 192 carriers with average of 20 sessions/carrier
#define MAX_ENCR_OUTPUT_SESSIONS (3840)                     

// For powerkey, only need 1 insertion per session, however for DVB or 
//   dual encryption this will need to increase to support multiple CA systems
//   per session and/or pid level scrambling
#define MAX_ECM_INSERTS          (MAX_ENCR_OUTPUT_SESSIONS)
 
// For powerkey, need 2 groups for every insertion (1 for non-hint bit version,
//   1 for hint bit version) plus extra 512 to allow Bass to play out old table
//   before reusing the group
#define MAX_ECM_PKT_GROUPS       ((MAX_ECM_INSERTS * 2) + 512)  

// The number of ECM packets is just 4 times the number of groups
#define MAX_ECM_PKTS             (MAX_ECM_PKT_GROUPS * ECM_PKTS_PER_GRP) 

// Stream types defined in MPEG-2
#define STREAM_TYPE_VIDEO_MPEG1                 0x01
#define STREAM_TYPE_VIDEO_MPEG2                 0x02
#define STREAM_TYPE_AUDIO_MPEG1                 0x03
#define STREAM_TYPE_AUDIO_MPEG2                 0x04

// Stream types defined somewhere else
#define STREAM_TYPE_AUDIO_AAC                   0x0F
#define STREAM_TYPE_VIDEO_H264                  0x1B
#define STREAM_TYPE_VIDEO_UNKNOWN               0x80
#define STREAM_TYPE_AUDIO_AC3                   0x81

#define PSI_SECT_LEN_ADJ                3
#define CRC_SIZE                        4

#define NUM_PROGS                       65536


// IPv4 Header
//
#define IPV4_VERSION                    4
#define IPV4_LENGTH                     5
#define IP_FLAG_MORE_MASK               0x1
#define IPV4_ADDR_LEN                   1
#define IPV6_ADDR_LEN                   4

typedef struct {
    uint8_t ver : 4;
    uint8_t ihl : 4;
    uint8_t tos;
    uint16_t len;
    uint16_t identification;
    uint16_t flags : 3;
    uint16_t frag_offset : 13;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t hdr_chksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} ip_header_t;


// IPv6 Header
//
#define IPV6_VERSION                    6

typedef struct {
    uint32_t ver : 4;
    uint32_t traffic_class : 8;
    uint32_t flow_label : 20;

    uint16_t payload_length;
    uint8_t next_header;
    uint8_t hop_limit;

    uint32_t src_ip[4];
    uint32_t dst_ip[4];
} ipv6_header_t;


// UPD Header
//
#define IP_PROT_UDP                     17

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t chksum;
} udp_header_t;


#define RTP_VERSION                     2
#define RTP_PAYLOAD_TYPE_MPEG2          33

// RTP Header
//
typedef struct {
    // Word 1
    uint32_t  version     : 2;    // version number (should be 2)
    uint32_t  padding     : 1;
    uint32_t  ext         : 1;    // extension
    uint32_t  csrc_cnt    : 4;    // number of CSRC identifiers
    uint32_t  marker      : 1;    // Used to indicate timestamp discontinuity
    uint32_t  payload_type: 7;    // payload type: 33 for MPEG TS (RFC 2250)
    uint32_t  sequence    : 16;   // sequence number
    uint32_t  timestamp;          // timestamp (in 90 kHz)
    uint32_t  ssrc;               // synchronization source
    uint32_t  csrc[0];            // 0 to 15 conributing sources
} rtp_header_t;


// RTP extension header
//
typedef struct {
    uint16_t prog_iden;
    uint16_t length;              // extension header length
} rtp_ext_header_t;


#define ETH_HDR_LEN                     14
#define IPV4_HDR_LEN                    20
#define IPV6_HDR_LEN                    40
#define UDP_HDR_LEN                     8
#define RTP_HDR_LEN                     12


// TP header
//
#ifdef __BIG_ENDIAN__
typedef struct {
    uint32_t sync : 8;
    uint32_t transport_err : 1;
    uint32_t payload_unit_start : 1;
    uint32_t priority : 1;
    uint32_t pid : 13;
    uint32_t scrambling_ctrl : 2;
    uint32_t af_ctrl : 2;
    uint32_t cc : 4;
} tp_header_t;

static inline uint16_t tp_get_pid (tp_header_t *hdr)
{
    return hdr->pid;
}

static inline void tp_set_pid (tp_header_t *hdr, uint16_t pid)
{
    hdr->pid = pid;
}
#endif  // __BIG_ENDIAN__


#ifdef __LITTLE_ENDIAN__
typedef struct {
    uint8_t sync;

    uint8_t pid_1 : 5;
    uint8_t priority : 1;
    uint8_t payload_unit_start : 1;
    uint8_t transport_err : 1;

    uint8_t pid_2;

    uint8_t cc : 4;
    uint8_t af_ctrl : 2;
    uint8_t scrambling_ctrl : 2;
} tp_header_t;

static inline uint16_t tp_get_pid (tp_header_t *hdr)
{
    return (hdr->pid_1 << 8) | hdr->pid_2;
}

static inline void tp_set_pid (tp_header_t *hdr, uint16_t pid)
{
    hdr->pid_1 = (pid >> 8) & 0x1F;
    hdr->pid_2 = pid & 0xFF;
}
#endif  // __LITTLE_ENDIAN__


// Adaptation field definition (for PCR processing)
//
#ifdef __BIG_ENDIAN__
typedef struct {
    uint8_t len;

    uint8_t discontinuity : 1;
    uint8_t random_access : 1;
    uint8_t es_priority : 1;
    uint8_t pcr_flag : 1;
    uint8_t opcr_flag : 1;
    uint8_t splice_point : 1;
    uint8_t private_data : 1;
    uint8_t af_ext : 1;

    uint16_t pcr_base_1;

    uint16_t pcr_base_2;

    uint16_t pcr_base_3 : 1;
    uint16_t resv : 6;
    uint16_t pcr_ext : 9;
} af_header_t;

static inline uint64_t af_get_pcr_base (af_header_t *hdr)
{
    return (hdr->pcr_base_1 << 17) | (hdr->pcr_base_2 << 1) | hdr->pcr_base_3;
}

static inline void af_set_pcr_base (af_header_t *hdr, uint64_t pcr_base)
{
    hdr->pcr_base_1 = (pcr_base >> 17) & 0xFFFF;
    hdr->pcr_base_2 = (pcr_base >> 1) & 0xFFFF;
    hdr->pcr_base_3 = pcr_base  & 1;
}

static inline uint16_t af_get_pcr_ext (af_header_t *hdr)
{
    return hdr->pcr_ext;
}

static inline void af_set_pcr_ext (af_header_t *hdr, uint16_t pcr_ext)
{
    hdr->pcr_ext = pcr_ext;
}

static inline void af_get_pcr_mod (af_header_t *hdr,
                                   uint32_t *pcr_base, int16_t *pcr_ext)
{
    *pcr_base = (hdr->pcr_base_1 << 16) | hdr->pcr_base_2;
    *pcr_ext = hdr->pcr_base_3 * 300 + hdr->pcr_ext;
}

static inline void af_set_pcr_mod (af_header_t *hdr,
                                   uint32_t pcr_base, uint16_t pcr_ext)
{
    hdr->pcr_base_1 = pcr_base >> 16;
    hdr->pcr_base_2 = pcr_base & 0xFFFF;
    hdr->pcr_base_3 = pcr_ext >= 300;
    hdr->pcr_ext = pcr_ext % 300;
}
#endif  // __BIG_ENDIAN__


#ifdef __LITTLE_ENDIAN__
typedef struct {
    uint8_t len;

    uint8_t af_ext : 1;
    uint8_t private_data : 1;
    uint8_t splice_point : 1;
    uint8_t opcr_flag : 1;
    uint8_t pcr_flag : 1;
    uint8_t es_priority : 1;
    uint8_t random_access : 1;
    uint8_t discontinuity : 1;

    uint8_t pcr_base_1;

    uint8_t pcr_base_2;

    uint8_t pcr_base_3;

    uint8_t pcr_base_4;

    uint8_t pcr_ext_1 : 1;
    uint8_t res : 6;
    uint8_t pcr_base_5 : 1;

    uint8_t pcr_ext_2;
} af_header_t;

static inline uint64_t af_get_pcr_base (af_header_t *hdr)
{
    return (hdr->pcr_base_1 << 25) | (hdr->pcr_base_2 << 17)
           | (hdr->pcr_base_3 << 9) | (hdr->pcr_base_4 << 1) | hdr->pcr_base_5;
}

static inline void af_set_pcr_base (af_header_t *hdr, uint64_t pcr_base)
{
    hdr->pcr_base_1 = (pcr_base >> 25) & 0xFF;
    hdr->pcr_base_2 = (pcr_base >> 17) & 0xFF;
    hdr->pcr_base_3 = (pcr_base >> 9) & 0xFF;
    hdr->pcr_base_4 = (pcr_base >> 1) & 0xFF;
    hdr->pcr_base_5 = pcr_base  & 1;
}

static inline uint16_t af_get_pcr_ext (af_header_t *hdr) {
    return (hdr->pcr_ext_1 << 8) | hdr->pcr_ext_2;
}

static inline void af_set_pcr_ext (af_header_t *hdr, uint16_t pcr_ext)
{
    hdr->pcr_ext_1 = (pcr_ext >> 8) & 1;
    hdr->pcr_ext_2 = pcr_ext & 0xFF;
}

static inline void af_get_pcr_mod (af_header_t *hdr,
                                   uint32_t *pcr_base, int16_t *pcr_ext)
{
    *pcr_base = (hdr->pcr_base_1 << 24) | (hdr->pcr_base_2 << 16)
                        | (hdr->pcr_base_3 << 8) | hdr->pcr_base_4;
    *pcr_ext = hdr->pcr_base_5 * 300 + ((hdr->pcr_ext_1 << 8) | hdr->pcr_ext_2);
}

static inline void af_set_pcr_mod (af_header_t *hdr,
                                   uint32_t pcr_base, uint16_t pcr_ext)
{
    hdr->pcr_base_1 = (pcr_base >> 24) & 0xFF;
    hdr->pcr_base_2 = (pcr_base >> 16) & 0xFF;
    hdr->pcr_base_3 = (pcr_base >> 8) & 0xFF;
    hdr->pcr_base_4 = pcr_base & 0xFF;
    hdr->pcr_base_5 = pcr_ext >= 300;
    pcr_ext %= 300;
    hdr->pcr_ext_1 = (pcr_ext >> 8) & 1;
    hdr->pcr_ext_2 = pcr_ext & 0xFF;
}
#endif  // __LITTLE_ENDIAN__


// Generic PSI section header
//
#ifdef __BIG_ENDIAN__
typedef struct {
    uint64_t table_id      : 8;
    uint64_t sect_syntax   : 1;
    uint64_t priv          : 1;
    uint64_t resv1         : 2;
    uint64_t sect_len      : 12;
    uint64_t table_id_ext  : 16;
    uint64_t resv2         : 2;
    uint64_t ver           : 5;
    uint64_t cur_next      : 1;
    uint64_t sect_num      : 8;
    uint64_t last_sect_num : 8;
} psi_section_header_t;

// Note: return the actual size of the PSI section, NOT the sect_lenh field!
static inline uint16_t psi_get_section_size (psi_section_header_t *hdr)
{
    return hdr->sect_len + PSI_SECT_LEN_ADJ;
}

static inline void psi_set_section_size (psi_section_header_t *hdr,
                                         uint16_t size)
{
    hdr->sect_len = size - PSI_SECT_LEN_ADJ;
}
#endif  // __BIG_ENDIAN__


#ifdef __LITTLE_ENDIAN__
typedef struct {
    uint8_t table_id;

    uint8_t sect_len_1    : 4;
    uint8_t resv1         : 2;
    uint8_t priv          : 1;
    uint8_t sect_syntax   : 1;

    uint8_t sect_len_2;

    uint8_t table_id_ext_1;

    uint8_t table_id_ext_2;

    uint8_t cur_next      : 1;
    uint8_t ver           : 5;
    uint8_t resv2         : 2;

    uint8_t sect_num;

    uint8_t last_sect_num;
} psi_section_header_t;

// Note: return the actual size of the PSI section, NOT the sect_lenh field!
static inline uint16_t psi_get_section_size (psi_section_header_t *hdr) {
    return ((hdr->sect_len_1 << 8) | hdr->sect_len_2) + PSI_SECT_LEN_ADJ;
}

static inline void psi_set_section_size (psi_section_header_t *hdr,
                                         uint16_t size) {
    size -= PSI_SECT_LEN_ADJ;
    hdr->sect_len_1 = (size >> 8 ) & 0xF;
    hdr->sect_len_2 = size & 0xFF;
}
#endif  // __LITTLE_ENDIAN__


// PAT section header
//
#ifdef __BIG_ENDIAN__
typedef struct {
    uint64_t table_id : 8;
    uint64_t sect_syntax : 1;
    uint64_t priv : 1;
    uint64_t resv1 : 2;
    uint64_t sect_len : 12;
    uint64_t tsid : 16;
    uint64_t resv2 : 2;
    uint64_t ver : 5;
    uint64_t cur_next : 1;
    uint64_t sect_num : 8;
    uint64_t last_sect_num : 8;
} pat_header_t;

// PAT program info
//
typedef struct {
    uint16_t prog_num;
    uint16_t resv : 3;
    uint16_t pmt_pid : 13;
} pat_prog_info_t;

static inline uint16_t pat_get_tsid (pat_header_t *hdr)
{
    return hdr->tsid;
}

static inline void pat_set_tsid (pat_header_t *hdr, uint16_t tsid)
{
    hdr->tsid = tsid;
}

static inline uint16_t pat_get_prog_num (pat_prog_info_t *info)
{
    return info->prog_num;
}

static inline void pat_set_prog_num (pat_prog_info_t *info, uint16_t prog_num)
{
    info->prog_num = prog_num;
}

static inline uint16_t pat_get_pmt_pid (pat_prog_info_t *info)
{
    return info->pmt_pid;
}

static inline void pat_set_pmt_pid (pat_prog_info_t *info, uint16_t pmt_pid)
{
    info->pmt_pid = pmt_pid;
}
#endif  // __BIG_ENDIAN__


#ifdef __LITTLE_ENDIAN__
typedef struct {
    uint8_t table_id;

    uint8_t sect_len_1 : 4;
    uint8_t resv1 : 2;
    uint8_t priv : 1;
    uint8_t sect_syntax : 1;

    uint8_t sect_len_2;

    uint8_t tsid_1;

    uint8_t tsid_2;

    uint8_t cur_next : 1;
    uint8_t ver : 5;
    uint8_t resv2 : 2;

    uint8_t sect_num;

    uint8_t last_sect_num;
} pat_header_t;

// PAT program info
typedef struct {
    uint8_t prog_num_1;

    uint8_t prog_num_2;

    uint8_t pmt_pid_1 : 5;
    uint8_t resv : 3;

    uint8_t pmt_pid_2;
} pat_prog_info_t;

static inline uint16_t pat_get_tsid (pat_header_t *hdr)
{
    return (hdr->tsid_1 << 8) | hdr->tsid_2;
}

static inline void pat_set_tsid (pat_header_t *hdr, uint16_t tsid)
{
    hdr->tsid_1 = (tsid >> 8) & 0xFF;
    hdr->tsid_2 = tsid & 0xFF;
}

static inline uint16_t pat_get_prog_num (pat_prog_info_t *info)
{
    return (info->prog_num_1 << 8) | info->prog_num_2;
}

static inline void pat_set_prog_num (pat_prog_info_t *info, uint16_t prog_num)
{
    info->prog_num_1 = (prog_num >> 8) & 0xFF;
    info->prog_num_2 = prog_num & 0xFF;
}

static inline uint16_t pat_get_pmt_pid (pat_prog_info_t *info)
{
    return (info->pmt_pid_1 << 8) | info->pmt_pid_2;
}

static inline void pat_set_pmt_pid (pat_prog_info_t *info, uint16_t pmt_pid)
{
    info->pmt_pid_1 = (pmt_pid >> 8) & 0x1F;
    info->pmt_pid_2 = pmt_pid & 0xFF;
}
#endif  // __LITTLE_ENDIAN__

//#if defined(__BIG_ENDIAN__) || defined(__LITTLE_ENDIAN__)
static inline uint16_t pat_get_section_size (pat_header_t *hdr)
{
    return psi_get_section_size((psi_section_header_t*)hdr);
}

static inline void pat_set_section_size (pat_header_t *hdr, uint16_t size)
{
    psi_set_section_size((psi_section_header_t*)hdr, size);
}
//#endif


// PMT section header
//

#ifdef __BIG_ENDIAN__
// Note: do not use sizeof(pmt_header_t)!!  Use PMT_SECT_HDR_SIZE instead.

typedef struct {
    uint32_t table_id : 8;
    uint32_t sect_syntax : 1;
    uint32_t priv : 1;
    uint32_t resv1 : 2;
    uint32_t sect_len : 12;
    uint32_t prog_num_1 : 8;

    uint32_t prog_num_2 : 8;
    uint32_t resv2 : 2;
    uint32_t ver : 5;
    uint32_t cur_next : 1;
    uint32_t sect_num : 8;
    uint32_t last_sect_num : 8;

    uint32_t resv3 : 3;
    uint32_t pcr_pid : 13;
    uint32_t resv4 : 4;
    uint32_t prog_info_len : 12;
}
#if __GNUC__ > 3
 __attribute__((__may_alias__))
#endif
pmt_header_t;

// PMT elementary stream info
typedef struct {
    uint8_t es_type;

    uint8_t resv1 : 3;
    uint8_t pid_1 : 5;

    uint8_t pid_2;

    uint8_t resv2 : 4;
    uint8_t es_info_len_1 : 4;

    uint8_t es_info_len_2;
} pmt_es_info_t;

static inline uint16_t pmt_get_pcr_pid (pmt_header_t *hdr)
{
    return hdr->pcr_pid;
}

static inline void pmt_set_pcr_pid (pmt_header_t *hdr, uint16_t pcr_pid)
{
    hdr->pcr_pid = pcr_pid;
}

static inline uint16_t pmt_get_prog_info_len (pmt_header_t *hdr)
{
    return hdr->prog_info_len;
}

static inline void pmt_set_prog_info_len (pmt_header_t *hdr, uint16_t info_len)
{
    hdr->prog_info_len = info_len;
}

static inline uint16_t pmt_get_es_pid (pmt_es_info_t *info)
{
    return (info->pid_1 << 8) | info->pid_2;
}

static inline void pmt_set_es_pid (pmt_es_info_t *info, uint16_t es_pid)
{
    info->pid_1 = (es_pid >> 8 ) & 0x1F;
    info->pid_2 = es_pid & 0xFF;
}
#endif  // __BIG_ENDIAN__


#ifdef __LITTLE_ENDIAN__
typedef struct {
    uint8_t table_id;

    uint8_t sect_len_1 : 4;
    uint8_t resv1 : 2;
    uint8_t priv : 1;
    uint8_t sect_syntax : 1;

    uint8_t sect_len_2;

    uint8_t prog_num_1;

    uint8_t prog_num_2;

    uint8_t cur_next : 1;
    uint8_t ver : 5;
    uint8_t resv2 : 2;

    uint8_t sect_num;

    uint8_t last_sect_num;

    uint8_t pcr_pid_1 : 5;
    uint8_t resv3 : 3;

    uint8_t pcr_pid_2;

    uint8_t prog_info_len_1 : 4;
    uint8_t resv4 : 4;

    uint8_t prog_info_len_2;
} pmt_header_t;

// PMT elementary stream info
typedef struct {
    uint8_t es_type : 8;

    uint8_t pid_1 : 5;
    uint8_t resv1 : 3;

    uint8_t pid_2;

    uint8_t es_info_len_1 : 4;
    uint8_t resv2 : 4;

    uint8_t es_info_len_2;
} pmt_es_info_t;

static inline uint16_t pmt_get_pcr_pid (pmt_header_t *hdr)
{
    return (hdr->pcr_pid_1 << 8) | hdr->pcr_pid_2;
}

static inline void pmt_set_pcr_pid (pmt_header_t *hdr, uint16_t pcr_pid)
{
    hdr->pcr_pid_1 = (pcr_pid >> 8) & 0x1F;
    hdr->pcr_pid_2 = pcr_pid & 0xFF;
}

static inline uint16_t pmt_get_prog_info_len (pmt_header_t *hdr)
{
    return (hdr->prog_info_len_1 << 8) | hdr->prog_info_len_2;
}

static inline void pmt_set_prog_info_len (pmt_header_t *hdr, uint16_t info_len)
{
    hdr->prog_info_len_1 = (info_len >> 8) & 0xF;
    hdr->prog_info_len_2 = info_len & 0xFF;
}

static inline uint16_t pmt_get_es_pid (pmt_es_info_t *info)
{
    return (info->pid_1 << 8) | info->pid_2;
}

static inline void pmt_set_es_pid (pmt_es_info_t *info, uint16_t pid)
{
    info->pid_1 = (pid >> 8) & 0x1F;
    info->pid_2 = pid & 0xFF;
}
#endif  // __LITTLE_ENDIAN__

//#if defined(__BIG_ENDIAN__) || defined(__LITTLE_ENDIAN__)
static inline uint16_t pmt_get_section_size (pmt_header_t *hdr)
{
    return psi_get_section_size((psi_section_header_t*)hdr);
}

static inline void pmt_set_section_size (pmt_header_t *hdr, uint16_t size)
{
    psi_set_section_size((psi_section_header_t*)hdr, size);
}

static inline uint16_t pmt_get_prog_num (pmt_header_t *hdr)
{
    return (hdr->prog_num_1 << 8) | hdr->prog_num_2;
}

static inline void pmt_set_prog_num (pmt_header_t *hdr, uint16_t prog_num)
{
    hdr->prog_num_1 = (prog_num >> 8) & 0xFF;
    hdr->prog_num_2 = prog_num & 0xFF;
}

static inline uint16_t pmt_get_es_info_len (pmt_es_info_t *info)
{
    return (info->es_info_len_1 << 8) | info->es_info_len_2;
}

static inline void pmt_set_es_info_len (pmt_es_info_t *info, uint16_t info_len)
{
    info->es_info_len_1 = (info_len >> 8) & 0xF;
    info->es_info_len_2 = info_len & 0xFF;
}
//#endif


// Generic descriptor header
//
typedef struct {
    uint8_t tag;
    uint8_t len;
} descriptor_header_t;


// CA descriptor header
//
typedef struct {
    uint8_t tag;
    uint8_t len;
    uint8_t sys_id_hi;
    uint8_t sys_id_lo;
    uint8_t resv : 3;
    uint8_t pid_hi : 5;
    uint8_t pid_lo;
} ca_descriptor_header_t;

typedef struct {
    ca_descriptor_header_t ca_header;
    uint8_t priv_data1;
    uint8_t priv_data2;
    uint8_t priv_data3;
} ext_ca_descr_header_t;


// Get CA system ID from CA descriptor
static inline
uint16_t ca_desc_get_sys_id (ca_descriptor_header_t *desc)
{
    return (desc->sys_id_hi<<8) | desc->sys_id_lo;
}


// Set CA system ID in CA descriptor
static inline
void ca_desc_set_sys_id (ca_descriptor_header_t *desc, uint16_t sys_id)
{
    desc->sys_id_hi = sys_id >> 8;
    desc->sys_id_lo = sys_id & 0xFF;
}


// Get CA PID from CA descriptor
static inline
uint16_t ca_desc_get_pid (ca_descriptor_header_t *desc)
{
    return (desc->pid_hi<<8) | desc->pid_lo;
}


// Set CA PID in CA descriptor
static inline
void ca_desc_set_pid (ca_descriptor_header_t *desc, uint16_t pid)
{
    desc->pid_hi = pid >> 8;
    desc->pid_lo = pid & 0xFF;
}

#endif  /* __RFGW_VIDEO_HDR_H__ */

