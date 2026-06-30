/*
 * Copyright (c) 2015 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *   Platform-specific video definitions
 */

#ifndef __VIDEO_DEFS_H__
#define __VIDEO_DEFS_H__


// Feature set
#define VIDEO_DIAG                      0

#define CACHE_SIZE                      64

// Platform-specific video parameters
#define NUM_INPUT_PORTS                 1
#define NUM_QAMS                        1
#define NUM_VIDEO_QAMS                  1
#define MAX_SESSIONS                    20
#define MAX_VIDEO_FLOWS_PER_QAM         20

#define SES_HASH_SLOTS                  1
#define PROG_HASH_SLOTS                 1
#define PID_HASH_SLOTS                  1
#define OUTCMDS_PER_QAM                 2048

#define SES_ID_HASH_SLOTS       	1
#define CRSL_ID_HASH_SLOTS      	1

#define MAX_CA_SESSIONS                 1
#define ECM_INSERT_CNT                  4

#define SYS_LATENCY                     40
#define BUF_HEADROOM                    5
#define MAX_JITTER                      200

// Schedule window sizes
#define SCH_WIN_MS                      40
#define PAST_SCH_WIN_MS                 500

#define VIDEO_MSG_BUF_SIZE              10000


// IP packet info
typedef struct {
    uintptr_t addr;
    uint32 arvl_time;
    uint32 num_bytes;
} ip_pkt_info_t;


// Ouptut command
typedef struct {
    uint32 qam_id : 6;
    uint32 flow_id : 6;
    uint32 out_time : 20;       // output time in 45 kHz ticks

    uint32 pid_restamp_flag : 1;
    uint32 cc_restamp_flag : 1;
    uint32 pcr_restamp_flag : 1;
    uint32 resv : 2;
    uint32 cc : 4;
    uint32 pid : 13;
    uint32 tbo_ext : 10;        // time base offset - extension part

    uint32 tbo_base;            // time base offset - base part

    uintptr_t addr;             // TP address
} out_command_t;

#define OUTCMD_FLAGS(out_cmd)     (*(((uint32*)(out_cmd)) + 2))


// IPC related definitions
#define VDMAN_IPC_OK            0

typedef uint32 vdman_ipc_rc_t;
typedef uint32 vipc_rc_t;

static inline
const char* vdman_ipc_rc_string (vdman_ipc_rc_t rc)
{ return ""; }

static inline
const char *vipc_rc_string (vipc_rc_t rc)
{ return ""; }

vdman_ipc_rc_t video_ca_ipc_send_to_scs(uint32_t msg_type, void *app_data,
                                        vipc_rc_t *rc);

// Error message definitions
#define em_vidman_LOGGER_OPEN_FAILED(...)
#define em_vidman_IPC_MALLOC_BUFFER_FAILED(...)
#define em_vidman_IPC_SEND_FAILED(...)
#define em_vidman_LCRED_BAD_MODE_ROLE_TRANSITION(...)
#define em_vidman_LCRED_UNEXPECTED_GO_HOT(...)
#define em_vidman_LCRED_UNEXPECTED_JOINED_GROUP(...)
#define em_vidman_LCRED_UNEXPECTED_LEFT_GROUP(...)
#define em_vidman_VIDEO_CONTEXT_RESET_FAILED(...)
#define em_vidman_VIDEO_BAD_OUT_SESSION_ID(...)
#define em_vidman_VIDEO_BAD_QAM_ID(...)
#define em_vidman_VIDEO_UNEXPECTED_PROG_NUM(...)
#define em_vidman_VIDEO_PROG_NUM_IN_USE(...)
#define em_vidman_VIDEO_MISSING_PROG_NUM(...)
#define em_vidman_VIDEO_COMMAND_FAILED(...)
#define em_vidman_VIDEO_IN_SESSION_INIT_FAILED(...)
#define em_vidman_VIDEO_OUT_SESSION_INIT_FAILED(...)
#define em_vidman_VIDEO_SESSION_NOT_USED(...)
#define em_vidman_VIDEO_OUT_SESSION_NOT_USED(...)
#define em_vidman_VIDEO_INCOMPLETE_MSG(...)
#define em_vidman_VIDEO_UNKNOWN_MSG(...)
#define em_vidman_VIDEO_CAROUSEL_ALLOC_FAILED(...)
#define em_vidman_VIDEO_CAROUSEL_NOT_FOUND(...)
#define em_vidman_VIDEO_FLOW_ALLOC_FAILED(...)
#define em_vidman_VIDEO_BAD_PRIMARY_ID(...)
#define em_vidman_VIDEO_CONTEXT_INIT_FAILED(...)
#define em_vidman_VIDEO_IN_SESSION_ALLOC_FAILED(...)
#define em_vidman_VIDEO_BAD_IN_SESSION_ID(...)
#define em_vidman_VIDEO_BAD_INPUT_TYPE(...)
#define em_vidman_VIDEO_BAD_IP_ADDR_LEN(...)
#define em_vidman_VIDEO_BAD_DST_UDP_PORT(...)
#define em_vidman_VIDEO_BAD_JITTER(...)
#define em_vidman_VIDEO_BAD_DELAY(...)
#define em_vidman_VIDEO_BAD_CR_MODE(...)
#define em_vidman_VIDEO_BAD_BITRATE_ALLOC(...)
#define em_vidman_VIDEO_BAD_PROG_NUM(...)
#define em_vidman_VIDEO_BAD_PID_RANGE(...)
#define em_vidman_VIDEO_BAD_INPUT_PORT(...)
#define em_vidman_VIDEO_CAROUSEL_IN_USE(...)
#define em_vidman_VIDEO_PACKET_INSERT_ALLOC_FAILED(...)
#define em_vidman_VIDEO_FEATURE_NOT_AVAIL(...)
#define em_vidman_VIDEO_BAD_FILTERED_PID(...)
#define em_vidman_VIDEO_BAD_FILTERED_PROG(...)
#define em_vidman_VIDEO_BAD_REMAPPED_PID(...)
#define em_vidman_VIDEO_OUT_PROG_ALLOC_FAILED(...)
#define em_vidman_VIDEO_PSI_SECTION_ALLOC_FAILED(...)
#define em_vidman_VIDEO_BAD_TABLE_IN_PAT(...)
#define em_vidman_VIDEO_TOO_MANY_SECTION_IN_PAT(...)
#define em_vidman_VIDEO_BAD_PSI_SECTION_NUM(...)
#define em_vidman_VIDEO_PSI_CRC_ERROR(...)
#define em_vidman_VIDEO_TOO_MANY_PROG_IN_PAT(...)
#define em_vidman_VIDEO_TOO_MANY_NIT_IN_PAT(...)
#define em_vidman_VIDEO_UNKNOWN_PROG_NUM(...)
#define em_vidman_VIDEO_TOO_MANY_PMT_SECTIONS(...)
#define em_vidman_VIDEO_TOO_MANY_CA_DESC(...)
#define em_vidman_VIDEO_BAD_PROG_DESC(...)
#define em_vidman_VIDEO_TOO_MANY_ES(...)
#define em_vidman_VIDEO_BAD_ES_DESC(...)
#define em_vidman_VIDEO_PSI_BLOCKED(...)
#define em_vidman_VIDEO_PMT_NOT_FOUND(...)
#define em_vidman_VIDEO_TOO_MANY_PROG_IN_QAM(...)
#define em_vidman_VIDEO_PAT_BUILD_FAILED(...)
#define em_vidman_VIDEO_PACKET_ALLOC_FAILED(...)
#define em_vidman_VIDEO_PROG_CA_BLOCKED(...)

