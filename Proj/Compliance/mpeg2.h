#ifndef __MPEG2_H__
#define __MPEG2_H__


#define TP_SIZE                         188
#define TP_SIZE_BIT                     (TP_SIZE * 8)

#define MAX_PSI_SECT_SIZE               1024
#define MAX_PRIVATE_SECT_SIZE           4096

#define SYNC_BYTE                       0x47

// Well known PIDs
#define PAT_PID                         0
#define NULL_PID                        0x1fff

#define PAT_SECT_HDR_SIZE               8
#define PMT_SECT_HDR_SIZE               12
#define PMT_ES_HDR_SIZE                 5

#define PAT_TABLE_ID                    0
#define PMT_TABLE_ID                    2

#define STUFF_BYTE                      0xff

#define CA_DESCRIPTOR_TAG               9

#define STREAM_TYPE_VIDEO_MPEG1         1
#define STREAM_TYPE_VIDEO_MPEG2         2

#define SECT_LEN_ADJ                    3
#define CRC_SIZE                        4

#define MAX_PROG_NUM                    65535

#define START_CODE_PREFIX               0x000001
#define SEQ_START_CODE                  0xB3
#define GOP_START_CODE                  0xB8
#define PIC_START_CODE                  0x00



// TP header
//
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


// Adaptation field definition (for PCR processing)
//
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

    uint16 pcr_base_1 : 16;

    uint16 pcr_base_2 : 16;

    uint16 pcr_base_3 : 1;
    uint16 resv : 6;
    uint16 pcr_ext : 9;
} af_header_t;


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
//
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


// PAT program info
//
typedef struct pat_prog_info_ {
    uint16 prog_num;
    uint16 resv : 3;
    uint16 pmt_pid : 13;
} pat_prog_info_t;


// PMT section header
//
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
} pmt_header_t;


// PMT elementary stream info
//   Note: care must be taken here to avoid Bus Error,
//         since there is no way to garantee data alignment.
//
typedef struct pmt_es_info_ {
    uint8 es_type;
    uint8 resv1 : 3;
    uint8 pid_hi : 5;
    uint8 pid_lo;
    uint8 resv2 : 4;
    uint8 es_info_len_hi : 4;
    uint8 es_info_len_lo;
} pmt_es_info_t;


// Generic descriptor header
//
typedef struct descriptor_header_ {
    uint8 tag;
    uint8 len;
} descriptor_header_t;


// PES header
//
typedef struct pes_header_ {
    uint8 sc1;                          // start code prefix (byte 1)
    uint8 sc2;
    uint8 sc3;
    uint8 stream_id;

    uint8 pes_len1;
    uint8 pes_len2;

    uint8 res1 : 2;
    uint8 pes_scramble_ctrl : 2;
    uint8 pes_priority : 1;
    uint8 data_align : 1;
    uint8 copyright : 1;
    uint8 copy_or_orig : 1;

    uint8 pts_dts_flag : 2;
    uint8 escr_flag : 1;
    uint8 es_rate_flag : 1;
    uint8 dsm_trick_mode_flag : 1;
    uint8 additional_copy_info_flag : 1;
    uint8 pes_crc_flag : 1;
    uint8 pes_ext_flag : 1;

    uint8 pes_header_data_len;
} pes_header_t;


// MPEG2 time stamp
//
typedef struct {
    uint8 marker_1 : 4;
    uint8 ts_1     : 3;
    uint8 marker_2 : 1;

    uint8 ts_2;

    uint8 ts_3     : 7;
    uint8 marker_3 : 1;

    uint8 ts_4;

    uint8 ts_5     : 7;
    uint8 marker_4 : 1;
} time_stamp_t;


// Video sequence header
//
typedef struct {
    uint32 seq_header_code;

    uint32 hori_size_value   : 12;
    uint32 vert_size_value   : 12;
    uint32 aspect_ratio_info : 4;
    uint32 frame_rate_code   : 4;

    uint32 bit_rate_value    : 18;
    uint32 marker            : 1;
    uint32 vbv_size_value    : 10;
    uint32 constrained_param : 1;
} video_seq_header_t;


// Video picture header
//
typedef struct {
    uint32 pic_header_code;

    uint32 tr        : 10;
    uint32 pic_type  : 3;
    uint32 vbv_delay : 16;
} video_pic_header_t;



extern float frame_rate[];
extern char pic_type[];



// Get ES info length from PMT section
//
static inline
uint16 es_info_get_info_len (pmt_es_info_t *es_info)
{
    return (es_info->es_info_len_hi << 8) | es_info->es_info_len_lo;
}


// Get ES PID from PMT section
//
static inline
uint16 es_info_get_pid (pmt_es_info_t *es_info)
{
    return (es_info->pid_hi << 8) | es_info->pid_lo;
}


// Set ES PID in PMT section
//
static inline
void es_info_set_pid (pmt_es_info_t *es_info, uint16 pid)
{
    es_info->pid_hi = pid >> 8;
    es_info->pid_lo = pid & 0xFF;
}


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


// Get CA PID from CA descriptor
static inline
uint16 ca_desc_get_pid (ca_descriptor_header_t *desc)
{
    return (desc->pid_hi<<8) | desc->pid_lo;
}


// SET CA PID in CA descriptor
static inline
void ca_desc_set_pid (ca_descriptor_header_t *desc, uint16 pid)
{
    desc->pid_hi = pid >> 8;
    desc->pid_lo = pid & 0xFF;
}


#endif  /* __MPEG2_H__ */

