/*****************************************************************************
 * Copyright (c) 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : udpclkc.c
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    11-12-01  Jimmy Gou      Created.
 *    03-13-02  FL      change prvPcr_idx = PktIdx to prvPcr_idx = PktInfoIdx
 *                      change adj_idx = PktIdx to adj_idx = PktInfoIdx
 *                      when detecting PCR, set cbInfo[ch].interpolFlag only
 *                      trigger interpol mailbox later in fetch
 *
 *****************************************************************************/

/***************************************************************************
 *                                                                         
 *     udpclkc.c                                                       
 *                                                                         
 *     Do delivery time adjustment for IP/UDP without RTP timestamp
 *                                                                         
 ***************************************************************************/

/* DSP/BIOS header files */
#include <std.h>
#include <log.h>

#include "cbinfo.h"
#include "swremap.h"
#include "xbhpi.h"
#include "fetch.h"
#include "track.h"
#include "interpol.h"
#include "clock.h"

#include "ns_com_sys.h"
#include "ns_com_mem.h"
#include "ns_com_msg.h"
#include "nsinfo.h"

extern far LOG_Obj trcPoll;
extern far LOG_Obj trace;
extern far LOG_Obj trace2;

extern far LOG_Obj errLog;
extern far LOG_Obj trcTickAdd;

extern int isPcrPresent(Uint32 *Pkt);

Uint32 *g_mlcPkt;
int tmbaseChange = 0;
int tm_gen = 0;
static void detectTimeBaseChange (int ch, Uint32 *mlcPkt)
{
    Uint32 *p = mlcPkt + 4;     // skip 4-word header
    Uint32 tmp;

    g_mlcPkt = mlcPkt;

    tmp = p[0];
    tmp = tmp >> 4;
    tmp = tmp & 0x0003; // adaptation field control

    if ((tmp == 0) || (tmp == 1))
        return; // no adaptation field, return;
    tmp = p[1];
    tmp = tmp >> 24;
    if (tmp == 0)
        return; // adaptation field length is 0

    tmp = p[1];
    tmp = tmp >> 23;
    tmp = tmp & 0x0001;

#ifdef FORCE_1_TIME_BASE_CHANGE
    // manually generate the condition for testing
    if (tm_gen == 1006) {
        tmp = 1;
    }
#endif

    if (tmp) {
        cbInfo[ch].tm_flag = 1; // set the bit only when discon_indicator is 1
        tmbaseChange++;
        LOG_printf(&errLog, "Time base changed: %d times (at ch =%d)",
                   tmbaseChange, ch);

#ifdef TIME_BASE_CHANGE_DEBUG
        {
            // to enable logging PCR value history right before and after time base change
            extern int tm_pcr_cnt;

            tm_pcr_cnt = 0;
        }
#endif

    }

    tm_gen++;

    return;
}

static SEM_Obj sem;
int wwww = 0;

volatile int accessCache = 0;
static int chId;
static Uint16 prevIdx;
static Uint16 currIdx;
static PKTINFO *pPrevInfo;

extern volatile int cache_idx;

void readPrevInfo (void)
{

    SEM_new(&sem, 0);

    if (accessCache) {
        //still in cache, read from cache
        PKTINFO *cache = pktInfoCache();
        static int counter = 0;

        //int idx = prevIdx - cbInfo[chId].cache_idx;
        int idx = prevIdx - cache_idx;

        CLOCK_ASSIGN(pPrevInfo->arrv, cache[idx].arrv);
        CLOCK_ASSIGN(pPrevInfo->send, cache[idx].send);
        CLOCK_ASSIGN(pPrevInfo->pcr, cache[idx].pcr);
        pPrevInfo->flag = cache[idx].flag;
        pPrevInfo->pid = cache[idx].pid;

#ifdef PKTINFO_40BYTES
        pPrevInfo->pidtype = cache[idx].pidtype;
#endif

        SEM_post(&sem);

        counter++;
        //        LOG_printf(&errLog, "direct read = %d", counter);
    } else {

        Uint32 infoAddr;

        infoAddr = cbInfo[chId].pkt_info + prevIdx * sizeof (PKTINFO);
        PktinfoRd((uint32 *)pPrevInfo, infoAddr,
                  1 * sizeof (PKTINFO) / sizeof (uint32), &sem);

        // delete the sellp, which caused great troubles and took a long time to pinpoint
        //TSK_sleep(2);

    }

    SEM_pend(&sem, SYS_FOREVER);

}

void writePrevInfo (void)
{
    SEM_new(&sem, 0);

    if (accessCache) {
        // if( (prevIdx >= cbInfo[chId].cache_idx) && (prevIdx < currIdx) ) {
        //still in cache, write to cache
        PKTINFO *cache = pktInfoCache();
        static int counter = 0;

        // int idx = prevIdx - cbInfo[chId].cache_idx;
        int idx = prevIdx - cache_idx;

        CLOCK_ASSIGN(cache[idx].arrv, pPrevInfo->arrv);
        CLOCK_ASSIGN(cache[idx].send, pPrevInfo->send);
        CLOCK_ASSIGN(cache[idx].pcr, pPrevInfo->pcr);
        cache[idx].flag = pPrevInfo->flag;
        cache[idx].pid = pPrevInfo->pid;

#ifdef PKTINFO_40BYTES
        cache[idx].pidtype = pPrevInfo->pidtype;
#endif

        SEM_post(&sem);
        counter++;
        //        LOG_printf(&errLog, "direct write = %d", counter);
    } else {
        Uint32 infoAddr;

        infoAddr = cbInfo[chId].pkt_info + prevIdx * sizeof (PKTINFO);
        PktinfoWr((uint32 *)pPrevInfo, infoAddr,
                  1 * sizeof (PKTINFO) / sizeof (uint32), &sem);

        // delete the sellp, which caused great troubles and took a long time to pinpoint
        // TSK_sleep(2);
    }

    SEM_pend(&sem, SYS_FOREVER);

}

