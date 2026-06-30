/*
 *-----------------------------------------------------------------------------
 * ermi_c_global.h - ERMI-Video Resource Control - global defines
 *
 * January 2004, Xiaomei Liu, July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __ERMI_C_GLOBAL_H__
#define __ERMI_C_GLOBAL_H__

#define MAX_SGID_LEN            64 /* servide group id len */
#define MAX_SOCKET_SERVICE_LOOP 5  /* handle up to five reads */
#define SESSID_LEN8             (8+1)  /* session_id max length +1 for '\0' */
#define MAX_SESSID_LEN          (20+1)  /* session_id max bytes + 1 for '\0' */
#define MAX_URL_LEN             128   /*url len*/
#define MAX_PROG_LEN            64   /*prog name len*/
#define URL_ESCAPE_LEN          3

#define DEFAULT_SGID "Unassigned"

typedef enum transport_ {
    MPEG_UNKNOWN = 0,
    MPEG_UDP = 1,
    MPEG_QAM
}transport_t;

typedef enum {
    ERMI_C_QAM_STATUS_UNKNOWN = -1,
    ERMI_C_QAM_STATUS_BAD,
    ERMI_C_QAM_STATUS_GOOD,
    ERMI_C_QAM_STATUS_MAINTENANCE,
} ermi_qam_status_t;

typedef enum {
    ERMI_C_INPUT_PORT_EVENT = 1,
    ERMI_C_QAM_EVENT,
    ERMI_C_LC_SESSION_EVENT
} lc_event_type_t;

typedef enum {
    ERMI_OK = 0,
    ERMI_MEM_ERROR,       /*malloc failed*/
    ERMI_OP_ERROR,        /*bad operation*/
    ERMI_PARAM_ERROR,     /*bad parameter*/
    ERMI_RTSP_ERROR,      /*general RTSP error*/
    ERMI_RTSP_SOCK_ERROR, /*rtsp socket error*/
    ERMI_RTSP_SOCK_BLOCKED, /*rtsp socket blocked */
    ERMI_TIMER_ERROR,     /*timer error*/
    ERMI_PARSE_ERROR,
    ERMI_PARSE_INCOMPLETE_MSG,
    ERMI_PARSE_CONTENT_AFTER_MSG,
    ERMI_FAIL,
    ERMI_LAST
} ermi_status;

typedef enum {
    ERMI_C_SETPARAM_TIMEOUT=601,
    ERMI_C_SETPARAM_RES_FAIL
} ermi_c_session_status_t;

#define RP_SETBIT(bitmap, i) (bitmap |= i)
#define RP_TESTBIT(bitmap, i) (bitmap & i)
#define RP_CLEARBITS(bitmap) (bitmap = 0)

/*
 * Externs
 */

/*
 * Prototypes
 */
void ermi_video_parser_init(void);
void ermi_c_global_commands(parseinfo *csb);

#endif __ERMI_C_GLOBAL_H__
