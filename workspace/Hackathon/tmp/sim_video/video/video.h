/*
 * Copyright (c) 2005-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "common.h"
#include "string.h"
#include "que.h"
#include "pool.h"
#include "hash.h"
#include "util.h"
#include "logger.h"
#include "phy_mem.h"
#include "thread_stat.h"
#include "trace.h"
#include "ipc.h"
#include "video_def.h"
#include "video_messages.h"
#include "video_hdr.h"
#include "video_event.h"
#include "video_ca.h"


// Messages to platform trace debug
//
extern void video_print_info_log(const char *format, ...);

#define VIDEO_LOG(fmt, str...) \
        logger_printf(video_log, fmt "\n", ##str); \
        video_print_info_log(fmt, ##str); \

// Debugging messages
//
#define VIDEO_DEBUG(fmt, str...)                                \
            logger_printf(video_log, fmt, ##str);

#define VIDEO_MSG_DEBUG(fmt, str...)                            \
            if (is_trace_enabled(1)) {                          \
                logger_printf(video_log, fmt, ##str);           \
            }

#define VIDEO_EVENT_DEBUG(fmt, str...)                          \
            if (is_trace_enabled(2)) {                          \
                logger_printf(video_log, fmt, ##str);           \
            }

#define VIDEO_IN_DEBUG(fmt, str...)                             \
            if (is_trace_enabled(3)) {                          \
                logger_printf(video_log, fmt, ##str);           \
            }

#define VIDEO_OUT_DEBUG(fmt, str...)                            \
            if (is_trace_enabled(4)) {                          \
                logger_printf(video_log, fmt, ##str);           \
            }

#define VIDEO_PSI_DEBUG(fmt, str...)                            \
            if (is_trace_enabled(5)) {                          \
                logger_printf(video_log, fmt, ##str);           \
            }

#define VIDEO_CLKREC_DEBUG(fmt, str...)                         \
            if (is_trace_enabled(6)) {                          \
                logger_printf(video_log, fmt, ##str);           \
            }

#define VIDEO_THREAD_DEBUG(fmt, str...)                         \
            if (is_trace_enabled(7)) {                          \
                logger_printf(video_log, fmt, ##str);           \
            }

#define VIDEO_CA_DEBUG(fmt, str...)                             \
            if (is_trace_enabled(8)) {                          \
                logger_printf(video_log, fmt "\n", ##str);      \
            }


#define BASE_FREQ               45000
#define EXT_FREQ                27000000
#define SCALE_BASE_EXT          600
#define SCALE_MS_BASE           45

#define IP_PKT_BUF_SIZE         2048
                                // Note: This size is used to avoid having
                                // a IP buffer accross the 4k byte boundaries.
                                // PCI to Cobia has the requirement!

#define MAX_MPEG_PKT_SIZE       256

#define MAX_MPTS_PER_QAM        8
#define MAX_SPTS_SESSIONS       MAX_SESSIONS
#define MAX_MPTS_SESSIONS       (MAX_MPTS_PER_QAM * NUM_QAMS)

#define MAX_PIDS_PER_PROG       32
#define MAX_PIDS_PER_SPTS       (MAX_PIDS_PER_PROG + 1)
                                        // + 1 to include PAT PID

#define MAX_PROGS_PER_MPTS      MAX_VIDEO_FLOWS_PER_QAM
#define MAX_PIDS_PER_MPTS       (MAX_PROGS_PER_MPTS + 1)
                                        // + 1 to include PAT PID
                                        // Assumes passthru only.  Not remux!

#define NUM_IN_PROGS            MAX_SESSIONS
#define NUM_OUT_PROGS           MAX_SESSIONS

#define MAX_PCR_CONTEXT_SPTS    4
#define MAX_PCR_CONTEXT_MPTS    64

#define MAX_LATENCY             (SYS_LATENCY + MAX_JITTER + 2 * BUF_HEADROOM)

#define NUM_PSI_SECTIONS        (MAX_SESSIONS * 10)
#define NUM_PRIVATE_SECTIONS    MAX_SESSIONS

#define NUM_PSI_TP_BUFS         ((NUM_QAMS + NUM_PSI_SECTIONS) * 2)

#define NUM_CRSLS               (MAX_AVG_CAROUSEL_PER_QAM * NUM_QAMS)
#define NUM_CRSL_INSERTS        (MAX_AVG_CAROUSEL_INSERT_PER_QAM * NUM_QAMS)
#define NUM_CRSL_TP_BUFS        MAX_CAROUSEL_TP_PER_LC

#define QAM_MASK_SZ             ((NUM_QAMS + 31) >> 5)

#define IP_FLOW_BUF_SIZE        512

#define PSI_PERIOD_DEFAULT      200
                                // in ms

#define INVALID_ID              (-1)
#define INVALID_SES_ID          0xFFFF
#define INVALID_CRSL_ID         0xFFFF
#define INVALID_QAM_ID          0xFFFF
#define INVALID_GROUP_ID        0xFFFF

#define PSI_VER_UNKNOWN         0xFF
#define PSI_VER_CURRENT         1
#define PSI_VER_NEXT            0

#define FRAC_BITS_CLKREC        24
#define FRAC_BITS_TIME          20
#define OUT_TIME_SHIFT          (32 - FRAC_BITS_TIME)
#define OUT_TIME_MASK           ((1 << FRAC_BITS_TIME) - 1)

#define MAX_PCR_ITVL            (300 * SCALE_MS_BASE)
                                // in 45 kHz ticks

#define SCALE_FREQ_ADJ          17592186
                                // 1e-6 in F.44

#define SLEW_CONST              3036
                                // to scale slew rate in ppm/hr to F0.44

#define CC_DISC                 0xff
#define INVALID_PID             0xffff
#define INVALID_INFO_IDX        0xffff


// scheduling window sizes (in 45 kHz ticks)
#define SCH_WIN                 (SCH_WIN_MS * SCALE_MS_BASE)
#define PAST_SCH_WIN            (PAST_SCH_WIN_MS * SCALE_MS_BASE)

// OUTCMDS_PER_QAM is provided via Makefile
// It is set to per channel maximum hardware allows
#define OUTCMDS_PER_FLOW        OUTCMDS_PER_QAM

#define MAX_BITRATE_ALLOC       52000000



// Info Flag
#define TP_INFO         0x0
#define PSI_USAGE       0x1
#define NEW_PCR_INFO    0x2
                        // for unified CR mode only
#define SLAVE_PCR_INFO  0x2
                        // for master-slave CR mode only
#define PCR_INFO        0x3


// Config types
typedef in_session_config_t in_config_t;
typedef out_session_config_t out_config_t;


// Video performance statistics
typedef struct {
    uint32 now;
    int32 avg_wake_itvl;
    int32 max_wake_itvl;
} video_perf_t;


#if 0
// IP packet info
typedef struct {
    uintptr_t addr;
    uint32 arvl_time;
} ip_pkt_info_t;
#endif


// TP info
typedef struct tp_info_ {
    // Word 1
    uint32 info_type   : 2;     // should be TP_INFO
    uint32 pcr_flag    : 1;     // set if the TP carries PCR
    uint32 disc_flag   : 1;     // discontinuity flag
    uint32 pat_flag    : 1;     // set if this TP carries PAT
    uint32 pmt_flag    : 1;     // set if this TP carries PMT
    uint32 idle_flag   : 1;     // set in 1st TP after session resume from idle
    uint32 unused_flag : 1;     // unused flag

    uint32 usage_id    : 4;     // for TP Info array overrun detection
    uint32 arvl_time   : 20;    // arrival time in 45 kHz ticks

    // Word 2
    uint16 pid;                 // input PID
    uint16 tp_idx;              // TP index

    // Word 3 (or 3-4 for 64-bit system)
    uintptr_t addr;             // TP address (physical)
} tp_info_t;


// PCR info
// Note: this structure is shared for PCR_INFO (unified cr mode),
//       and MASTER_PCR_INFO and SLAVE_PCR_INFO (master-slave cr mode)
typedef struct {
    // Word 1
    uint32 info_type   : 2;     // should be PCR_INFO or SLAVE_PCR_INFO
    uint32 out_time    : 20;    // out time (in 45 kHz ticks)
                                // (least significant 20 bits)
    uint32 tbo_ext     : 10;    // time base offset - 27 MHz ext part

    // Word 2
    uint32 tbo_base;            // time base offset - 45 kHz base part (32 bit)

    // Word 3
    uint32 valid_flag  : 1;     // whether tp_itvl has been updated with
                                // next PCR value (not used for SLAVE_PCR_INFO)
    uint32 tp_itvl     : 31;    // TP interval - 45 kHz clock ticks in F10.20
                                // Note: For debugging, we store PID here
                                //       for slave PCR
} pcr_info_t;


// New PCR info
typedef struct {
    // Word 1
    uint32 info_type   : 2;     // should be NEW_PCR_INFO
    uint32 out_time    : 20;    // out time (in 45 kHz ticks)
    uint32 pcr_ext     : 10;    // time base offset - 27 MHz ext part

    // Word 2
    uint32 pcr_base;            // time base offset - 45 kHz base part (32 bit)

    // Word 3
    uint8 pcr_ctx_idx;          // PCR context index
    uint16 link;                // tp_info index of next new PCR TP
} new_pcr_info_t;


#define PSI_NOT_READY_USAGE_ID 0


// PSI info
typedef struct {
    // Word 1
    uint8 info_type    : 2;    // should be PSI_USAGE
    uint8 pat_flag     : 1;    // set if this TP carries PAT
    uint8 pmt_flag     : 1;    // set if this TP carries PMT
    uint8 unused_flag  : 4;    // this flag is unused
    
    uint8 psi_usage_id;        // id to check for psi changes
    uint16 prog_num;           // program number if it is a pmt change
    
    // Word 2
    uint16 pid; 
    uint16 unused_1;

    // Word 3
    uint32 unused_2;
} psi_info_t;


#define INFO_FLAGS(tp_info)     (*((uint8*)(tp_info)))


// TP info buffer
#define NUM_TP_INFO_BUF_CHUNKS  (MAX_SESSIONS * 4)

#define TP_INFO_CHUNK_SIZE      2048
                                // corresponds to ~12 Mbps for
                                // up to 250 ms of system latency
#define TP_INFO_CHUNK_MASK      0x7FF
#define TP_INFO_CHUNK_SHIFT     11
#define TP_INFO_MAX_CHUNKS      5
                                // up to 5 chunks for 52 Mbps session

// TP info bufer
typedef struct {
    uint32 size;                // total buffer size in tp_info count
    tp_info_t* chunk[TP_INFO_MAX_CHUNKS];
} tp_info_buffer_t;

// Reference to TP info bufer
typedef struct {
    tp_info_buffer_t *buf;
    tp_info_t *chunk;
    uint32 idx;
} tp_info_ref_t;


// MPEG-2 clock
//   Note: the clock is defined in slightly different way as in MPEG-2 spec.
//         It is more SW friendly.  It consists of a 45 kHz clock tick
//         base part, and a 27 MHz extension part from 0 to 599.
//
typedef struct mpeg_clock_ {
    uint32 base;                // in 45 kHz clock ticks
    int16  ext;                 // in 27 MHz clock ticks, from 0 to 599
} mpeg_clock_t;


#define MPEG_TIME_RANGE         ( ((int64)SCALE_BASE_EXT) << (32 + 20) )

typedef int64 mpeg_time_t;      // mpeg time format in ext F42.20
                                // value should be in [0, MPEG_TIME_RANGE - 1]

typedef int64 mpeg_timediff_t;  // mpeg time difference format in ext F42.20
                                // value should be in
                                // [-MPEG_TIME_RANGE/2, MPEG_TIME_RANGE/2 - 1]

enum {
    VIDEO_CR_MODE_UNIFIED_VBR,
    VIDEO_CR_MODE_UNIFIED_CBR,
    VIDEO_CR_MODE_MASTER_SLAVE,
    VIDEO_CR_MODE_MAX = VIDEO_CR_MODE_MASTER_SLAVE
};

// Clock Recovery Parameters
// Note: all parameters are scaled by (1 << FRAC_BITS_CLKREC)
//
typedef struct clkrec_par_ {
    int32 lpf_par;
    int32 loop_gain;
    int32 max_slew;
    int64 max_freq_adj;

    int32 review_period;        // # monitor periods for drift review
                                // (0 to turn off, must be <= 255)
    int32 drift_thres;          // threshold to trigger drift adjustment
                                // (in base ticks)
    int64 adjust_thres;         // adjustment threshold (in base ticks)

} clkrec_par_t;


typedef struct psi_section_ {
    que_elem_t link;
    uint32 crc;
    psi_section_header_t sect_hdr[0];
} psi_section_t;


// Generic PSI snoop info
//
typedef struct psi_snoop_ {
    int16 snooped_len;          // length of current section snooped
    int16 tp_byte_left;         // remaining # bytes in current TP
    psi_section_t* sect;        // points to section buffer
    uint8* tp_ptr;              // TP read pointer (should be NULL
                                // the first time a TP is snooped)
    psi_section_t* psi_buf;     // preallocated buffer for PSI section
    psi_section_t* priv_buf;    // preallocated buffer for private section
} psi_snoop_t;


// PAT info
//
#define MAX_PAT_SECT            4
#define MAX_PROGS_PER_TS        MAX_PROGS_PER_MPTS

typedef struct pat_info_ {
    uint8 ver;                  // version number
    uint8 sect_cnt;             // number of PAT sections
    uint8 sect_snooped;         // number of sections already snooped
    uint8 prog_cnt;             // number of programs
    uint16 tsid;                // transport stream ID
    uint16 nit_pid;             // NIT PID

    psi_section_t *sect[MAX_PAT_SECT]; // point to PAT section buffers
    pat_prog_info_t *prog_info[MAX_PROGS_PER_TS];
} pat_info_t;


typedef struct {
    uint16 processed_flag : 1;  // item has been processed before
                                // (should only be used in ca_desc)
    uint16 offset : 15;         // offset within PMT section
    uint16 orig_pid;            // input PID value (for PMT in out session)
} pmt_item_t;


// PMT info
//
#define MAX_ES_PER_PROG         (MAX_PIDS_PER_PROG - 1)
#define MAX_CA_DESC_PER_PROG    32 

typedef struct pmt_info_ {
    uint8 ver;                  // version number
    uint8 es_cnt;               // number of elementary streams in PMT
    uint8 ca_desc_cnt;          // number of CA descriptors in PMT
    uint16 pcr_pid;             // PCR PID
    uint16 orig_pcr_pid;        // original PCR PID (for output PMT only)
                                //   TODO: Need to set in output proc
    psi_section_t* sect;        // points to section buffer
                                // (Note: PMT can only have one section)

    pmt_item_t es[MAX_ES_PER_PROG];
    pmt_item_t ca_desc[MAX_CA_DESC_PER_PROG];
} pmt_info_t;


// PSI carousel
typedef struct {
    uint8 num_tp;
    uint8 used_flag : 1;
    uint8 next_cc   : 7;
    uint16 period;
    uint32 next_time;
    que_elem_t tp_list;
} psi_crsl_t;


// ECM carousel
typedef struct {
    que_elem_t link;
    uintptr_t ecm_info_index;
    uint8 next_cc;
    uint16 period;
    uint32 next_time;
} ecm_crsl_t;


// Carousel
// (for PAT/PMT regeneration, and GQI packet insertion support)
//
typedef struct crsl_ {
    uint32 resource_id;         // resource ID from client
    uint8 owner_id;
    uint8 used_flag : 1;        // whether the carousel is in use
    uint8 num_tp    : 7;
    uint8 num_qam;
    int32 insert_count;         // -1 means continuous insersion
    int32 insert_period;        // in ms
    que_elem_t tp_list;
    uint32 qam_mask[QAM_MASK_SZ];       // not used in PSI carousel
} crsl_t;


// Carousel insert
//
#define CRSL_INSERT_CONTINUOUS  0xFFFFFFFF

typedef struct crsl_insert_ {
    que_elem_t link;            // not used in PSI carousel

    void *crsl;                 // (crsl_t *) for PSI or GQI packet insertion
                                // uintptr_t ecm_info_index for ECM insertion

    uint8 ecm_flag;             // set if this insert is for ECM
    uint8 next_cc;
    uint32 period;              // number of base ticks between insertions
    uint32 target_cnt;          // target number of time to insert,
                                //   or CRSL_INSERT_CONTINUOUS
    uint32 cnt;                 // number of time carousel has been inserted
    uint32 next_time;           // next time to insert
} crsl_insert_t;


// Input program info
//
typedef struct in_prog_ {
    que_elem_t link;
    uint16     prog_num;        // program number
    uint16     pmt_pid;         // PMT PID
    uint8      pmt_ver;         // PMT version
    uint8      pmt_usage_id;

    // PMT snooping
    uint8 pmt_snooped;          // whether PMT has been snooped
    psi_snoop_t snoop;          // snoop context info
    pmt_info_t pmt;             // PMT info
    que_elem_t priv_sect_list;  // list of private sections in PMT PID
    crsl_t priv_crsl;           // carousel for private sections
} in_prog_t;


// Input PID info
typedef struct {
    uint8 seen : 1;             // if this PID has been seen
    uint8 has_psi : 1;          // if this is a PAT or PMT PID
    uint8 has_pcr : 1;          // if this is a PCR PID
    uint8 referenced : 1;       // if this PID is referenced by PSI
    uint8 prev_cc : 4;
} in_pid_info_t;


// Output PID info
typedef struct {
    uint16 reversed : 1;        // reversed the defaulted select decision
    uint16 pid_reversed : 1;    // reversed due to PID filtering/selection
    uint16 prog_reversed : 1;   // reversed due to program filtering/selection
    uint16 pid : 13;            // output PID - for remapped session only
} out_pid_info_t;


// PCR context
//
#define PCR_CONTEXT_FLAG_LOCKED         0x01
                                        // timebase is locked
#define PCR_CONTEXT_FLAG_BITRATE_KNOWN  0x02
                                        // bitrate known
#define PCR_CONTEXT_FLAG_ADJUSTED       0x04
                                        // PLL has been adjusted

typedef struct {
    uint8 flags;
    uint8 last_seen;            // wakeup which the PCR is last seen
                                // (bit 15 to 8 of sys_clk.base)
    uint16 pid;                 // PID
    uint32 tp_idx;              // TP index of previous PCR

    mpeg_time_t pcr_time;       // input PCR (ext F42.0)
    mpeg_time_t out_time;       // PCR output time (ext F42.20)
    mpeg_timediff_t tbo_est;    // estimated TBO (ext F42.20)
    int64 drift;                // time base drift error (ext F.40)

    // PLL
    int64 tbo_err_lpf;          // low-pass filtered TBO (ext F.20)
    int64 freq_adj;             // clock frequency adjust (F-12.44)

    // Statistics
    mpeg_time_t prev_arvl_time; // previous PCR arrival time
    mpeg_timediff_t max_pcr_inc; // max PCR interval

    int32 tp_itvl;              // TP interval (base F.20)
} pcr_context_t;


// Input session info
//

// Note: the following 5 flags are shared between in_session and out_session
#define SESSION_FLAG_PID_REMAP          0x01
#define SESSION_FLAG_PROCESS_TIME       0x02
#define SESSION_FLAG_PROCESS_PSI        0x04
#define SESSION_FLAG_CBR                0x08
#define SESSION_FLAG_MPTS               0x10

#define SESSION_FLAG_GO_ACTIVE          0x20
                                        // session is going active
#define SESSION_FLAG_SET_DISC           0x40
                                        // disc point is to be inserted
#define SESSION_FLAG_STATE_CHANGED      0x80
                                        // if there is unreported state change

#define SESSION_FLAGS_SHARED            0x1F
                                        // mask of shared flags between
                                        // in_session_t and out_session_t

#define PROCESS_TYPE_REMAP              1
#define PROCESS_TYPE_REMUX              2
#define PROCESS_TYPE_PASSTHRU           3
#define PROCESS_TYPE_DATA               4

typedef struct in_session_ {
    uint8 state;                // session state (SESSION_STATE_XXX
                                // defined in video_messages.h)
    uint8 flags;                // session flags (SESSION_FLAG_XXX)
    uint8 type;                 // processing type (PROCESS_TYPE_XXX)
    uint8 tp_info_usage_id;     // for TP info array overrun detection
    uint16 id;                  // session ID
    uint16 anchor_info_idx;     // index of pcr_info for last processed anchor
    uint32 tp_idx;              // TP index for the session
    uint32 prev_tp_arvl;        // previous TP arrival time

    in_config_t *cfg;           // session's configuration

    tp_info_buffer_t tp_info_buf; // TP info buffer reference
    tp_info_ref_t tp_info_ref;  // TP info buffer reference

    uint32 tp_info_buf_wrap_time;

    uint32 arvl_time;           // arrival time of latest IP packet
    ip_header_t *ip_hdr;        // IP header of latest packet

    que_elem_t prog_list;       // list of in_prog_t

    uint32 init_thres;          // init threashold (base)
    uint32 idle_thres;          // idle threashold (base)
    int32 off_timer;            // off timer (base)
    uint32 src_ip[4];           // source IP (used for collision detection)

    uint32 anchor_arvl;         // anchor arrival time (base part)

    uint32 ref_tp_info_idx;     // tp_info ref idx for read/write sync

    uint16 group_id;            // session group ID (used by PD code)
    int8 mon_id;                // monitor id: 0 to 15, or -1 if not used
    uint8 bitrate_exceeded : 1; // BITRATE_EXCEEDED event sent

    // PAT snooping
    uint8 pat_snooped;          // whether PAT has been snooped
    uint8 pat_ver;              // current PAT version number
    uint8 pmt_snooped_cnt;      // number of PMTs that have been snooped
    uint8 pat_usage_id;
    psi_snoop_t snoop;          // snoop context info
    pat_info_t pat;             // PAT info

    // Clock recovery
    pcr_context_t *pcr_ctx_tab; // PCR context table
    uint8 max_pcr_ctx;          // max number of PCR contexts in PCR ctx table
    uint8 pcr_ctx_cnt;          // number of PCR contexts used
    int8 locked_pcr_cnt;        // number of locked PCR contexts
    int8 periods_till_review;   // number of monitor periods before next review
    uint16 first_new_pcr;       // link to 1st new PCR info (using info idx)
    uint16 last_new_pcr;        // link to last new PCR info (using info idx)
    int64 anchor_out_time;      // anchor PCR out time (ext F42.20)
    mpeg_clock_t pcr;           // current PCR value
    mpeg_timediff_t out_tbo_adj;// TBO offset adjust (ext F.20)
    mpeg_time_t jitter;         // estimated input jitter
    mpeg_time_t max_jitter;     // max jitter allowed
    int32 stay_time_limit;      // max TP stay time for the session (base ticks)
    uint32 anchor_tp_idx;       // last PCR's TP index
    int32 tp_itvl;              // TP interval (base F10.20)
    int32 stay_time;            // current stay time (base F.0)
    int32 tp_itvl_lower;        // lower bound of TP interval
                                // (based on bitrate_alloc + 5%)
    int32 tp_itvl_upper;        // upper bound of TP interval
                                // (based on measured bitrate - 5%)

#if COMP_DRIFT_RATE
    int32 drift_rate;           // output PCR drift rate (for debugging only)
#endif

    // Statistics
    video_in_stat_t stat;       // in session statistics
    int32 out_ses_cnt;          // number of output sessions
    int32 min_tp_itvl;          // min TP interval (base F.20)
    int32 max_tp_itvl;          // max TP interval (base F.20)
    int32 avg_tp_itvl;          // avg TP interval (base F.20)
    uint32 prev_bitrate_update_time; // base of mpeg clock
    uint32 prev_tp_cnt;         // tp count of previously calcluated bitrate
    uint32 min_bitrate;         // minimum bitrate
    uint32 max_bitrate;         // maximum bitrate
    uint32 avg_bitrate;         // average bitrate
    uint32 src_ip_change_cnt;   // number of times source IP changes

    video_perf_t perf;

    // PID table
    in_pid_info_t pid_tab[NUM_PIDS];
}
#ifdef CACHE_SIZE
__attribute__((__aligned__(CACHE_SIZE)))        // force cache-line alignment
#endif
in_session_t;


// Session hash table structures
//
typedef struct {
    uint32 dst_ip[4];           // to supprt ipv6 need 128 bit addresses
    uint32 extra[4];            // dst_udp for unicast, src_ip for multicast
                                // Note: for IPv4, only [0] is used
    uint8 ip_ver;               // IP version
    uint8 mcast_flag;           // multicast flag
} ses_hash_key_t;


typedef struct {
    que_elem_t link;            // required
    uint32 hash_code;           // required
    ses_hash_key_t key;         // session key
    uint16 cast_type;           // cast type: unicast / asm / ssm
    uint16 flow_rd;             // read pointer of IP flow
    uint16 flow_wr;             // write pointer of IP flow
    ip_pkt_info_t* info;        // pointer to current IP packet info
} ses_hash_item_t;


// QAM program info
typedef struct {
    uint16 used : 1;            // whether the program number is used in the qam
    uint16 out_prog_id : 15;    // id of out_prog_t in the pool
} qam_prog_info_t;


// QAM PID info
typedef struct {
    uint16 used : 1;            // if PID is used (as referenced pid) in qam
    uint16 forwarded : 1;       // if the PID is forwarded from an input
                                // i.e., not be set for inserted pids
    uint16 out_ses_id : 14;     // id of out_session_t in the pool
    uint16 cw_idx : 15;         // codeword index
    uint16 scrambled : 1;       // if PID is scrambled
} qam_pid_info_t;


// Output program info
// - in_prog, in_prog_num, in_pmt_pid are set up in out_session_update_XXX_pat()
// - prog_num, pmt_pid, pmt_ver, pmt_usage_id are set up in regenerate_pmt()
//   or passthru_pmt()
//
#define INVALID_CFG_IDX         255

typedef struct out_prog_ {
    que_elem_t link;

    in_prog_t *in_prog;         // point to corresponding in_prog_t
    uint16 prog_num;            // program number
    uint16 pmt_pid;             // PMT PID
    uint16 in_prog_num;         // input program number
    uint16 in_pmt_pid;          // input PMT PID

    uint8  pmt_ver;             // PMT version
    uint8  pmt_usage_id;

    uint8  cfg_idx;             // matching config index
    uint8  present : 1;         // program present in new PAT?


    uint8  filtered : 1;        // program is filtered
    uint8  blocked  : 1;        // conflict detected (passthru)
                                // or no output pid available (remapped)

    uint8  cfg_changed  : 1;    // config changed
    uint8  ses_reg_prog : 1;    // need to register PMT pid in session
    uint8  ses_reg_pid  : 1;    // need to register referenced pids in session
    uint8  qam_reg_prog : 1;    // need to register program/PMT pid in QAM
    uint8  qam_reg_pid  : 1;    // need to register reference pids in QAM

    uint16 out_ses_id;          // corresponding out sessino ID

    // PMT regeneration
    uint8  on_air    : 1;       // program is on
    uint8  pmt_built : 1;
    pmt_info_t pmt;
    psi_crsl_t pmt_crsl;
    crsl_insert_t priv_insert;  // carousel insert for private sections
                                // (carousel stored in in_prog)
    ca_descriptor_header_t  ca_desc_data;
    uint16  ca_desc_loc;
} out_prog_t;


// Output session info
//

// Note: the following 5 flags are copied from corresponding in session
#define SESSION_FLAG_PID_REMAP          0x01    
#define SESSION_FLAG_PROCESS_TIME       0x02    
#define SESSION_FLAG_PROCESS_PSI        0x04
#define SESSION_FLAG_CBR                0x08
#define SESSION_FLAG_MPTS               0x10

#define SESSION_FLAG_TP_FOUND           0x20
#define SESSION_FLAG_PAT_BUILT          0x40
                                        // whether the session's programs
                                        // have been included in the qam's PAT
#define SESSION_FLAG_MASTER_SLAVE_CR    0x80

#define INVALID_FLOW_ID                 0x3F

typedef struct out_session_ {
    uint8 state;                // session state
    uint8 flags;                // session flags

    uint8 include_prog_flag;    // if session has programs to be included
    uint8 exclude_prog_flag;    // if session has programs to be excluded

    uint16 qam_id  : 10;        // QAM ID
    uint16 flow_id : 6;         // flow ID

    uint16 id;                  // session ID
    uint16 tp_idx;              // TP index

    uint16 anchor_info_idx;     // pcr_info index of last input anchor processed
    uint16 anchor_tp_idx;       // last master PCR TP index

    int16 cmdbuf_avail;         // available output command buffers (estimated)

    in_session_t *in_ses;       // points to input session

    tp_info_ref_t tp_info_ref;  // TP info buffer reference

    uint32 tp_itvl;             // output TP interval (base F.20)
    uint32 out_time;            // delivery time (least 20 bits of base)

    uint8 prev_traffic_state;   // previous in session traffic state

    uint8 tp_info_usage_id;
    uint8 pat_usage_id;
    uint8 pmt_generated_cnt;    // number of PMTs that have been generated

    int32 stay_time_limit;      // max TP stay time for the session

    out_config_t *cfg;          // session's configuration
    que_elem_t prog_list;       // list of out_prog_t

    // Statistics
    video_out_stat_t stat;
    int32 min_stay_time;        // in 45 kHz ticks
    int32 max_stay_time;        // in 45 kHz ticks
    uint32 prev_tp_cnt;         // tp count of previously calcluated bitrate
    uint32 min_bitrate;         // minimum bitrate
    uint32 max_bitrate;         // maximum bitrate
    uint32 avg_bitrate;         // average bitrate

    // New PID table
    out_pid_info_t pid_tab[NUM_PIDS];
} 
#ifdef CACHE_SIZE
__attribute__((__aligned__(CACHE_SIZE)))        // force cache-line alignment
#endif
out_session_t;


// Ring Buffer
//
typedef struct ring_buf_ {
    uintptr_t *buf;             // buffer's start address (virtual)
    int16 rd;                   // read index (of next item to read)
    int16 wr;                   // write index (of next available item)
                                // Note: rd == wr means buffer is empty
} ring_buf_t;

#if 0
// DMA Ring Buffer
//
typedef struct {
    uintptr_t *src_buf;         // source buffer start address
    uintptr_t *dst_buf;         // destination buffer start address
    uint16 rd;                  // read index (to data to be consumed next)
    uint16 wr;                  // write index (where new data will be stored)
    uint16 tx;                  // transmit index (where next dma will start)
} dma_ring_buf_t;
#endif


// QAM info
//
#define PSI_FLOW_ID             (FLOWS_PER_QAM - 2)
                                // flow id used for PAT/PMT insertion

#define CRSL_FLOW_ID            (FLOWS_PER_QAM - 1)
                                // flow id used for carousel packet insertion

#define QAM_FLAG_READY          0x1

typedef struct {
    uint16 prog_num;
    uint16 ver;
} pmt_ver_t;

typedef struct qam_info_ {
    qam_config_t *cfg;          // qam config
    uint8 flags;
    uint8 prog_cnt;             // number of output programs
    uint16 id;                  // QAM id
    uint16 group_id;            // QAM group ID (used by PD code)

    uint8 pat_rebuild : 1;      // need to update PAT?
    uint8 recheck_conflict : 1; // need to recheck conflict?
    uint8 monitored : 1;        // qam monitoring

    // PAT generation
    uint8 pat_built : 1;        // whether PAT is built
    uint32 pmt_next_insert_time;
    pat_info_t pat;
    psi_crsl_t pat_crsl;
    pat_prog_info_t prog_info[MAX_PROGS_PER_TS];

    uint16 cmdbuf_avail[FLOWS_PER_QAM];

    // PMT version history
    uint32 pmt_ver_idx;         // for next pmt_ver_history entry to be used
    pmt_ver_t pmt_ver_history[MAX_VIDEO_FLOWS_PER_QAM];

    // Statistics
    video_qam_stat_t stat;

    uint32 prev_bitrate_update_time; // base of mpeg clock
    uint32 prev_tp_cnt;         // tp count of previously calcluated bitrate
    uint32 min_bitrate;         // minimum bitrate
    uint32 max_bitrate;         // maximum bitrate
    uint32 avg_bitrate;         // average bitrate

    video_perf_t perf;

    // New PID and program tables
    qam_prog_info_t prog_tab[NUM_PROGS];
    qam_pid_info_t pid_tab[NUM_PIDS];
}
#ifdef CACHE_SIZE
__attribute__((__aligned__(CACHE_SIZE)))        // force cache-line alignment
#endif
qam_info_t;


// Video context
//
typedef struct video_context_ {
    uint8 id;

    // Counters
    uint32 in_session_cnt;
    uint32 out_session_cnt;
    uint32 qam_cnt;
    uint32 crsl_cnt;

    hash_table_t *ses_hash_tab[NUM_INPUT_PORTS];
    ses_hash_item_t ses_hash_item[MAX_SESSIONS];

    // Input session configs
    in_config_t in_session_config[MAX_SESSIONS];

    // Output session configs
    out_config_t out_session_config[MAX_SESSIONS];

    // QAM configs
    qam_config_t qam_config[NUM_QAMS];

    // QAM flow map: mapping from flow to in_session_id and out_session_id
    // Note: -1 means the flow not used.
    //       Otherwise, the upper 16-bit is the in session id,
    //       Lower  16-bit is the out session id.
    //
    int32 qam_flow[NUM_QAMS][MAX_VIDEO_FLOWS_PER_QAM];

    // Carousel (for GQI Packet Insertion only)
    crsl_t crsl[NUM_CRSLS];             // carousel array

    // Carousel insert
    que_elem_t crsl_insert_list[NUM_QAMS];  // list of crsl insert in use
    crsl_insert_t crsl_insert[NUM_CRSL_INSERTS];

    // Carousel TP buffers
    uint32 crsl_tp_buf_offset;          // carousel TP buffer address offset

    // Pools
    pool_t crsl_insert_pool;            // carousel insert pool
    pool_t crsl_tp_buf_pool;            // carousel TP buffer pool
    pool_t in_ses_id_pool;              // for new VSCM support

    // Platform specific configs
    void *platform;

} video_context_t;

extern video_context_t *video_ctx;


// Video IP statistics
//
typedef struct {
    uint32 ipv4_cnt;            // IPv4 packets
    uint32 ipv6_cnt;            // IPv6 packets
    uint32 ucast_cnt;           // unicast packets
    uint32 mcast_cnt;           // multicast packets
    uint32 err_cnt;             // packets with error
    uint32 unref_cnt;           // unreferenced packets (session not found)
    uint32 overrun_cnt;         // IP flow buffer overruns
} video_ip_stat_t;


// PID statistics (for debugging only)
typedef struct {
    uint32 tp_cnt;
    uint32 cc_err_cnt;
    uint8 prev_cc;
} pid_stat_t;


// PSI record (for debugging only)
typedef struct {
    char filename[128];
    int fd;
    uint16 in_id;
    uint32 max_tp;
    uint32 tp_cnt;
    boolean rec_flag;
} psi_record_t;


// Global variables
extern video_context_t *video_context[];
extern in_session_t *in_session;
extern out_session_t *out_session;
extern qam_info_t *qam_info;
extern ip_pkt_info_t *ip_flow_buf;

extern pid_t my_pid;
#ifdef __QNX__
extern ipc_port_info *video_control_port_info;
#endif

// TP buffer address offset
extern uint32 input_buf_offset[NUM_INPUT_PORTS];
extern uint32 psi_tp_buf_offset;
extern uint32 crsl_tp_buf_offset;

// System clock
extern mpeg_clock_t sys_clk;
extern clkrec_par_t clkrec_par;

// Pools
extern pool_t in_prog_pool;
extern pool_t out_prog_pool;
extern pool_t psi_section_pool;
extern pool_t private_section_pool;
extern pool_t tp_info_buf_chunk_pool;   // TP info buffer chunk pool
extern pool_t pcr_context_spts_pool;    // SPTS PCR context table pool
extern pool_t pcr_context_mpts_pool;    // MPTS PCR context table pool
extern pool_t psi_tp_buf_pool;          // TP buffer pool for output PSI

extern logger_t *video_log;

extern uint32 out_time_limit;

// Stats
#define SESSION_STATE_STAT_ERR_UPDATE   0x80
                                        // error stat to be updated

extern video_in_stat_t video_in_stat_history;
extern video_out_stat_t video_out_stat_history;

// LCRED
extern int lcred_primary_id;

// LC-CLI
extern uint32 cli_context_id;
extern video_context_t *cli_video_ctx;
extern boolean disable_input_thread;

// Monitor
extern int mon_period;
extern int ses_mon_wake_cnt;
extern uint32 mon_time;
extern out_session_t *mon_out_ses;

// Message counts
extern uint32 msg_cnt_config_qam;
extern uint32 msg_cnt_add_session;
extern uint32 msg_cnt_delete_session;
extern uint32 msg_cnt_delete_session_list;
extern uint32 msg_cnt_delete_owner;
extern uint32 msg_cnt_change_source;
extern uint32 msg_cnt_update_pid_filter;
extern uint32 msg_cnt_update_prog_filter;
extern uint32 msg_cnt_update_pid_remap;
extern uint32 msg_cnt_update_prog_remap;
extern uint32 msg_cnt_add_crsl;
extern uint32 msg_cnt_delete_crsl;

// Misc
extern mpeg_timediff_t hw_time_adj;
extern boolean rec_on;
extern uint32 rec_in_sid;
extern boolean monitor_platform_flag;
extern boolean session_state_update_flag;

// Video packet Capture
extern int capture_mode;
extern boolean capture_flag;
extern uint8 capture_buf[IP_PKT_BUF_SIZE];
extern psi_record_t psi_record;

// For testing only
extern boolean disc_insert_enable;


// Function prototypes
//

// Generic video functions
int video_config_init(void);
int video_state_init(void);
void video_go_active (int primary_id);
void video_prepare_go_hot(int primary_id);
void video_go_hot(int primary_id);
void video_monitor_in_session(in_session_t *ses);
void video_monitor_qam(qam_info_t *qam);

boolean video_msg_handler(ipc_message *ipc_msg, ipc_message *rsp_msg,
                          boolean *rsp_flag);
video_context_t* get_config_context(int context_id, const char *label, int *rc);
void process_video_config_qam(ipc_message *ipc_msg);
int process_video_config_qam_msg(video_config_qam_msg_t* msg,
                                 qam_config_t* cfg);
int process_video_add_session_msg (video_add_session_msg_t *msg,
                                   in_config_t *in_cfg, out_config_t *out_cfg);
int process_video_change_source_msg (video_change_source_msg_t *msg,
                                     in_config_t *old_cfg,
                                     uint32 *new_src_ip,
                                     uint32 *new_grp_ip,
                                     uint32 *init_thres);

int process_video_add_crsl_msg(video_add_crsl_msg_t *msg,
                               video_context_t *ctx, uint16 crsl_id);

void process_video_update_pid_filter(ipc_message *ipc_msg);
void process_video_update_prog_filter(ipc_message *ipc_msg);
void process_video_update_pid_remap(ipc_message *ipc_msg);
void process_video_update_prog_remap(ipc_message *ipc_msg);
void process_video_add_crsl(ipc_message *msg);
void process_video_delete_crsl(ipc_message *msg);
void process_video_query_crsl_insert(ipc_message *qry_msg,
                                     ipc_message *rsp_msg);

struct cli_control_block_;
boolean video_cli_handler(struct cli_control_block_ *ccb);

void qam_mux(uint16 qam_id, uint32 out_time_limit);
uint32 outcmd_get_size(void);
uint32 outcmd_get_outtime(out_command_t*);
out_command_t* get_muxcmd(uint16 qam_id);
void outcmd_copy(out_command_t* dst_outcmd, out_command_t* src_outcmd);

// Platform specific video functions
void get_mpeg_time(mpeg_clock_t *clk);
uint64 get_clock_time(void);
uint64 get_elapsed_sec(uint64 start_time, uint64 end_time);
boolean lcred_is_valid_primary(int id);
int32 video_get_flow_avail_space(uint16 qam_id, uint16 flow_id);
boolean qam_channel_ready(uint16 qam_id);
boolean qam_is_encrypt(uint16 qam_id);
boolean is_video_qam(uint16 qam_id);
void process_outcmd_for_flow(uint16 qam_id, uint16 flow_id);
void video_hw_stat_update(void);
void video_sniff_check(void);
void video_monitor_init(void);
void video_monitor_error_report(void);
void video_monitor_timing(out_session_t *ses);
void process_input_ip_pkt(ip_pkt_info_t *ip_pkt_info, in_session_t *ses);
int out_cfg_wait_for_pending(out_config_t *cfg);
void set_load_credit(int value);
boolean acquire_load_credit(int value);

// Returns out ID for given session ID
uint16 session_id_to_out_id(video_context_t* ctx, uint32 ses_id);

// Returns crsl ID for given carousel resource ID
uint16 resource_id_to_crsl_id(video_context_t *ctx, uint32 resource_id);

// For in session processing protection
void in_session_lock(int rid);
void in_session_unlock(int rid);

// For out session processing protection
void out_session_lock(int rid);
void out_session_unlock(int rid);

// For PSI update protection
void psi_lock(int rid);
void psi_unlock(int rid);

// For QAM processing  protection
void qam_channel_lock(int rid);
void qam_channel_unlock(int rid);

// For carousel processing  protection
void crsl_lock(int rid);
void crsl_unlock(int rid);

// For session hash update  protection
void ses_hash_table_lock(int port_id);
void ses_hash_table_unlock(int port_id);

// Utilities
void get_in_session_key(ip_header_t *ip_hdr, boolean mcast_flag,
                        ses_hash_key_t *key);
const char* get_in_session_state_label(uint8 state);
const char* get_out_session_state_label(uint8 state);
const char* get_session_psi_state_label(uint8 state);
const char* get_process_type_label(out_config_t *cfg);
int video_context_init(int id);
int video_context_cleanup(int id);
void* platform_video_context_init(void);
void platform_video_context_cleanup(void* platform_ctx);

video_context_t* check_get_video_context(FILE* fp, int context_id);
boolean is_video_es(pmt_es_info_t *es_info);
int get_rtp_header_size(uintptr_t addr);

int tp_buf_pool_create(pool_t *pool, int num_buf);
int tp_buf_pool_destroy(pool_t* pool);
int tp_buf_pool_create_named(pool_t *pool, int num_buf, const char *name);
int tp_buf_pool_destroy_named(pool_t* pool, const char *name);
int tp_buf_pool_find_named(pool_t* pool, const char *name);

uint16 qam_find_next_avail_pid(uint16 pid, uint16 max_pid, uint16 qam_id);
uint16 qam_find_prev_avail_pid(uint16 pid, uint16 min_pid, uint16 qam_id);

uint16 out_pmt_pid_check(pmt_info_t *pmt, out_session_t *ses);

int in_session_init(video_context_t *ctx, int16 rid);
int in_session_update(video_context_t *ctx, int16 rid);
void in_session_forget_pat(in_session_t *ses);
void in_session_cleanup(in_session_t *ses);
void in_session_cleanup_progs(in_session_t *ses);
uint8 in_session_set_type(in_config_t *cfg);
in_config_t* in_session_alloc(video_context_t *ctx, uint16 *ses_id,
                              const char* label, int *rc);
in_config_t *in_session_assign(video_context_t *ctx_p, uint16 in_id);

in_prog_t* in_prog_alloc(void);
void in_prog_free(in_prog_t *prog);
in_prog_t* in_prog_lookup_by_prog_num(que_elem_t *prog_list, int prog_num);
in_prog_t* in_prog_lookup_by_pmt_pid(que_elem_t *prog_list, uint16 pmt_pid);

int out_session_init(video_context_t *ctx, int16 ses_id, int16 in_ses_id);
void out_session_cleanup(out_session_t *ses);
void out_session_cleanup_progs(out_session_t *ses);
void out_session_delete(video_context_t *ctx, int16 ses_id);
int out_session_dealloc(video_context_t *ctx, uint16 ses_id);
void out_session_clear_ca_pids(out_session_t *ses, out_prog_t *prog);

out_prog_t* out_prog_alloc(void);
void out_prog_setup(out_prog_t *prog, in_prog_t *in_prog, out_session_t *ses);
void out_prog_forget_pmt(out_prog_t *prog, qam_info_t *qam);
void out_prog_cleanup(out_prog_t *prog, out_session_t *ses);
void out_prog_free(out_prog_t *prog);
out_prog_t* out_prog_lookup_by_prog_num(que_elem_t *prog_list, int prog_num);
out_prog_t* out_prog_lookup_by_in_prog_num(que_elem_t *prog_list,
                                           int in_prog_num);
out_prog_t* out_prog_lookup_by_pid(que_elem_t *prog_list, uint16 es_pid);

psi_section_t* psi_sect_lookup_by_table_id(que_elem_t *sect_list, int table_id);

void qam_init(video_context_t *ctx, uint16 qam_id);
int qam_config_program_check(video_context_t *ctx, uint16 qam_id, int prog_num);

void pmt_ver_store(qam_info_t *qam, uint16 prog_num, uint8 ver);
uint16 pmt_ver_init(qam_info_t *qam, uint16 prog_num);

void tp_info_buffer_print(FILE *fp, tp_info_buffer_t *buf);

void video_ip_stat_update(video_ip_stat_t *acc_stat, video_ip_stat_t *stat);
void video_in_stat_update(video_in_stat_t *acc_stat, video_in_stat_t *stat);
void video_out_stat_update(video_out_stat_t *acc_stat, video_out_stat_t *stat);
void video_qam_stat_update(video_qam_stat_t *acc_stat, video_qam_stat_t *stat);

void video_in_stat_collect(video_in_stat_t *stat);
void video_out_stat_collect(video_out_stat_t *stat);
void video_qam_stat_collect(video_qam_stat_t *stat);

boolean snoop_section(psi_snoop_t *snoop, tp_header_t *tp_hdr);
boolean snoop_pat_section(in_session_t *ses);
boolean snoop_pmt_section(in_session_t *ses, in_prog_t **prog);
void parse_pat_tp(in_session_t *ses, tp_info_t *tp_info);
void parse_pmt_tp(in_session_t *ses, tp_info_t *tp_info);

void dump_tp_header(FILE *fp, tp_header_t *tp_hdr);
void psi_section_print(FILE *fp, psi_section_header_t *sect);
void dump_pat(FILE *fp, pat_info_t *pat);
void dump_pmt(FILE *fp, pmt_info_t *pmt);

void clkrec_config(void);
void pll_review(in_session_t *ses);
void process_pcr_tp(in_session_t *ses, uint16 pid, int disc_flag);

int parse_pat(pat_info_t *pat);
int parse_pmt(pmt_info_t *pmt, psi_section_t *sect);

int psi_section_packetize(psi_section_t *sect, uint16 pid, que_elem_t *tp_list);
int C0_section_packetize(uint16 pid, que_elem_t *tp_list);
void add_C0_table_to_pmt_carousel(out_prog_t *prog, uint16 pmt_pid);

void clear_pat(pat_info_t *pat);
int update_pat(in_session_t *ses);
void qam_build_pat(qam_info_t *qam);
void regenerate_pat(qam_info_t *qam);
int regenerate_pmt(out_session_t *ses, out_prog_t *prog, uint16 pmt_pid);
int passthru_pmt(out_session_t *out_ses, out_prog_t *prog);

void print_tp_info(FILE *fp, tp_info_t *tp_info);
void print_pcr_info(FILE *fp, pcr_info_t *pcr_info);
void print_out_prog(FILE *fp, out_prog_t *prog);
void print_crsl(FILE *fp, crsl_t *crsl);
void print_crsl_pkt(FILE *fp, crsl_t *crsl);
void print_crsl_insert(FILE *fp, crsl_insert_t *insert);
void print_video_ip_stat(FILE *fp, video_ip_stat_t *stat);

uint32 tp_itvl_to_bitrate(uint32 tp_itvl);
uint32 tp_bitrate_to_itvl(uint32 bitrate);

int prepare_session_key(uint16 rid, in_config_t *cfg, ses_hash_key_t *key);
int provision_in_session(video_context_t *ctx, uint16 *rid, in_config_t *cfg);
int unprovision_in_session(video_context_t *ctx, uint16 *rid, in_config_t *cfg);
int provision_qam_flow(video_context_t *ctx, uint16 rid, out_config_t *cfg);
int unprovision_qam_flow(video_context_t *ctx, out_config_t *cfg);

int check_capture(ip_header_t *ip_hdr, int mode);

void video_monitor_in_set(int in_rid);
void video_monitor_in_clear(void);

void video_monitor_timing_set(int out_rid, int period, char* tty);
void video_monitor_timing_clear(void);

void print_in_session_config(in_config_t *cfg);
void print_video_in_stat(FILE *fp, video_in_stat_t *stat);
void print_video_out_stat(FILE *fp, video_out_stat_t *stat);

void clkrec_record_init(FILE *fp, char *filename, int max_size, int in_sid);
void clkrec_record_start(void);
void clkrec_record_stop(void);
void clkrec_record_status(FILE *fp);
void clkrec_record_tp(uint32 tp_idx, uint16 pid, uint32 arvl_time,
                      boolean pcr_flag, mpeg_clock_t *pcr, boolean disc_flag);

void psi_record_init(psi_record_t *rec, const char *filename, int max_tp);
void psi_record_start(psi_record_t *rec, uint16 in_id);
void psi_record_status(psi_record_t *rec, FILE *fp);
void psi_record_tp(psi_record_t *rec, uint16 in_id, tp_header_t *tp_hdr);

void crsl_init(video_context_t *ctx, int16 rid);
//void crsl_delete(video_context_t *ctx, int16 rid);
void crsl_insert_delete(video_context_t *ctx, crsl_t *crsl, uint16 qam_id);

uint64 crsl_insert_bitrate(crsl_insert_t *insert);
crsl_insert_t* find_crsl_insert(que_elem_t *crsl_insert_list, crsl_t *crsl);
int crsl_insert_schedule(crsl_insert_t *insert, uint16 qam_id, uint16 flow_id,
                         uint32 offset, uint16 pid);
int psi_crsl_insert_schedule(psi_crsl_t *crsl, uint16 qam_id, uint16 flow_id,
                             uint32 offset, uint16 out_pid);



void video_cc_check_init(uint8* cc_record);
boolean video_cc_check(tp_header_t hdr, uint8* cc_record, uint8* exp_cc);

uint32 comp_stay_time_limit(uint16 jitter_size);
uint32 comp_stay_time_oper(uint16 jitter_size);

int pcr_context_alloc(in_session_t *ses);
void pcr_context_free(in_session_t *ses);
void pcr_context_reset(in_session_t *ses);
void pcr_context_unlock(in_session_t *ses);
void pcr_context_adjust_tbo(pcr_context_t* pcr_ctx, mpeg_timediff_t adj);

void psi_section_alloc(psi_snoop_t *snoop, uint8 table_id);
void psi_section_free(psi_snoop_t *snoop);
void psi_snoop_cleanup(psi_snoop_t *snoop);

pcr_context_t* pcr_context_lookup(in_session_t *ses, uint16 pid);
pcr_context_t* pcr_context_add(in_session_t *ses);

ring_buf_t* get_cmd_ring(uint16 qam_id, uint16 flow_id);
out_command_t* get_cmd(uint16 qam_id, uint16 flow_id, uint16 idx);
out_command_t* setup_outcmd(uint16 qam_id, uint16 flow_id, 
                                  uint32 out_time, uintptr_t addr);

void outcmd_set_pid_restamp(out_command_t *cmd, uint16 out_pid);
void outcmd_set_pcr_restamp(out_command_t *cmd, mpeg_clock_t out_tbo);
void outcmd_set_cc_restamp(out_command_t *cmd, uint8 out_cc);
void outcmd_set_cw_index(out_command_t *cmd, uint16 cw_idx);

void demux_ip_pkt(uintptr_t ip_pkt_addr, hash_table_t *in_ses_hash_tab,
                  video_ip_stat_t *stat);
void advance_input_tp_info_wr(in_session_t *ses);
void process_input_session(in_session_t *ses, uint16 *flow_rd, uint16 flow_wr);
void process_input_ip_pkt_std(ip_pkt_info_t *ip_pkt_info, in_session_t *ses);
int process_input_tp(tp_header_t tp_hdr, uintptr_t tp_addr, in_session_t *ses);
void process_qam_channel(qam_info_t *qam);

void process_input_session_going_active(in_session_t *ses);
void update_avg_bitrate(in_session_t *ses);
void update_avg_out_bitrate(out_session_t *ses, int32 time_elapsed);
boolean check_session_pcr_missing(in_session_t *ses);
void pcr_context_cleanup(in_session_t *ses);

uint32 ses_hash_code(void *key);
boolean ses_hash_match(void *key, hash_item_t *item);
void ses_hash_print(FILE *fp, hash_item_t *item);

#if VIDEO_DIAG
struct tp_diag_;
extern uint16 diag_qam_id;
extern uint32 diag_tp_idx;
void video_diag_prepare(uint16 flow_id, tp_info_t *tp_info,
                        pcr_info_t *pcr_info, out_command_t *out_cmd,
                        uintptr_t tp_addr_vir);
#endif

// capture buffer related functions
int check_capture(ip_header_t *ip_hdr, int mode);
int get_l2_header_size(void);
void video_show_l2_hdr(FILE *fp);
void video_show_l3_hdr_payload(FILE *fp);

void show_in_session_cast_type(FILE *fp, in_config_t *cfg);
void show_in_session_config(FILE *fp, in_config_t *cfg);
void show_in_session_out_list(FILE *fp, in_session_t *ses);
void show_ip_flow(FILE *fp, uint16 rid);
void show_in_session_stat(FILE *fp, in_session_t *ses);
void show_in_session_pat(FILE *fp, in_session_t *ses);
void show_in_session_pmt(FILE *fp, in_session_t *ses);
void show_in_session_time(FILE *fp, in_session_t *ses);
void show_in_session_prog_list(FILE *fp, in_session_t *ses);
void show_in_session_pcr_context_table(FILE *fp, in_session_t *ses);
void show_out_session_process_type(FILE *fp, out_config_t *cfg);
void show_out_session_config(FILE *fp, out_config_t *cfg);
void show_out_session_pid_list(FILE *fp, out_config_t *cfg);
void show_out_session_program_list(FILE *fp, out_config_t *cfg);
void show_out_session_stat(FILE *fp, out_session_t *ses);
void show_out_session_psi(FILE *fp, out_session_t *ses);
void show_out_session_time(FILE *fp, out_session_t *ses);
void show_out_session_prog_list(FILE *fp, out_session_t *ses);
void show_qam_config(FILE *fp, qam_config_t *cfg);
void show_qam_stat(FILE *fp, qam_info_t *qam);
void show_qam_flows(FILE *fp, qam_info_t *qam);
void show_qam_map(FILE *fp, video_context_t *ctx, uint16 qam_id);
void clear_in_session_time(in_session_t *ses);


void in_pid_table_setup_pmt(in_pid_info_t* pid_tab, pmt_info_t *pmt);
void in_pid_table_clear_pmt(in_pid_info_t* pid_tab, uint16 pmt_pid,
                            pmt_info_t *pmt);
void in_pid_table_print(FILE *fp, in_pid_info_t *tab);
void out_pid_table_print(FILE *fp, out_pid_info_t *tab);

//void pid_histogram_print(FILE *fp, uint32 *histogram);

int video_list_search(uint16 value, uint16* list, uint8 num_item);
int video_list_add(uint16 value, uint16* list, uint8* num_item, uint8 max);
int video_list_delete(uint16 value, uint16* list, uint8* num_item);
void video_list_print(FILE *Fp, uint16 *list, uint8 num_item);

void out_prog_set_out_pid_table(out_prog_t *prog, out_pid_info_t *out_pid_tab,
                                uint16 pid_rev, uint16 prog_rev);
void out_prog_set_qam_pid_table(out_prog_t *prog, qam_pid_info_t *qam_pid_tab,
                 uint16 used, uint16 forwarded, uint16 out_ses_id);

void show_video_pid_range(FILE *fp, video_pid_range_t *pid_range,
                          int list_size);
boolean prog_num_check_conflict(uint32 prog_num, qam_prog_info_t *qam_prog_tab);
boolean out_prog_check_conflict(out_prog_t *prog, out_session_t *ses,
                                uint16 *pid);
boolean pmt_check_conflict(pmt_info_t *pmt, out_session_t *ses, uint16 *pid);
boolean out_session_recheck_conflict(out_session_t *ses, qam_info_t *qam);

void out_prog_register(out_prog_t *prog, out_session_t *ses, qam_info_t *qam);
void out_prog_unregister(out_prog_t *prog, out_session_t *ses, qam_info_t *qam);
void out_prog_remap_prog(out_prog_t *prog, out_session_t *ses);

uint64 get_physical_mem_addr(void *ptr);

boolean is_PME_session(out_prog_t *prog);


void process_video_snmp_query_input_ts_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_input_prog_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_prog_es_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_input_stats_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_input_udp_org_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_output_stats_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_output_ts_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_output_prog_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_output_prog_stats_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_output_udp_dest_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_video_sess_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);
void process_video_snmp_query_video_sess_ptr_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg);

void process_video_query_stat_req(ipc_message *ipc_msg, ipc_message *rsp_msg);
void process_video_query_in_pat_req(ipc_message *ipc_msg, ipc_message *rsp_msg);
void process_video_query_qam_pat_req(ipc_message *ipc_msg,
                                     ipc_message *rsp_msg);
void process_video_query_in_pmt_req(ipc_message *ipc_msg, ipc_message *rsp_msg);
void process_video_query_out_pmt_req(ipc_message *ipc_msg,
                                     ipc_message *rsp_msg);
void process_video_query_pid_map_req(ipc_message *ipc_msg,
                                     ipc_message *rsp_msg);
void process_video_get_sess_list (uint8_t  owner_id,
                                  uint32_t *sess_array,
                                  uint16_t *total_sessions);

// Video event related definitions
typedef void* video_event_t;
void video_event_fire_source_state_change(in_config_t *cfg,
                                          uint8 old_state, uint8 new_state);
void video_event_fire_session_state_change(uint32 session_id, uint16 prog_num,
                                           uint8 owner_id,
                                           uint8 old_state, uint8 new_state);
void video_event_fire_config_err(int msg_type, int oper, uint8 owner_id,
                                 uint32 resource_id, int err_code,
                                 uint16 num_item, uint16 *items);
void video_event_fire_source_err(in_session_t *ses, int subtype);
void video_event_fire_session_err(out_session_t *ses);
void video_event_fire_qam_err(qam_info_t *qam);
void video_event_fire_conflict_err(int subtype, uint16 qam_id, uint8 owner_id,
                                   uint32 session_id, uint16 id);
void video_event_fire_operation_err(int err_code);

// Prototype of platform-specific utilities
video_event_t* video_event_alloc(void);
video_event_hdr_t* video_event_get_data(video_event_t *ev);
void video_event_send(video_event_t *ev);

void video_perf_wake_update(video_perf_t *perf, uint32 now);

#endif /* __VIDEO_H__ */

