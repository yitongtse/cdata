/*
 *-------------------------------------------------------------------
 * parser_defs_ermi_c.h - Definitions for ERM parser commands.
 *
 * 02/2004, Xiaomei Liu: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

/*
 *  show commands
 */

#ifndef __PARSER_DEFS_ERMI_C_H__
#define __PARSER_DEFS_ERMI_C_H__

/* EDA related commands */
typedef enum {
    EDA_SG_SUBMODE = 100,  /* service group config sub mode */
    EDA_SG_ERM,  
    EDA_SG_CFG_INT_SG_ID,
    EDA_SUBMODE,          /* eda config sub mode */
    EDA_ENABLE, 
    EDA_CFG_SVC_GRP,
    EDA_VREP,
    EDA_SG_ERM_NVGEN,
    EDA_NVGEN,
    VIDEO_EDA_NVGEN
} eda_global_cmd_id;

/* General Global commands */
typedef enum {
    ERMI_C_CONTROL_PLANE_IP = 300,
    ERMI_C_CONTROL_PLANE_IP_DEBUG,
    ERMI_C_CFG_VIDEO
} ermi_c_global_cmd_id;

typedef enum {
    RTSP_SHOW_SERVER,
    RTSP_SHOW_SERVER_MSG_STATS,
    RTSP_SHOW_CLIENT
} rtsp_show_cmd_id;

typedef enum {
    EDA_SHOW_SG_ALL = 1,
    EDA_SHOW_SG_ONE,
    EDA_SHOW_QAM,
} eda_show_cmd_id;

typedef enum {
    TEST_EDA_SESSION_SETUP = 100,
    TEST_EDA_SESSION_TEARDOWN,
    TEST_EDA_VREP,
    TEST_EDA_NO_VREP,
} eda_test_cmd_id;

/* CLI commands to be sent to master process */
typedef enum {
    ERMI_C_RTSP_SERVER_INIT = 600,
    ERMI_C_RTSP_SERVER_SETPARAM,
    ERMI_C_RTSP_SERVER_STOP,
    ERMI_C_RTSP_CLIENT_CONNECT,
    ERMI_C_RTSP_CLIENT_SETUP,
    ERMI_C_RTSP_CLIENT_SETPARAM,
    ERMI_C_RTSP_CLIENT_TEARDOWN,
    ERMI_C_RTSP_CLIENT_STOP,
    ERMI_C_TEST_ERM_EDA,
    ERMI_C_TEST_ERM_SM,
    ERMI_C_TEST_ERM_SS,
    ERMI_C_TEST_RTSP_NO,
    ERMI_C_RTSP_PORT,
    ERMI_C_RTSP_MEM,
    ERMI_C_ENABLE,
    ERMI_C_DISABLE,
} ermi_c_rtsp_cmd_id;

#define ERM_MSG_SOURCE_SM   0
#define ERM_MSG_SOURCE_EDA  1
#define ERM_MSG_SOURCE_SS   2

#define ERM_TEST_MSG_TEARDOWN   0
#define ERM_TEST_MSG_SETUP      1
#define ERM_TEST_MSG_SETPARAM   2

#define ERM_TEST_MSG_RESP_OK    200

#endif __PARSER_DEFS_ERMI_C_H__


