/*
 * Copyright (c) 2006-2012, 2014 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __RFGW_VIDEO_HDR_H__
#define __RFGW_VIDEO_HDR_H__


#define TP_SIZE                         188
#define MAX_MPEG_PKT_SIZE				256				//must be a power of 2 in order to divide by 4096 in FW
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

#define STUFF_BYTE                      0xff

#define CA_DESCRIPTOR_TAG               9

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

#define SECT_LEN_ADJ                    3
#define CRC_SIZE                        4

#define NUM_PROGS                       65536

#define MAX_RFGW_CA_DESC_PER_PROG       8


// IPv4 Header
//
#define IPV4_VERSION                    4
#define IPV4_LENGTH                     5
#define IP_FLAG_MORE_MASK               0x1

typedef struct ip_header_ {
    uint8 ver : 4;
    uint8 ihl : 4;
    uint8 tos;
    uint16 len;
    uint16 identification;
    uint16 flags : 3;
    uint16 frag_offset : 13;
    uint8 ttl;
    uint8 protocol;
    uint16 hdr_chksum;
    uint32 src_ip;
    uint32 dst_ip;
} ip_header_t;


// IPv6 Header
//
#define IPV6_VERSION                    6

typedef struct ipv6_header_ {
    uint32 ver : 4;
    uint32 traffic_class : 8;
    uint32 flow_label : 20;

    uint16 payload_length;
    uint8 next_header;
    uint8 hop_limit;

    uint32 src_ip[4];
    uint32 dst_ip[4];
} ipv6_header_t;


// UPD Header
//
#define IP_PROT_UDP                     17

typedef struct udp_header_ {
    uint16 src_port;
    uint16 dst_port;
    uint16 length;
    uint16 chksum;
} udp_header_t;


#define RTP_VERSION                     2
#define RTP_PAYLOAD_TYPE_MPEG2          33

// RTP Header
//
typedef struct rtp_header_ {
    // Word 1
    uint32  version     : 2;    // version number (should be 2)
    uint32  padding     : 1;
    uint32  ext         : 1;    // extension
    uint32  csrc_cnt    : 4;    // number of CSRC identifiers
    uint32  marker      : 1;    // Used to indicate timestamp discontinuity
    uint32  payload_type: 7;    // payload type: 33 for MPEG TS (RFC 2250)
    uint32  sequence    : 16;   // sequence number
    uint32  timestamp;          // timestamp (in 90 kHz)
    uint32  ssrc;               // synchronization source
    uint32  csrc[0];            // 0 to 15 conributing sources
} rtp_header_t;


// RTP extension header
//
typedef struct rtp_ext_header_ {
    uint16 prog_iden;
    uint16 length;              // extension header length
} rtp_ext_header_t;


// TP header
#if _BIG_ENDIAN
typedef struct tp_header_ {
    uint32 sync : 8;
    uint32 transport_err : 1;
    uint32 payload_unit_start : 1;
    uint32 priority : 1;
    uint32 pid : 13;
    uint32 scrambling_ctrl : 2;
    uint32 af_ctrl : 2;
    uint32 cc : 4;
} tp_header_t;

static inline uint16 tp_get_pid (tp_header_t *hdr) {
    return hdr->pid;
}
#else

typedef struct {
    uint8 sync;

    uint8 pid_1 : 5;
    uint8 priority : 1;
    uint8 payload_unit_start : 1;
    uint8 transport_err : 1;

    uint8 pid_2;

    uint8 cc : 4;
    uint8 af_ctrl : 2;
    uint8 scrambling_ctrl : 2;
} tp_header_t;

static inline uint16 tp_get_pid (tp_header_t *hdr) {
    return (hdr->pid_1 << 8) | hdr->pid_2;
}
#endif

#if _BIG_ENDIAN
// Adaptation field definition (for PCR processing)
typedef struct af_header_ {
    uint8 len;

    uint8 discontinuity : 1;
    uint8 random_access : 1;
    uint8 es_priority : 1;
    uint8 pcr_flag : 1;
    uint8 opcr_flag : 1;
    uint8 splice_point : 1;
    uint8 private_data : 1;
    uint8 af_ext : 1;

    uint16 pcr_base_1;

    uint16 pcr_base_2;

    uint16 pcr_base_3 : 1;
    uint16 resv : 6;
    uint16 pcr_ext : 9;
} af_header_t;

#else

typedef struct {
    uint8 len;

    uint8 af_ext : 1;
    uint8 private_data : 1;
    uint8 splice_point : 1;
    uint8 opcr_flag : 1;
    uint8 pcr_flag : 1;
    uint8 es_priority : 1;
    uint8 random_access : 1;
    uint8 discontinuity : 1;

    uint8 pcr_base_1;

    uint8 pcr_base_2;

    uint8 pcr_base_3;

    uint8 pcr_base_4;

    uint8 pcr_ext_1 : 1;
    uint8 res : 6;
    uint8 pcr_base_5 : 1;

    uint8 pcr_ext_2;
} af_header_t;

static inline uint64 af_get_pcr_base (af_header_t *hdr)
{
    return (hdr->pcr_base_1 << 25) | (hdr->pcr_base_2 << 17)
           | (hdr->pcr_base_3 << 9) | (hdr->pcr_base_4 << 1) | hdr->pcr_base_5;
}

static inline uint16 af_get_pcr_ext (af_header_t *hdr) {
    return (hdr->pcr_ext_1 << 8) | hdr->pcr_ext_2;
}
#endif


// Generic PSI section header
//
typedef struct psi_section_header_ {
    uint64 table_id      : 8;
    uint64 sect_syntax   : 1;
    uint64 priv          : 1;
    uint64 resv1         : 2;
    uint64 sect_len      : 12;
    uint64 table_id_ext  : 16;
    uint64 resv2         : 2;
    uint64 ver           : 5;
    uint64 cur_next      : 1;
    uint64 sect_num      : 8;
    uint64 last_sect_num : 8;
} psi_section_header_t;


// PAT section header
#if _BIG_ENDIAN
typedef struct pat_header_ {
    uint64 table_id : 8;
    uint64 sect_syntax : 1;
    uint64 priv : 1;
    uint64 resv1 : 2;
    uint64 sect_len : 12;
    uint64 tsid : 16;
    uint64 resv2 : 2;
    uint64 ver : 5;
    uint64 cur_next : 1;
    uint64 sect_num : 8;
    uint64 last_sect_num : 8;
} pat_header_t;

#else

typedef struct pat_header_ {
    uint8 table_id;

    uint8 sect_len_1 : 4;
    uint8 resv1 : 2;
    uint8 priv : 1;
    uint8 sect_syntax : 1;

    uint8 sect_len_2;

    uint8 tsid_1;

    uint8 tsid_2;

    uint8 cur_next : 1;
    uint8 ver : 5;
    uint8 resv2 : 2;

    uint8 sect_num;

    uint8 last_sect_num;
} pat_header_t;

static inline uint16 pat_get_section_size (pat_header_t *hdr) {  
    return ((hdr->sect_len_1 << 8) | hdr->sect_len_2) + 3;
}

static inline uint16 pat_get_tsid (pat_header_t *hdr) {  
    return (hdr->tsid_1 << 8) | hdr->tsid_2;
}
#endif


// PAT program info
#if _BIG_ENDIAN
typedef struct pat_prog_info_ {
    uint16 prog_num;
    uint16 resv : 3;
    uint16 pmt_pid : 13;
} pat_prog_info_t;

#else

typedef struct pat_prog_info_ {
    uint8 prog_num_1;

    uint8 prog_num_2;

    uint8 pmt_pid_1 : 5;
    uint8 resv : 3;

    uint8 pmt_pid_2;
} pat_prog_info_t;

static inline uint16 pat_get_prog_num (pat_prog_info_t *info) {
    return (info->prog_num_1 << 8) | info->prog_num_2;
}

static inline uint16 pat_get_pmt_pid (pat_prog_info_t *info) {
    return (info->pmt_pid_1 << 8) | info->pmt_pid_2;
}
#endif


// PMT section header
// Note: do not use sizeof(pmt_header_t)!!  Use PMT_SECT_HDR_SIZE instead.

#if _BIG_ENDIAN
typedef struct pmt_header_ {
    uint64 table_id : 8;
    uint64 sect_syntax : 1;
    uint64 priv : 1;
    uint64 resv1 : 2;
    uint64 sect_len : 12;
    uint64 prog_num : 16;
    uint64 resv2 : 2;
    uint64 ver : 5;
    uint64 cur_next : 1;
    uint64 sect_num : 8;
    uint64 last_sect_num : 8;

    uint16 resv3 : 3;
    uint16 pcr_pid : 13;
    uint16 resv4 : 4;
    uint16 prog_info_len : 12;
}
#if __GNUC__ > 3
 __attribute__((__may_alias__))
#endif
pmt_header_t;

#else

typedef struct pmt_header_ {
    uint8 table_id;

    uint8 sect_len_1 : 4;
    uint8 resv1 : 2;
    uint8 priv : 1;
    uint8 sect_syntax : 1;

    uint8 sect_len_2;

    uint8 prog_num_1;

    uint8 prog_num_2;

    uint8 cur_next : 1;
    uint8 ver : 5;
    uint8 resv2 : 2;

    uint8 sect_num;

    uint8 last_sect_num;

    uint8 pcr_pid_1 : 5;
    uint8 resv3 : 3;

    uint8 pcr_pid_2;

    uint8 prog_info_len_1 : 4;
    uint8 resv4 : 4;

    uint8 prog_info_len_2;
} pmt_header_t;

static inline uint16 pmt_get_section_size (pmt_header_t *hdr) {
    return ((hdr->sect_len_1 << 8) | hdr->sect_len_2) + 3;
}

static inline uint16 pmt_get_prog_num (pmt_header_t *hdr) {
    return (hdr->prog_num_1 << 8) | hdr->prog_num_2;
}

static inline uint16 pmt_get_pcr_pid (pmt_header_t *hdr) {
    return (hdr->pcr_pid_1 << 8) | hdr->pcr_pid_2;
}

static inline uint16 pmt_get_prog_info_len (pmt_header_t *hdr) {
    return (hdr->prog_info_len_1 << 8) | hdr->prog_info_len_2;
}
#endif


// PMT elementary stream info
//   Note: care must be taken here to avoid Bus Error,
//         since there is no way to garantee data alignment.

#if _BIG_ENDIAN
typedef struct pmt_es_info_ {
    uint8 es_type;

    uint8 resv1 : 3;
    uint8 pid_hi : 5;

    uint8 pid_lo;

    uint8 resv2 : 4;
    uint8 es_info_len_hi : 4;

    uint8 es_info_len_lo;
} pmt_es_info_t;

#else

typedef struct pmt_es_info_ {
    uint8 es_type : 8;

    uint8 pid_1 : 5;
    uint8 resv1 : 3;

    uint8 pid_2;

    uint8 es_info_len_1 : 4;
    uint8 resv2 : 4;

    uint8 es_info_len_2;
} pmt_es_info_t;


static inline uint16 pmt_get_es_pid (pmt_es_info_t *info) {
    return (info->pid_1 << 8) | info->pid_2;
}

static inline uint16 pmt_get_es_info_len (pmt_es_info_t *info) {
    return (info->es_info_len_1 << 8) | info->es_info_len_2;
}
#endif

// Generic descriptor header
//
typedef struct descriptor_header_ {
    uint8 tag;
    uint8 len;
} descriptor_header_t;



// CA descriptor header
//
typedef struct ca_descriptor_header_ {
    uint8 tag;
    uint8 len;
    uint8 sys_id_hi;
    uint8 sys_id_lo;
    uint8 resv : 3;
    uint8 pid_hi : 5;
    uint8 pid_lo;
} ca_descriptor_header_t;


// Get CA system ID from CA descriptor
static inline
uint16 ca_desc_get_sys_id (ca_descriptor_header_t *desc)
{
    return (desc->sys_id_hi<<8) | desc->sys_id_lo;
}


// Set CA system ID in CA descriptor
static inline
void ca_desc_set_sys_id (ca_descriptor_header_t *desc, uint16 sys_id)
{
    desc->sys_id_hi = sys_id >> 8;
    desc->sys_id_lo = sys_id & 0xFF;
}


// Get CA PID from CA descriptor
static inline
uint16 ca_desc_get_pid (ca_descriptor_header_t *desc)
{
    return (desc->pid_hi<<8) | desc->pid_lo;
}


// Set CA PID in CA descriptor
static inline
void ca_desc_set_pid (ca_descriptor_header_t *desc, uint16 pid)
{
    desc->pid_hi = pid >> 8;
    desc->pid_lo = pid & 0xFF;
}


#endif  /* __RFGW_VIDEO_HDR_H__ */

