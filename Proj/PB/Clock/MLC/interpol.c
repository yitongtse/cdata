/*****************************************************************************
 * Copyright (c) 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : interpol.c
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    05-11-01  Jimmy Gou      Created.
 *
 *****************************************************************************/

/***************************************************************************
 *                                                                         
 *     interpol.c                                                       
 *                                                                         
 *     Do interpolation for packets between two packets which can do clock 
 *     drifting compensation.  
 *                                                     
 ***************************************************************************/

#include <assert.h>
#include <std.h>
#include <tsk.h>
#include <log.h>

#include "cbinfo.h"
#include "clock.h"
#include "interpol.h"

extern far LOG_Obj trcPoll;
extern far LOG_Obj trcTickAdd;

#pragma DATA_ALIGN(bufInterpol, 8)
static far PKTINFO bufInterpol[NPKTS_INTERPOL_BUF];
static void InterpolSegment(int ch, uint16 idx1, uint16 idx2);
static void ProcessInterpolBuf(int num, int indx, int cb);

Void tskInterpolFunc(Void)
{
    INTERPOLMSG msg;
    int cbIdx;
    Uint16 trk_idx, intp_idx;
    
    while(1) {
        MBX_pend(&mbxInterpol, &msg, SYS_FOREVER);
        cbIdx = msg.cbIdx;
        
        if(IS_IP_NO_RTP(cbInfo[cbIdx].io_mask) ) {
            trk_idx = cbInfo[cbIdx].adj_idx;
        } else {
            trk_idx = cbInfo[cbIdx].trk_idx;
        }           
    
        intp_idx = cbInfo[cbIdx].intp_idx;

        if(trk_idx == intp_idx) continue; // nothing new, do nothing

        // LOG_printf(&trcPoll, "===============>trk_idx=%u intp_idx = %u", trk_idx, intp_idx);

        if(trk_idx > intp_idx) {
            InterpolSegment(cbIdx, intp_idx, trk_idx);
        } else {
            // always do this segment
            InterpolSegment(cbIdx, intp_idx, cbInfo[cbIdx].len);
            if(trk_idx) InterpolSegment(cbIdx, 0, trk_idx); 
        }

        // update trk_idx
        cbInfo[cbIdx].intp_idx = trk_idx;
    }
}


Void doInterpol(int cbIdx)
{
    Uint16 trk_idx, intp_idx;
    
        
        if(IS_IP_NO_RTP(cbInfo[cbIdx].io_mask) ) {
            trk_idx = cbInfo[cbIdx].adj_idx;
        } else {
            trk_idx = cbInfo[cbIdx].trk_idx;
        }           
    
        intp_idx = cbInfo[cbIdx].intp_idx;

        // if(trk_idx == intp_idx) continue; // nothing new, do nothing
        if(trk_idx == intp_idx) return; // nothing new, do nothing


        // LOG_printf(&trcPoll, "===============>trk_idx=%u intp_idx = %u", trk_idx, intp_idx);

        if(trk_idx > intp_idx) {
            InterpolSegment(cbIdx, intp_idx, trk_idx);
        } else {
            // always do this segment
            InterpolSegment(cbIdx, intp_idx, cbInfo[cbIdx].len);
            if(trk_idx) InterpolSegment(cbIdx, 0, trk_idx); 
        }

        // update trk_idx
        cbInfo[cbIdx].intp_idx = trk_idx;
}







