/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: rfgw_errp.h
*    @brief Header file containing data structures used
*           in RFGW-10 implementation
*    @author Yi Tong Tse
*/


// The following are copied from the RFGW-10 implementation
//
//

#define MAX_DEPI_SESSION_PER_QAM   4

#define RFGW_MB_DEFAULT_QAM_FORMAT64   1
#define RFGW_MB_DEFAULT_QAM_FORMAT256  2


/*
 * RFGW Annex types
 */
typedef enum {
    RFGW_ANNEX_UNKNOWN = 0,
    RFGW_ANNEX_A = 1,
    RFGW_ANNEX_B = 2,
    RFGW_ANNEX_C = 3
} rfgw_qam_annex_t;


/*
 * RFGW Qam Mode types
 */
typedef enum {
    LCC_QAM_TYPE_INVALID = 0,
    LCC_QAM_TYPE_NONE,
    LCC_QAM_TYPE_DEPI,
    LCC_QAM_TYPE_VIDEO,
    LCC_QAM_TYPE_MAX = 4,
} rfgw_qam_type_t;


/*
 * RFGW Qam Owner type
 */
typedef enum {
    LCC_QAM_OWNER_NONE = 0,
    LCC_QAM_OWNER_LOCAL,
    LCC_QAM_OWNER_REMOTE
} rfgw_qam_owner_t;


/*
 * RFGW Qam map types
 */ 
typedef enum {
    LCC_VIDEO_QAM_MAP_NONE = 0,
    LCC_VIDEO_QAM_MAP_24,
    LCC_VIDEO_QAM_MAP_MAX
} rfgw_video_qam_map_t;


typedef uint32 ipaddrtype;
typedef uint8 tinybool;
typedef uint64 sys_timestamp;


//! QAM data structure used in RFGW-10
typedef struct rfgw_qam_ {
    uint32              slot;           /* key 1 */
    uint32              port;           /* key 2 */
    uint32              channel;        /* key 3 */

//    BITMASK_DEFINITION( record_download, SLOT_ARRAY_SIZE );

    /* PORT level attributes */
    uint8               stacking;       /* stacking */
    rfgw_qam_annex_t    annex_type;     /* annex type */
    uint32              srate;          /* symbol rate */
    uint32              s_rate_m;       /* symbol rate M */
    uint32              s_rate_n;       /* symbol rate N */
    tinybool            freq_inv;       /* enable freq inv */
    uint32              format;         /* modulation format */

    rfgw_qam_type_t     type;           /* DEPI or Video */
    rfgw_qam_owner_t    owner;
    boolean             learn_mode;
    rfgw_video_qam_map_t    video_qam_map;
    uint32              session_id[MAX_DEPI_SESSION_PER_QAM]; /* DEPI session id on qam */
    uint32              tsid;           /* chassis-wide unique*/
    uint32              bw;             /* bw in Hz */
    boolean             is_enabled;     /* via stacking cli */
    uint32              mode;           /* psp/mpt/video/off */
    uint8               intl_level;     /* interleave level */
    uint8               fec_i;          /* FEC I level*/
    uint8               fec_j;          /* FEC J level*/
    uint16              offset;         /* DOCSIS timing offset */
    uint32              freq;           /* qamch frequency */
    uint32              power;          /* single qam pwr in dBmV x10 */
    ipaddrtype          qam_ip;         /* data ip address */
    ipaddrtype          erm_ip;         /* ctrl ip address */
    uint32              test_mode;      /* cw/prbs */
    uint32              psi_interval;   /* video PSI timeout interval */
    uint8               cnfg_lock;      /* config lock */
    tinybool            cnfg_changed;   /* has config changed */
    tinybool            vidcnfg_changed;/* has video config changed */
    sys_timestamp       state_time;     /* system up time since current state */
    uint32              reserved_bw;    /* bandwidth reserved by sessions */
    uint32              total_sessions;
    uint32              total_groups;
    uint32              total_shells;   /* count of shells created on this qam */
    uint32              total_binds;    /* count of which shells are bound */
    uint32              shell_reserved_bw;    /* bandwidth reserved for shell sessions, sum of all group bws*/
    uint32              shell_used_bw;    /* bandwidth used for shell sessions, sum of all bw for bound sessions */
    uint32              link_trap_enable;/* SNMP trap status */

} rfgw_qam_t;


// Function prototypes
//

boolean rfgw_errp_get_qam_cfg(const struct rfgw_qam_ *rfgw_cfg,
                              errp_qam_cfg_t *errp_cfg);

void rfgw_errp_get_qam_cap(const struct rfgw_qam_ *rfgw_cfg,
                           errp_qam_cap_t *errp_cap);




