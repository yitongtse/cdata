/*****************************************************************************
 * Copyright (c) 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : track.c
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    05-22-01  Jimmy Gou      Created.
 *    01-28-02  FL      declare outPidgInfo, 
 *                      replace cbInfo[ch].len by outPidgInfo[ch].pktInfoSzie
 *    02-14-02  FL      IOS coding style
 *
 *****************************************************************************/

/***************************************************************************
 *
 *     track.c
 *
 *     Interface and task interaction between the system and the Tracking
 *     Loop (including the IIR) of the Clock Drifting Compensation
 *
 ***************************************************************************/

#include <assert.h>
#include <std.h>
#include <tsk.h>

#include "cbinfo.h"
#include "clock.h"
#include "track.h"
#include "trkloop.h"
#include "interpol.h"

#include "ns_com_sys.h"
#include "ns_com_mem.h"
#include "ns_com_msg.h"
#include "nsinfo.h"

extern OutPidgInfo outPidgInfo[];
#pragma DATA_ALIGN( bufTrack, 8 );
static far PKTINFO bufTrack[NPKTS_TRACK_BUF];
static void TrackSegment(int ch, uint16 idx1, uint16 idx2);
static void ProcessTrackBuf(int num, int indx, int cb);

Void tskTrackFunc (Void)
{
    TRACKMSG msg;
    INTERPOLMSG msgIntpol;
    int cbIdx;
    Uint16 stamp_idx, trk_idx;

    while (1) {
        MBX_pend(&mbxTrack, &msg, SYS_FOREVER);
        cbIdx = msg.cbIdx;
        stamp_idx = cbInfo[cbIdx].stamp_idx;
        trk_idx = cbInfo[cbIdx].trk_idx;
        if (trk_idx == stamp_idx)
            continue;   // no new timestamp yet

        if (stamp_idx > trk_idx) {
            TrackSegment(cbIdx, trk_idx, stamp_idx);
        } else {
            TrackSegment(cbIdx, trk_idx, outPidgInfo[cbIdx].pktInfoSize - 1);
            TrackSegment(cbIdx, 0, stamp_idx);
        }

        // update trk_idx
        cbInfo[cbIdx].trk_idx = stamp_idx;

        // Don't forget to post interpolation here
        msgIntpol.cbIdx = cbIdx;
        MBX_post(&mbxInterpol, &msgIntpol, SYS_FOREVER);
    }
}

static void TrackSegment (int ch, uint16 idx1, uint16 idx2)
{
    // Assumed: idx1 < idx2, process [idx1, idx2]
    int i;
    int nLps, remPkts, nPkts;
    uint32 pktinfoAddr;
    uint16 idx;

    nLps = (idx2 - idx1 + 1) / NPKTS_TRACK_BUF;
    remPkts = (idx2 - idx1 + 1) % NPKTS_TRACK_BUF;
    if (remPkts) {
        nLps++;
    } else {
        remPkts = NPKTS_TRACK_BUF;
    }

    pktinfoAddr = cbInfo[ch].pkt_info + idx1 * sizeof (PKTINFO);
    idx = idx1;

    for (i = 0; i < nLps; i++) {
        if (i == (nLps - 1))
            nPkts = remPkts;
        else
            nPkts = NPKTS_TRACK_BUF;
        // Read in Packet Info
        SEM_new(&semXbHpiTrack, 0);
        PktinfoRd((uint32 *)bufTrack, pktinfoAddr,
                  nPkts * sizeof (PKTINFO) / sizeof (uint32), &semXbHpiTrack);
        // XbHpiRd_Intrpt((uint32 *)bufTrack, pktinfoAddr,
        //     nPkts * sizeof(PKTINFO) / sizeof(uint32), &semXbHpiTrack);

        // Process Tracking Buffer
        SEM_pend(&semXbHpiTrack, SYS_FOREVER);
        ProcessTrackBuf(nPkts, idx, ch);

        // Write Packet Info back to SDRAM
        PktinfoWr((uint32 *)bufTrack, pktinfoAddr,
                  nPkts * sizeof (PKTINFO) / sizeof (uint32), NULL);
        // XbHpiWr_Intrpt((uint32 *)bufTrack, pktinfoAddr,
        //          nPkts * sizeof(PKTINFO) / sizeof(uint32), NULL);

        // update variables for next iteration
        pktinfoAddr = pktinfoAddr + nPkts * sizeof (PKTINFO);
        idx = idx + nPkts;
    }
}

static void ProcessTrackBuf (int num, int indx, int cb)
{

#if 0
    int i, idx;
    int did_init = 0;
    PKTINFO *buf = bufTrack;

    // Don't need to do sanity checking here, already done in the caller

    did_init = 0;
    for (i = 0, idx = indx; i < num; i++, idx++) {

        if (!CLKCFLAG_PRESENT(buf[i]))
            continue;

        // processing for channel buffer in the RESTART state
        if (cbInfo[cb].clkc_state == CLKC_STATE_RESTART) {
            pcrSubPcr_in(&buf[i].arrv, &buf[i].send, &cbInfo[cb].diff0);

            cbInfo[cb].prev_fdl = 0;
            cbInfo[cb].clkc_state = CLKC_STATE_ONGOING;

            // state variables which can be inited from PKTINFO
            TrackLoopStateInit(&buf[i].send, &buf[i].arrv, idx);
            did_init = 1;
            continue;
        }
        // processing for channel buffer already in ONGOING state
        if (!did_init) {
            // use the overlapped first sample to do init;
            TrackLoopStateInit(&buf[i].send, &buf[i].arrv, idx);
            did_init = 1;
        } else {
            TrkLoop(cb, idx, &buf[i].arrv, &buf[i].send);
        }
    }

    return;
#endif

}
