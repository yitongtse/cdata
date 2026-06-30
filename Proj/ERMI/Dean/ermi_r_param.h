/*
 *------------------------------------------------------------------
 * ermi_r_param.c
 *
 * Header file for wraper functions to get QAM parameters
 *
 * Jan 2009, Dean Chen
 *
 * Copyright (c) 2009 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

typedef enum {
    ERRP_PARAM_QAM_GROUP_NAME,
    ERRP_PARAM_STREAMING_ZONE_NAME,
    ERRP_PARAM_SIGNALING_ADDR,
    ERRP_PARAM_ALT_SIGNALING_ADDR,
    ERRP_PARAM_MAX_MPEG_FLOWS,
    ERRP_PARAM_QAM_NAME,
    ERRP_PARAM_QAM_COST,
    ERRP_PARAM_QAM_CHAN_BW_CAP,
    ERRP_PARAM_QAM_J83_CAP,
    ERRP_PARAM_QAM_INTERLEAVER_CAP,
    ERRP_PARAM_QAM_DOCSIS_VIDEO_CAP,
    ERRP_PARAM_QAM_MODULATION_CAP,
    ERRP_PARAM_QAM_TOTAL_BW,
    ERRP_PARAM_QAM_AVAIL_BW,
    ERRP_PARAM_QAM_RF_PORT_ID,
    ERRP_PARAM_QAM_SERVICE_STATUS,
    ERRP_PARAM_QAM_FREQ,
    ERRP_PARAM_QAM_MOD_MODE,
    ERRP_PARAM_QAM_INTERLEAVER,
    ERRP_PARAM_QAM_TSID,
    ERRP_PARAM_QAM_ANNEX,
    ERRP_PARAM_QAM_CHAN_WIDTH,
    ERRP_PARAM_QAM_STATIC_UDP_MAP,
    ERRP_PARAM_QAM_STATIC_RANGE_UDP_MAP,
    ERRP_PARAM_QAM_DYNAMIC_RANGE_UDP_MAP,
    ERRP_PARAM_QAM_FIBER_NODE_NAME,
    ERRP_PARAM_QAM_INPUT_MAP,
    ERRP_PARAM_EDGE_INPUT_SUBNET_MASK,
    ERRP_PARAM_EDGE_INPUT_ADDR,
    ERRP_PARAM_EDGE_INPUT_BW,
    ERRP_PARAM_EDGE_INPUT_INTF_ID,
    ERRP_PARAM_EDGE_INPUT_GROUP_NAME,
} errp_param_t;

typedef enum {
  ERRP_QAM_STATUS_OPERATIONAL = 1,
  ERRP_QAM_STATUS_SHUTTING_DOWN,
  ERRP_QAM_STATUS_STANDBY,
};

#define NUM_UDP_MAP 1024
typedef struct {
    uint32 num_static_port;
    ushort udp_port[NUM_UDP_MAP];
    ushort mpeg_prog[NUM_UDP_MAP];
} static_udp_map;

typedef struct {
    uint32 num_static_port_ranges;
    ushort starting_udp_port[NUM_UDP_MAP];
    ushort starting_mpeg_prog[NUM_UDP_MAP];
    uint32 count[NUM_UDP_MAP];
} static_range_udp_map;    

typedef struct {
    uint32 num_dynamic_port_ranges;
    ushort starting_udp_port[NUM_UDP_MAP];
    ushort count[NUM_UDP_MAP];
} dynamic_range_udp_map;   

#define ERRP_DFLT_EDGE_INPUT_GROUP "ERRP_DFLT_EDGE_INPUT_GROUP"