#pragma DATA_ALIGN(prevInfo, 8)
PKTINFO prevInfo;       // debug by Jimmy on 2/15/02

#ifdef DEBUG_INDPF
// debug per-flow index
int16 glb_prv_indpf;
#endif

// Do delivery time adjustment for IP/UDP without RTP timestamp
void doUdpClkc (int ch, int PktIdx, Uint32 *Pkt, PKTINFO *PktInfo)
{

    if (cbInfo[ch].prvPcr_idx == INVALID_PCR_INDEX) {

        cbInfo[ch].prev_dl_hi = 0;
        cbInfo[ch].prev_dl_lo = 0;
        cbInfo[ch].prev_rate_hi = 0;
        cbInfo[ch].prev_rate_lo = 0;
        cbInfo[ch].prev_fdl_hi = 0;
        cbInfo[ch].prev_fdl_lo = 0;

        if (isPcrPresent(Pkt)) {
            cbInfo[ch].prvPcr_idx = PktIdx;
            cbInfo[ch].pktCnt = 1;

#ifndef IGNORE_MPTS_INDPF
            if (IS_CHTYPE_IP_MPTS(ch)) {
                cbInfo[ch].prv_indpf = PktInfo->indpf;
            }
#endif

            SET_CLKCFLAG(*PktInfo);
            TICK_INC(*PktInfo) = TICKINC_INVALID;
        } else {
            TICK_INC(*PktInfo) = TICKINC_INVALID;
        }
    } else {
        if (isPcrPresent(Pkt)) {
            // PKTINFO prevInfo;
            int diff;
            Uint32 tick_inc;

            Uint16 prev = cbInfo[ch].prvPcr_idx;
            Uint16 num = cbInfo[ch].pktCnt;

            SET_CLKCFLAG(*PktInfo);

            cbInfo[ch].prvPcr_idx = PktIdx;
            cbInfo[ch].pktCnt = 1;

            chId = ch;
            prevIdx = prev;
            currIdx = PktIdx;
            pPrevInfo = &prevInfo;

            if ((prevIdx >= cache_idx) && (prevIdx < currIdx)) {
                accessCache = 1;
            } else {
                accessCache = 0;
            }

            readPrevInfo();

            detectTimeBaseChange(ch, Pkt);
            if (cbInfo[ch].tm_flag) {

                // tick_inc = TICK_INC(prevInfo);
                // ticks = tick_inc * num;
                // pcrAddTick_in(&prevInfo.arrv, ticks, &PktInfo->arrv);

                // if this happens, just use the arrvial time as the deliv time
                // may cause out of order, but that's the price to pay 
                TICK_INC(*PktInfo) = TICKINC_INVALID;

                // don't forget to reset the time base flag to zero
                cbInfo[ch].tm_flag = 0;

                // change gap between last pcr and current pcr pkts to be zero
                TICK_INC(prevInfo) = 0;
                writePrevInfo();

            } else {
                {
                    extern void doAdjustment(int cb, int currIdx, Clock *arrv,
                                             Clock *send, Clock *pdeliv);
                    extern Clock orig_send;
                    extern Clock orig_arrv;
                    Clock arrv, send, deliv;

                    CLOCK_ASSIGN(send, orig_send);
                    CLOCK_ASSIGN(arrv, orig_arrv);

                    doAdjustment(ch, PktIdx, &arrv, &send, &deliv);

                    CLOCK_ASSIGN(PktInfo->arrv, deliv)
                        diff = timeDiff_in(&PktInfo->arrv, &prevInfo.arrv);

#ifndef IGNORE_MPTS_INDPF
                    if (IS_CHTYPE_IP_MPTS(ch)) {
                        int tmpnnn = (PktInfo->indpf - cbInfo[ch].prv_indpf)&0x00003fff;        // 14-bit per-flow index

                        tick_inc = diff / tmpnnn;

                    } else {
                        tick_inc = diff / num;
                    }
#else
                    tick_inc = diff / num;
#endif

                }

                //=======================

                TICK_INC(prevInfo) = tick_inc;

                writePrevInfo();

            }

            // update adj_idx and post interpol task
            // cbInfo[ch].adj_idx = prev;
            cbInfo[ch].adj_idx = PktIdx;
            cbInfo[ch].interpolFlag = INTERPOL_SET;

#ifdef DEBUG_INDPF
            {
                LOG_printf(&trcTickAdd, "prev = %d  curr==>%d",
                           cbInfo[ch].prv_indpf, PktInfo->indpf);
            }
#endif

            cbInfo[ch].prv_indpf = PktInfo->indpf;

#ifdef DEBUG_INDPF
            glb_prv_indpf = PktInfo->indpf;
#endif

        } else {
            TICK_INC(*PktInfo) = TICKINC_PADDING;

            (cbInfo[ch].pktCnt)++;

        }
    }

    return;

}