// LCHA related definitions
typedef enum {
    LCRED_NON_REDUNDANT_MODE = 0,  // card is defined as non-redundant
    LCRED_PRIMARY_MODE,            // card is defined as a primary
    LCRED_SECONDARY_MODE,          // card is defined as a secondary
    LCRED_MAX_MODE,
} lcred_mode_t;

typedef enum {
    LCRED_ACTIVE_ROLE = 0,         // card is currently active
    LCRED_STANDBY_ROLE,            // card is currently standby
    LCRED_MAX_ROLE,
} lcred_role_t;

typedef struct msg_card_lcred_mode_role_t_ {
    unsigned int   _errno;         // return errno
    lcred_mode_t    mode;          // either NONE, PRIMARY or SECONDARY
    lcred_role_t    role;          // either ACTIVE or STANDBY
} msg_card_lcred_mode_role_t;

typedef struct msg_card_lcred_go_hot_t_ {
    unsigned int   _errno;         // return errno
    int  primary_slot;             // specifies which of the configs to make HOT
    int  group;                    // redundancy group number
} msg_card_lcred_go_hot_t;

typedef struct msg_card_lcred_joined_left_group_t_ {
    unsigned int   _errno;         // return errno
    int  primary_slot;             // specifies which primary joined the group
    int  group;                    // redundancy group number
} msg_card_lcred_joined_left_group_t;

extern lcred_mode_t lcred_mode;
extern lcred_role_t lcred_role;

boolean check_lcred_state(FILE* fp);


//// Additional stuff to make porting works
#define INT_MAX		2147483647

enum {
    INPUT_TYPE_UNICAST = 1,
    INPUT_TYPE_MULTICAST_SSM,
    INPUT_TYPE_MULTICAST_ASM
};

enum {
    STREAM_TYPE_SPTS = 1,
    STREAM_TYPE_MPTS
};


#define ES_ACTION_ADD           0
#define ES_ACTION_DELETE        1
typedef struct {
    uint16_t action      : 1;     // ES_ACTION_ADD or ES_ACTION_DELTE
    uint16_t stream_type : 2;     // same value as en_stream_type_t
    uint16_t pid         : 13;    // PID value
} es_change_t;


#define MAX_UPDATES             32
typedef struct {
    int count;
    es_change_t table[MAX_UPDATES];
} pmt_change_t;

typedef int err_t;

typedef enum{
    esVideoComponent,          ///< Video
    esAudioComponent,          ///< Audio
    esDataComponent,           ///< Data
    esUnknownComponent         ///< Unknown
}en_stream_type_t;

typedef struct{
    uint16_t pid;                       //< PID of elementary stream
    en_stream_type_t m_en_stream_type;  //< Stream type of component
} es_pid_t;


#define __UNUSED	
#endif  // __VIDEO_DEFS_H__
