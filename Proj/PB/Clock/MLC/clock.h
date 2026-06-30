/*****************************************************************************
 * Copyright (c) 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : clock.h
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    04-23-01  Jimmy Gou      Created.
 *    12-18-01  FL      replace alias.h by csl.h and oscover.h
 *    02-14-02  FL      IOS coding style
 *
 *****************************************************************************/

#ifndef _H_CLOCK_

#define _H_CLOCK_

#include "mlcdef.h"
#include "csl.h"
#include "oscover.h"

typedef struct mpeg_clock {
    Uint32 pcr45K;      /* Ticks in 45 KHz */
    Uint32 pcr27M;      /* remainder of 27 MHz ticks divided by 45 KHz */
} mpgClock;

typedef mpgClock CLK;
typedef mpgClock Clock;

/* typedef mpgClock Clock; */

/*
 * Constants
 */
#define MSEC_DELAY  500 // 300 ms delay
#define TICK_DELAY  (27*1000*MSEC_DELAY)

/*
 * Macros
 */

#define CLOCK_ASSIGN(lval, rval)  \
    {\
        (lval).pcr45K = (rval).pcr45K;   \
        (lval).pcr27M = (rval).pcr27M; \
    }

/*
 * Function Prototypes
 */
#ifdef DEBUG_PCR_FUNCS

Int32 timeDiff_in(Clock *t0, Clock *t1);
void pcrAddTick_in(Clock *t0, Uint32 ticks, Clock *t1);
void pcrAddPcr_in(Clock *t0, Clock *t1, Clock *t2);
void pcrSubPcr_in(Clock *t0, Clock *t1, Clock *t2);
void tickToPcr_in(Uint32 tick, Clock *t0);
Uint32 pcrGTpcr_in(Clock *t0, Clock *t1);
Uint32 pcrGEpcr_in(Clock *t0, Clock *t1);

#else


inline Int32 timeDiff_in (Clock *t0, Clock *t1)
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
inline void pcrAddTick_in (Clock *t0, Uint32 ticks, Clock *t1)
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
inline void pcrAddPcr_in (Clock *t0, Clock *t1, Clock *t2)
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
inline void pcrSubPcr_in (Clock *t0, Clock *t1, Clock *t2)
{

    // Placeholder: 

}

/* 
 * From 27MHz ticks to PCR.
 *
 */
inline void tickToPcr_in (Uint32 tick, Clock *t0)
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
inline Uint32 pcrGTpcr_in (Clock *t0, Clock *t1)
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
inline Uint32 pcrGEpcr_in (Clock *t0, Clock *t1)
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

inline Uint32 pcrLEpcr_in (Clock *t0, Clock *t1)
{
    return (!pcrGTpcr_in(t0, t1));
}



#endif

/* ------------------------------------------------------------------------ */
/* Code will just copy PCR to another PCR
 */
__INLINE void pcrToPcr(Clock *t0, Clock *t1)
{
    t1->pcr45K = t0->pcr45K;
    t1->pcr27M = t0->pcr27M;
}

/* Code will return truncated 31 bits interger of the time difference
 * between two PCR.  The return value is in 27 MHz ticks.
 */
__INLINE Int32 timeDiff(Clock *t0, Clock *t1)
{
    Int32 pcr45K, pcr27M;
    Int32 rslt;

    pcr27M = (Int32)(t0->pcr27M - t1->pcr27M);
    pcr45K = (Int32)(t0->pcr45K - t1->pcr45K);
    rslt = pcr45K * ((Int16)600) + pcr27M;
    return (rslt);
}

/* Code adds the ticks (in 27 MHz) to PCR.
 *
 * t = t0 + ticks;
 */
__INLINE void pcrAddTick(Clock *t0, Uint32 ticks, Clock *t)
{
    Uint32 pcr27M, carry;

    pcr27M = t0->pcr27M + ticks;
    carry = (pcr27M >= 600) ? (pcr27M / 600) : 0;

    t->pcr27M = pcr27M - (carry * 600);
    t->pcr45K = t0->pcr45K + carry;
}

/* Code adds PCR to PCR.
 *
 * t = t0 + t1;
 */
__INLINE void pcrAddPcr(Clock *t0, Clock *t1, Clock *t)
{
    Uint32 tmp27M, tmp45K;

    tmp27M = t0->pcr27M + t1->pcr27M;
    tmp45K = t0->pcr45K + t1->pcr45K;
    if (tmp27M > 599) {
        tmp27M -= 600;
        tmp45K += 1;
    }

    t->pcr27M = tmp27M;
    t->pcr45K = tmp45K;
}

/* Code adds PCR to PCR.
 *
 * t = t0 - t1;
 */
__INLINE void pcrSubPcr(Clock *t0, Clock *t1, Clock *t)
{
    Uint32 tmp45K, tmp27M;

    tmp27M = t0->pcr27M - t1->pcr27M;
    tmp45K = t0->pcr45K - t1->pcr45K;
    if (tmp27M > 599) {
        tmp27M += 600;
        tmp45K -= 1;
    }

    t->pcr27M = tmp27M;
    t->pcr45K = tmp45K;
}

/* From 27MHz ticks to PCR.
 */
__INLINE void tickToPcr(Uint32 tick, Clock *t)
{
    Uint32 tmp;

    tmp = tick / 600;
    t->pcr27M = tick - (tmp * 600);
    t->pcr45K = tmp;
}

/* PCR compare to PCR --> t0 > t1 ?
 */
__INLINE Uint32 pcrGTpcr(Clock *t0, Clock *t1)
{
    Int32 rslt45K;
    Uint32 rslt;

    rslt45K = (Int32)(t0->pcr45K - t1->pcr45K);
    if (rslt45K != 0)
        rslt = (rslt45K > 0) ? 1 : 0;
    else
        rslt = (t0->pcr27M > t1->pcr27M) ? 1 : 0;
    return (rslt);
}

/* PCR compare to PCR --> t0 > t1 ?
 */
__INLINE Uint32 pcrGEpcr(Clock *t0, Clock *t1)
{
    Int32 rslt45K;
    Uint32 rslt;

    rslt45K = (Int32)(t0->pcr45K - t1->pcr45K);
    if (rslt45K != 0)
        rslt = (rslt45K > 0) ? 1 : 0;
    else
        rslt = (t0->pcr27M >= t1->pcr27M) ? 1 : 0;
    return (rslt);
}

__INLINE Uint32 pcrGTpcrLow(Clock *t0, Clock *t1)
{
    Int32 rslt;

    rslt = (Int32)(t0->pcr45K - t1->pcr45K);
    return ((rslt > 0));
}

__INLINE Uint32 calcTickFromCmnd(Uint32 cmndNumb, Uint32 pRate,
                                 Uint32 pRateFrac)
{
#if PRATE_NEED_SUB
    Uint32 unit = 1;
#else
    Uint32 unit = 0;
#endif
    return (cmndNumb * (pRate - unit) + ((cmndNumb * pRateFrac) >> 18));
}

#endif
