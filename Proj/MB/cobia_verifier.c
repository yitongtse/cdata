/*
 * Copyright (c) 2007-2008 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <inttypes.h>


typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;
typedef int64_t         int64;
typedef uint64_t        uint64;



#define NUM_QAMS                48


// TP header
//
typedef struct {
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
typedef struct {
    uint64 len : 8;
    uint64 discontinuity : 1;
    uint64 random_access : 1;
    uint64 es_priority : 1;
    uint64 pcr_flag : 1;
    uint64 opcr_flag : 1;
    uint64 splice_point : 1;
    uint64 private_data : 1;
    uint64 af_ext : 1;
    uint64 pcr_base_1 : 32;
    uint64 pcr_base_2 : 1;
    uint64 resv : 6;
    uint64 pcr_ext : 9;
} af_header_t;


// Output command
//
typedef struct {
    // Word 1
    uint32 qam_id : 6;
    uint32 flow_id : 6;
    uint32 out_time : 20;       // output time in 45 kHz ticks

    // Word 2
    uint32 addr;

    // Word 3
    uint32 pid_restamp_flag : 1;
    uint32 cc_restamp_flag : 1;
    uint32 scramble_flag : 1;
    uint32 pcr_restamp_flag : 1;
    uint32 resv : 1;
    uint32 cc : 4;
    uint32 pid : 13;
    uint32 tbo_ext : 10;        // time base offset - extension part

    // Word 4
    uint32 tbo_base;            // time base offset - base part
} out_command_t;


// Diagnostic info to be overlaid to a MPEG-2 TP
//
typedef struct {
    tp_header_t tp_hdr;
    af_header_t af_hdr;
    uint8 gap[148];
    tp_header_t orig_tp_hdr;
    af_header_t orig_af_hdr;
    out_command_t out_cmd;
    uint32 flow_tp_idx;
    uint32 ses_tp_idx;
    uint32 arvl_time;
} tp_diag_t;


typedef struct {
    uint32 base;                // in 45 kHz clock ticks
    int16  ext;                 // in 27 MHz clock ticks, from 0 to 599
} mpeg_clock_t;


// Global variables
uint32 qam_out_time[NUM_QAMS];
uint32 max_delay[NUM_QAMS];


void cobia_verify (tp_diag_t *tp)
{
    out_command_t* cmd = &tp->out_cmd;
    int32 diff;
    mpeg_clock_t pcr, local_pcr;

    // Check PID remap
    if (cmd->pid_restamp_flag) {
        if (tp->tp_hdr.pid != cmd->pid) {
            printf("PID remap failed\n");
        }
    }

    // Check CC restamp
    if (cmd->cc_restamp_flag) {
        if (tp->tp_hdr.cc != cmd->cc) {
            printf("CC restamp failed\n");
        }
    }

    // Check FIFO scheduling
    diff = (int32)((cmd->out_time << 12) - (qam_out_time[cmd->qam_id] << 12));
    if (diff < 0) {
        printf("DT goes backward\n");
    }
    qam_out_time[cmd->qam_id] = cmd->out_time;

    // Check scheduling delay
    if (cmd->pcr_restamp_flag) {
        af_header_t* af_hdr = &tp->af_hdr;

        // Get output PCR
        if ((tp->tp_hdr.af_ctrl & 2)            // adaptation field presents
              && (af_hdr->len > 0)              // flags field in AF presents
              && af_hdr->pcr_flag               // PCR presents
           ) {
            pcr.base = af_hdr->pcr_base_1;
            pcr.ext = af_hdr->pcr_base_2 * 300 + af_hdr->pcr_ext;

            // Calculate local PCR: local_pcr = out_pcr - pcr_offset
            local_pcr.base = pcr.base - cmd->tbo_base;
            diff = (int32)(pcr.ext - cmd->tbo_ext);
            if (diff < 0) {
                local_pcr.base--;
                diff += 600;
            }
            local_pcr.ext = diff;

            // Compare local PCR with scheduled out time
            local_pcr.base &= 0xFFFFF;          // least 20 bits of local PCR
            diff = (int32)((cmd->out_time << 12) - (local_pcr.base << 12));
            if (diff < 0) {
                printf("TP scheduled before delivery time\n");
            }
            if (diff > max_delay[cmd->qam_id]) {
                max_delay[cmd->qam_id] = diff;
            }

        } else {
            printf("PCR restamp flag set for TP not containing PCR\n");
        }
    }
}