static void InterpolSegment(int ch, uint16 idx1, uint16 idx2)
{
    // Assumed: idx1 < idx2, process [idx1, idx2)
    int i;
    int nLps, remPkts, nPkts;
    uint32 pktinfoAddr;
    uint16 idx;
    
    
    nLps = (idx2 - idx1 ) / NPKTS_INTERPOL_BUF;
    remPkts = (idx2 - idx1) % NPKTS_INTERPOL_BUF;
    if(remPkts) {
        nLps++;
    } else {        
        remPkts = NPKTS_INTERPOL_BUF;
    }       



    
    pktinfoAddr = cbInfo[ch].pkt_info + idx1 * sizeof(PKTINFO);
    idx = idx1;
    
    for(i=0; i<nLps; i++) {
    	SEM_Obj *pSem;
    	
        if(i == (nLps-1) ) { nPkts = remPkts; pSem = &semXbHpiInterpolWr; }
        else { nPkts = NPKTS_INTERPOL_BUF; pSem = NULL; }
        
        // Read in Packet Info
        SEM_new(&semXbHpiInterpol, 0);
        PktinfoRd((uint32 *)bufInterpol, pktinfoAddr, 
            nPkts * sizeof(PKTINFO) / sizeof(uint32), &semXbHpiInterpol);
        // XbHpiRd_Intrpt((uint32 *)bufInterpol, pktinfoAddr, 
        //     nPkts * sizeof(PKTINFO) / sizeof(uint32), &semXbHpiInterpol);
            
        // Process Interpol Buffer
        SEM_pend(&semXbHpiInterpol, SYS_FOREVER);
        ProcessInterpolBuf(nPkts, idx, ch);

        // Write Packet Info back to SDRAM
        PktinfoWr((uint32 *)bufInterpol, pktinfoAddr, 
            nPkts * sizeof(PKTINFO) / sizeof(uint32), pSem);
        // XbHpiWr_Intrpt((uint32 *)bufInterpol, pktinfoAddr, 
        //     nPkts * sizeof(PKTINFO) / sizeof(uint32), NULL);

        // update variables for next iteration
        pktinfoAddr = pktinfoAddr + nPkts * sizeof(PKTINFO);
        idx = idx + nPkts;
    }
    
    SEM_pend(&semXbHpiInterpolWr, SYS_FOREVER);
}   
        
            
static void ProcessInterpolBuf(int num, int indx, int cb)
{
    int i, idx;
    PKTINFO *buf;

    // since every calling to Interpolation is always start at index with 
    // new timestamp, this should be no problem. The key word static is used  
    // to keep state since ProcessInterpolBuf() may be invoked multiple 
    // times each time Interpol Task is triggered. 
    static Clock prev_arrv;
    static Uint32 tick_inc = TICKINC_INVALID;
    static int indpf; // per-flow index
    Uint32 tick_add;

    if(IS_IP_NO_RTP(cbInfo[cb].io_mask)) {
    // ==========================================
    
    buf = bufInterpol;
    for(i=0, idx=indx; i<num; i++, idx++) {
        Uint32 tick_inc_tmp;
         
        tick_inc_tmp= TICK_INC(buf[i]);

        if(CLKCFLAG_PRESENT(buf[i])) {

#ifdef USE_MPTS_INDPF        
        	indpf = buf[i].indpf;     // meaningful for MPTS channel only
#endif        	
            tick_inc = tick_inc_tmp;
            CLOCK_ASSIGN(prev_arrv, buf[i].arrv);
            
            //LOG_printf(&trcPoll, "~~~~~~~~~~~~~~~~~~~==>%u", tick_inc);
            continue; // this pkt is compst'd by tracking, don't do interpol 
        } 

        // this can only happen at the very beginning when a channel is set up and 
        // no PCR packets come yet. Otherwise the tick inc will be set to 
        // TICKINC_PADDING for non-PCR packet in pktinfo extraction
        if(tick_inc_tmp == TICKINC_INVALID) continue;


		if(IS_CHTYPE_MPTS(cb) ) {
			int cnt = (buf[i].indpf - indpf) & 0x00003fff; // 14-bit per-flow index
			tick_add = tick_inc * cnt;
            // LOG_printf(&trcTickAdd, "~~~~~~~~~~~~~~~~~~~==>%d", tick_add);
            indpf = buf[i].indpf;
		} else {
			tick_add = tick_inc;
		}

        // LOG_printf(&trcTickAdd, "~~~~~~~~~~~~~~~~~~~==>%d", tick_add);
		
        // normal operation
        pcrAddTick_in(&prev_arrv, tick_add, &buf[i].arrv);
        CLOCK_ASSIGN(prev_arrv, buf[i].arrv);

    }

    } else {
    //===========================================================

    buf = bufInterpol;
    for(i=0, idx=indx; i<num; i++, idx++) {

        if(CLKCFLAG_PRESENT(buf[i])) {
            tick_inc = TICK_INC(buf[i]);
            CLOCK_ASSIGN(prev_arrv, buf[i].arrv);
            continue; // this pkt is compst'd by tracking, don't do interpol 
        } 

        // this can only happen at the very beginning when a channel is set up; 
        if(tick_inc == TICKINC_INVALID) continue;
    
        // normal operation
        pcrAddTick_in(&prev_arrv, tick_inc, &buf[i].arrv);
        CLOCK_ASSIGN(prev_arrv, buf[i].arrv);
    }
    
    }
    //=======================================================================

}


