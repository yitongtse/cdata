/*****************************************************************************
 * Copyright (c) 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : clock.c
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    04-23-01  Jimmy Gou      Created.
 *    02-19-02  FL      IOS coding style
 *
 *****************************************************************************/

#include "clock.h"

#ifdef DEBUG_PCR_FUNCS
/*
 * Code will return truncated 31 interger of the time
 * difference between two PCR.  The return value is in
 * 27 MHz ticks.
 */
Int32 timeDiff_in (Clock *t0, Clock *t1)
{
    Int32 pcrH, pcrL, carry;
    Int32 rslt;

    carry = 0;
    if (t0->pcr27M < t1->pcr27M) {
        t0->pcr27M += 600;
        carry = 1;
    }
    pcrH = t0->pcr27M - t1->pcr27M;
    pcrL = (Int32)(t0->pcr45K - t1->pcr45K);

    rslt = (pcrL - carry) * 600 + pcrH;

    // restore t0
    if (carry)
        t0->pcr27M -= 600;

    return (rslt);
}

/*
 * Code adds the ticks (in 27 MHz) to PCR.
 *
 * t1 = t0 + ticks;
 */
void pcrAddTick_in (Clock *t0, Uint32 ticks, Clock *t1)
{
#if 1
    Uint32 tmp;

    tmp = t0->pcr27M + ticks;
    tmp = (tmp >= 600) ? (tmp / 600) : 0;

    t1->pcr27M = t0->pcr27M + ticks - (tmp * 600);
    t1->pcr45K = t0->pcr45K + tmp;

    if (t1->pcr27M >= 600) {
        t1->pcr27M -= 600;
        t1->pcr45K++;
    }

#else
    t1->pcr45K = t0->pcr45K;
    t1->pcr27M = t0->pcr27M + ticks;

    while (t1->pcr27M >= 600) {
        t1->pcr27M -= 600;
        t1->pcr45K++;
    }
#endif

}

/*
 * Code adds PCR to PCR.
 *
 * t2 = t0 + t1;
 */
void pcrAddPcr_in (Clock *t0, Clock *t1, Clock *t2)
{
    t2->pcr27M = t0->pcr27M + t1->pcr27M;
    t2->pcr45K = t0->pcr45K + t1->pcr45K;

    if (t2->pcr27M >= 600) {
        t2->pcr27M -= 600;
        t2->pcr45K++;
    }
}

/*
 * Code to sub PCR to PCR.
 *
 * t2 = t0 - t1;
 */
void pcrSubPcr_in (Clock *t0, Clock *t1, Clock *t2)
{

    // Placeholder: 

}

/* 
 * From 27MHz ticks to PCR.
 *
 */
void tickToPcr_in (Uint32 tick, Clock *t0)
{
    Uint32 tmp;

    tmp = tick / 600;
    t0->pcr27M = tick - (tmp * 600);
    t0->pcr45K = tmp;

}

/* 
 * PCR compare to PCR --> t0 > t1 ?
 *
 */
Uint32 pcrGTpcr_in (Clock *t0, Clock *t1)
{
    Int32 rsltLow, rsltHigh;
    Uint32 rslt;

    rsltLow = (Int32)(t0->pcr45K - t1->pcr45K);
    rsltHigh = (Int32)(t0->pcr27M - t1->pcr27M);

    if (rsltLow == 0)
        rslt = (rsltHigh > 0) ? 1 : 0;
    else
        rslt = (rsltLow > 0) ? 1 : 0;
    return (rslt);
}

/* 
 * PCR compare to PCR --> t0 >= t1 ?
 *
 */
Uint32 pcrGEpcr_in (Clock *t0, Clock *t1)
{
    Int32 rsltLow;
    Uint32 rslt;

    rsltLow = (Int32)(t0->pcr45K - t1->pcr45K);

    if (rsltLow == 0)
        rslt = (t0->pcr27M >= t1->pcr27M) ? 1 : 0;
    else
        rslt = (rsltLow > 0) ? 1 : 0;
    return (rslt);
}

Uint32 pcrLEpcr_in (Clock *t0, Clock *t1)
{
    return (!pcrGTpcr_in(t0, t1));
}

#endif
