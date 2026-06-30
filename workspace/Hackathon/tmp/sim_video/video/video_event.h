/*
 * Copyright (c) 2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __VIDEO_EVENT_H__
#define __VIDEO_EVENT_H__

#include "vidman_defs.h"


// Video event major types
enum {
    VIDEO_EVENT_SOURCE_STATE_CHANGE,
    VIDEO_EVENT_SESSION_STATE_CHANGE,
    VIDEO_EVENT_CONFIG_ERR,
    VIDEO_EVENT_SOURCE_ERR,
    VIDEO_EVENT_SESSION_ERR,
    VIDEO_EVENT_QAM_ERR,
    VIDEO_EVENT_CONFLICT_ERR,
    VIDEO_EVENT_OPERATION_ERR 
};


// Video source error subtypes
enum {
    VIDEO_EVENT_BITRATE_EXCEEDED
};


// Video conflict error subtypes
enum {
    VIDEO_EVENT_PID_CONFLICT,
    VIDEO_EVENT_PROGRAM_CONFLICT
};


typedef struct {
    uint8_t ip_ver;
    uint32_t src_ip[4];
    uint32_t dst_ip[4];
    uint16_t src_udp;
    uint16_t dst_udp;
} video_source_t;


// Video event header
typedef struct {
    uint8_t type;               // event major type
    uint8_t subtype;            // event subtype
    uint16_t owner_id;
} video_event_hdr_t;


typedef struct {
    video_source_t source;
    uint8_t old_state;
    uint8_t new_state;
} video_event_source_state_change_t;

typedef struct {
    uint32_t session_id;
    uint16_t prog_num;          // out program number for remux sessions,
                                // or 0 for other processing types
    uint8_t old_state;
    uint8_t new_state;
} video_event_session_state_change_t;

// Note: subtype is set to the config message type
typedef struct {
    uint32_t resource_id;       // session ID or carousel resource ID
    uint32_t oper;              // only used in update pid/prog filter/remap
    uint16_t err_code;          // EINVAL, EBUSY, ENOENT, ENOMEM, EPERM
    uint16_t num_item;
    uint16_t item[VIDEO_MAX_FILTERS];   // pids / program numbers in error
} video_event_config_err_t;

typedef struct {
    video_source_t source;
    uint32_t bitrate;
} video_event_source_err_t;

typedef struct {
    uint32_t session_id;
} video_event_session_err_t;

typedef struct {
    uint32_t qam_id;
} video_event_qam_err_t;

typedef struct {
    uint32_t qam_id;
    uint32_t session_id;
    uint32_t id;                // pid or prog_num in conflict
} video_event_conflict_err_t;

typedef struct {
    uint32_t err_code;
} video_event_operation_err_t;


typedef struct {
    video_event_hdr_t hdr;
    union {
        video_event_source_state_change_t src_state_chg;
        video_event_session_state_change_t ses_state_chg;
        video_event_config_err_t cfg_err;
        video_event_source_err_t src_err;
        video_event_session_err_t ses_err;
        video_event_qam_err_t qam_err;
        video_event_conflict_err_t conflict_err;
        video_event_operation_err_t oper_err;
    } body;
} video_event_msg_t;


#endif  // __VIDEO_EVENT_H__
