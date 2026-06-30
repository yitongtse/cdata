/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "video.h"
#include "video_cli.h"


// Global variables
in_session_t *in_session;
out_session_t *out_session;
qam_info_t *qam_info;
ip_pkt_info_t *ip_flow_buf;

uint32 input_buf_offset[NUM_INPUT_PORTS];
uint32 psi_tp_buf_offset;
uint32 crsl_tp_buf_offset;

pool_t in_prog_pool;
pool_t out_prog_pool;
pool_t tp_info_buf_chunk_pool;
pool_t pcr_context_spts_pool;
pool_t pcr_context_mpts_pool;
pool_t psi_section_pool;
pool_t private_section_pool;
pool_t psi_tp_buf_pool;

// Statistics for sessions already deleted
video_in_stat_t video_in_stat_history;  // overall video input statistics
video_out_stat_t video_out_stat_history;// overall video ouptut statistics

video_context_t *video_ctx = NULL;

boolean monitor_platform_flag = FALSE;

boolean session_state_update_flag = TRUE;

// Capture buffer
int capture_mode = CLI_VIDEO_CAPTURE_OFF;
boolean capture_flag = FALSE;
uint8 capture_buf[IP_PKT_BUF_SIZE];
psi_record_t psi_record;

// For testing only
boolean disc_insert_enable = TRUE;

