/*
 * Copyright (c) 2011-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "prio_que.h"
#include "video.h"
#include "video_util.h"
#include "video_cli.h"


static
int mux_flow_cmp (void *a, void *b)
{
    return ((int32)(*((uint32*)b) - *((uint32*)a))) > 0;
}


void qam_mux (uint16 qam_id, uint32 _out_time_limit)
{
    out_command_t *outcmd;
    qam_info_t* qam = get_qam_info(qam_id);
    prio_que_t mux_pq;
    ring_buf_t* ring;
    uint32 mux_cache[FLOWS_PER_QAM];
    uint32 past_time_limit = _out_time_limit - PAST_SCH_WIN;

    // Set up the priority queue for muxing
    prio_que_create(&mux_pq, mux_flow_cmp);

    _out_time_limit <<= OUT_TIME_SHIFT;
    past_time_limit <<= OUT_TIME_SHIFT;

    // Populate mux cache
    int i;
    for (i=0; i<FLOWS_PER_QAM; i++) {
        ring = get_cmd_ring(qam_id, i);

        // Drop all overdue out commands
        while (ring->rd != ring->wr) {
            outcmd = get_cmd(qam_id, i, ring->rd);
            mux_cache[i] = outcmd_get_outtime(outcmd) << OUT_TIME_SHIFT;
            if ((int32)(mux_cache[i] - past_time_limit) > 0) {
                if (((int32)(mux_cache[i] - _out_time_limit)) < 0) {
                    prio_que_enqueue(&mux_pq, &mux_cache[i]);
                }
                break;
            }

            ring->rd = ring_mod(++ring->rd, OUTCMDS_PER_FLOW);
            qam->stat.drop_tp_cnt++;
        }
    }

    // Perform multiplexing
    while (!prio_que_is_empty(&mux_pq)) {
        int refill = 0;
        int best_flow = ((uint32*)mux_pq.data[0]) - mux_cache;

        out_command_t* muxcmd = get_muxcmd(qam_id);
        if (!muxcmd)  return;

        // Copy selected output command to muxed command buffer
        ring = get_cmd_ring(qam_id, best_flow);
        outcmd_copy(muxcmd, get_cmd(qam_id, best_flow, ring->rd));
        qam->stat.mux_tp_cnt++;

        // Replenish the selected flow
        ring->rd = ring_mod(++ring->rd, OUTCMDS_PER_FLOW);
        if (ring->rd != ring->wr) {
            outcmd = get_cmd(qam_id, best_flow, ring->rd);
            mux_cache[best_flow] = outcmd_get_outtime(outcmd) << OUT_TIME_SHIFT;
            if (((int32)(mux_cache[best_flow] - _out_time_limit)) < 0) {
                mux_pq.data[0] = &mux_cache[best_flow];
                refill = 1;
            }
        }

        if (!refill) {
            mux_pq.data[0] = mux_pq.data[--mux_pq.count];
        }
        prio_que_percolate_down(&mux_pq, 0);
    }
}
