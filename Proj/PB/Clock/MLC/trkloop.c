/*****************************************************************************
 * Copyright (c) 2001, 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : trkloop.c
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    05-11-01  Jimmy Gou      Created.
 *
 *****************************************************************************/

/***************************************************************************
 *                                                                         
 *     trkloop.c                                                       
 *                                                                         
 *     The Tracking Loop (including the IIR) of the Clock Drifting       
 *     Compensation 
 *                                                     
 ***************************************************************************/

#include "clock.h"
#include "trkloop.h"


// #define  DEBUG_64BIT	1
int debug_dummy=0;

/*
 * state variables which can be inited from PKTINFO array
 */
static Clock prev_deliv;
static Clock prev_send;
static int	prev_trkIndx;

// inline Int32 RshiftInt64(Int32 Hi, Uint32 Lo, int nbits);
// inline Int32 LshiftInt32(Int32 number, int nbits);
// inline void Mul32(Int32 op1, Int32 op2, Int32 *prdtH, Uint32 *prdtL);

void TrackLoopStateInit(Clock *send, Clock *deliv, int indx)
{
	CLOCK_ASSIGN(prev_send, *send);  // for tracking loop use
	CLOCK_ASSIGN(prev_deliv, *deliv); // for tracking loop use
	prev_trkIndx = indx;
}


void TrkLoop(int cb, int indx, Clock *arrv, Clock *send)
{
#if 0
	Clock diff;
	Int32 dl;
	Int32 y;
	Int32 fdl;
	Int32 x;
	int pw2;
	Int32 yoH; // High 32 bits
	Uint32 yoL; // Low 32 bits
	Int32 aprev, a;
	Int32 tmp1, tmp2;

	pcrSubPcr_in(arrv, send, &diff);
	dl = timeDiff_in(&diff, &cbInfo[cb].diff0);
	
	// The IIR
	y = (dl - cbInfo[cb]. prev_fdl) >> ABS_EXPO_ALPHA;
	fdl = cbInfo[cb]. prev_fdl + y;

	/*
	 * Note: 32-lmbd(x) and  ceil(log2(x)) is not the same when x=2^m,
	 * In other case, they are the same.
	 */	
	x = timeDiff_in(send, &prev_send);
	pw2 = 32 - _lmbd(0, x);  // try to approximate: pw2 = ceil(log2(x));


	aprev = cbInfo[cb].prev_rate;
	Mul32(aprev, x, &yoH, &yoL);
	
	tmp1 = RshiftInt64(yoH, yoL, ABS_EXPO_BETA + pw2);
	tmp2 = LshiftInt32(y, EXPO_ASCALE + EXPO_BETA -pw2); // rshift if neg num
	a = aprev + (tmp2 - tmp1);
	

	// to do: calculate the incremental for interpolation 
	// and store back to the PKTINFO buffer of the previous trked pkt
	// to do: calculate, *tick_inc = 
	
	
	// To do: need special processing for degenerate state, 
	

	
	// to do: calculate the deliv time;
	// and store back to the PKTINFO buffer
	 		 		
	
	// update state variables
	CLOCK_ASSIGN(prev_deliv, *arrv); // for tracking loop use
	CLOCK_ASSIGN(prev_send, *send);  // for tracking loop use
	prev_trkIndx = indx;
	cbInfo[cb].prev_rate = a;
	cbInfo[cb]. prev_fdl = fdl;
#endif
	
}

inline int lmbdInt32(int x)
{
	int i;
	int prev_bit = (x>>31) & 0x0001;
	int curr_bit;
	int retval = 0;
	
	for(i=30; i>=0; i--) {
		curr_bit = (x>>i) & 0x0001;
		if(curr_bit != prev_bit) break;
		
		prev_bit = curr_bit;
		retval++;
	}
	
	return retval;
}
		
	
// y = (Int64)x
static inline void	int32ToInt64(Int32 x, Int32 *y_hi, Uint32 *y_lo)
{
	*y_lo = x;
	if(x>=0) *y_hi = 0;
	else *y_hi = 0xffffffff;
}
	


// z= x + y
inline void int64AddInt64(Int32 x_hi, Uint32 x_lo, Int32 y_hi, Uint32 y_lo, 
		Int32 *z_hi, Uint32 *z_lo)
{
	unsigned long tmp; // TI's 40-bit data type
	
	tmp =  (unsigned long)x_lo + (unsigned long)y_lo;
	*z_lo = (Uint32)tmp;
	
	*z_hi = (Int32)(tmp>>32);  // get the carry_in
	*z_hi = (*z_hi) + x_hi;
	*z_hi = (*z_hi) + y_hi;
}


// z= x - y,
inline void int64SubInt64(Int32 x_hi, Uint32 x_lo, Int32 y_hi, Uint32 y_lo, 
		Int32 *z_hi, Uint32 *z_lo)
{
	unsigned long tmp; // TI's 40-bit data type


	// y = -y	
	y_hi = ~y_hi;
	y_lo = ~y_lo;
	if(y_lo != 0xffffffff) {
		y_lo += 1;
	} else {
		y_lo = 0;
		y_hi += 1;
	}
	
	
	// x + (-y)
	tmp =  (unsigned long)x_lo + (unsigned long)y_lo;
	*z_lo = (Uint32)tmp;
	
	*z_hi = (Int32)(tmp>>32);  // get the carry_in
	*z_hi = (*z_hi) + x_hi;
	*z_hi = (*z_hi) + y_hi;
}




inline void	int64Lshift(Int32 x_hi, Uint32 x_lo, int nbits, Int32 *y_hi, Uint32 *y_lo)
{
	Uint32 utmp;
	
#ifdef DEBUG_64BIT
	// sanity checking
	if(nbits < 0)  {
		debug_dummy++;
	}
#endif

	if(nbits == 0) {
		*y_lo = x_lo;
		*y_hi = x_hi;
		return;
	}

	if(nbits >= 64) {
		*y_lo = 0;
		*y_hi = 0;
		return;
	}
	
	if(nbits >= 32) {
		*y_lo = 0;
		*y_hi = x_lo << (nbits-32);
	} else { // nbits < 32 
		*y_lo = x_lo << nbits;

		// got high nbits from x_lo;
		utmp = x_lo >> (32-nbits);
		*y_hi = x_hi << nbits;
		*y_hi |= utmp;
	}
	
	return;
}


// y = x >> nbits
inline void	int64Rshift(Int32 x_hi, Uint32 x_lo, int nbits, Int32 *y_hi, Uint32 *y_lo)
{
	Uint32 tmp;

#ifdef DEBUG_64BIT
	// sanity checking
	if(nbits < 0) {
		debug_dummy++;
	}
#endif

	if(nbits == 0) {
		*y_lo = x_lo;
		*y_hi = x_hi;
		return;
	}
	
	if(nbits >=64) {
		if(x_hi >= 0) {
			*y_lo = 0;
			*y_hi = 0;
		} else {
			*y_lo = 0xffffffff;
			*y_hi = 0xffffffff;
		}
	
		return;
	}


	
	if(nbits <= 32) {

		*y_lo = x_lo >> nbits;
		tmp = x_hi;
		tmp = ( tmp << (32-nbits) );
		*y_lo |= tmp;
		
		*y_hi = x_hi >> nbits;
	} else {
		*y_lo = ( x_hi >> (nbits-32));
		if(x_hi < 0 ) *y_hi = 0xffffffff;
		else *y_hi = 0x00000000;
	}
		
}




// y = Rounding (x >> nbits)
inline void	int64RshiftRnd(Int32 x_hi, Uint32 x_lo, int nbits, Int32 *y_hi, Uint32 *y_lo)
{
	Int32 tmp_hi;
	Uint32 tmp_lo;
	Int32 z_hi;
	Uint32 z_lo;


#ifdef DEBUG_64BIT
	// sanity checking
	if(nbits < 0) {
		debug_dummy++;
	}
#endif
	
	if(nbits == 0) {
		*y_lo = x_lo;
		*y_hi = x_hi;
		return;
	}
	
	if(nbits >=64) {
		*y_lo = 0;
		*y_hi = 0;
		return;
	}
	
	
	if(nbits <= 32) {
		tmp_lo = (Uint32)1 << (nbits-1);
		tmp_hi = 0;
	} else{
		tmp_lo = 0;
		tmp_hi = (Uint32)1 << (nbits-33);
	}
		
	int64AddInt64(x_hi, x_lo, tmp_hi, tmp_lo, &z_hi, &z_lo);
	int64Rshift(z_hi, z_lo, nbits, y_hi, y_lo);
}

	

//	calculate mant and pw where x = mant * exp(2, -pw)
//  if x is beyond 32 bit range, pw will be negative
inline int64Normalize(Int32 x_hi, Uint32 x_lo, int *pw, Int32 *mant)
{

	if( (x_hi==0) || (x_hi==0xffffffff) ) { // high 32-bits are all sign bits
		*pw = lmbdInt32(x_lo);
		*mant = x_lo << (*pw);
	} else {
		int tmp_pw;
		Int32 tmp_hi;
		Uint32 tmp_lo;
		
		tmp_pw = lmbdInt32(x_hi);
		int64Lshift(x_hi, x_lo, tmp_pw, &tmp_hi, &tmp_lo);
		*pw = tmp_pw - 33;		
		*mant = tmp_hi;
	}
}
		



inline Int32 RshiftInt64(Int32 Hi, Uint32 Lo, int nbits)
{
	Int32 retval;
	// right shift n bits, nbits supposed to be 0 or positive number
	// after that supposedly the High 32 bits will all be sign bits

	if(nbits <= 32 ) {	
		Lo = Lo >> nbits;
		Hi = Hi << (32-nbits);
		retval =  Hi | Lo;

	} else {
		retval =  Hi >> (nbits - 32);
	}

	return retval;
}


inline Int32 LshiftInt32(Int32 number, int nbits)
{
	Int32 val;
	
	if(nbits >= 0) val = number << nbits;
	else val = number >> (-nbits);
	
	return val;
}

int xx, yy;

inline void Mul32(Int32 x, Int32 y, Int32 *prdtH, Uint32 *prdtL)
{

#if 0
	{
	// tough to debug, leave it later
	// ======================================
	// test case: x = 0xaaaaaaaa y=0xbbbbbbbb
	// product: 0x16c1 6c17 2d82 d82e
	unsigned long tmp;
	Uint32 xlyl;
	Int32 xhyl, xlyh, xhyh;
	
	xlyl = _mpyu(x, y);
	xhyl = _mpyhslu(x, y);
	xlyh = _mpyluhs(x, y);
	xhyh = _mpyh(x, y);
	
	tmp = (unsigned long)xlyl;
	tmp = tmp + (unsigned long)((Uint32)xhyl << 16);
	tmp = tmp + (unsigned long)((Uint32)xlyh << 16);
	
	*prdtL = (Uint32)tmp;  // only the low 32 bits;
	
	*prdtH = tmp >> 32;		// the upper 8-bits used as carry-in
	*prdtH = (*prdtH) + (xhyl >> 16);
	*prdtH = (*prdtH) + (xlyh >> 16);
	*prdtH = (*prdtH) + xhyh;
	}
	
#else

	{

	unsigned long tmp;
	Uint32 xlyl;
	Uint32 xhyl, xlyh, xhyh;
	Uint32 xl, yl, xh, yh;
	Uint32 hi;
	

	int sgnx, sgny, sgn;
	
	if( x > 0) sgnx = 1;
	else if( x==0) sgnx = 0;
	else sgnx = -1;

	if( y > 0) sgny = 1;
	else if( y==0) sgny = 0;
	else sgny = -1;
	
	sgn = sgnx * sgny;
	
	if(sgn==0) {
		*prdtH = 0;
		*prdtL = 0;
		return;
	}
	
	xx = x;
	yy = y;
	
	if(sgnx == -1) x = -x;
	if(sgny == -1) y = -y;


	xl = x & 0x0000ffff;
	xh = x>>16;
	yl = y & 0x0000ffff;
	yh = y>>16;
	
	xlyl = xl * yl;;
	xhyl = xh * yl;
	xlyh = xl * yh;
	xhyh = xh * yh;
	
	tmp = (unsigned long)xlyl;
	tmp = tmp + (unsigned long)((Uint32)xhyl << 16);
	tmp = tmp + (unsigned long)((Uint32)xlyh << 16);
	
	*prdtL = (Uint32)tmp;  // only the low 32 bits;
	
	hi = (Uint32)(tmp >> 32);		// the upper 8-bits used as carry-in
	hi = hi + ((Uint32)xhyl >> 16);
	hi = hi + ((Uint32)xlyh >> 16);
	hi = hi + xhyh;
	
	*prdtH = hi;
	
	if(sgn==-1) {
		*prdtL = ~(*prdtL);
		*prdtH = ~(*prdtH);
		
		if( *prdtL == 0xffffffff) {
			*prdtL = 0x00000000;
			*prdtH = (*prdtH) + 1;
		} else {
			*prdtL = (*prdtL) + 1;
		}
	}
	
	}

#endif
	
}
	



	

#ifdef DEBUG_TRKLOOP

int pcr_wrap_around =0;
int arrv_jump =0;
int diff_too_big=0;

#define NUM 64
int num=0;
Clock prevArrv[NUM];
Clock prevSend[NUM];
Clock prevDeliv[NUM];
Clock prevDeliv2[NUM];
int prevDiff[NUM];

char branch[NUM];
int d_send[NUM];
int d_arrv[NUM];


Clock prev_deliv1[NUM];
Clock prev_deliv2[NUM];
Clock val_pdeliv[NUM];
int xdft[NUM];

int jitter[NUM];
int jit_jump =0;

// int prev_deliv_fract=0;

int loop_id = 0;

#endif



#define BIG_JUMP  (27000L * 400L )
#define BIG_DIFF   (27000L * 400L)	
#define BIG_JIT   (27000L * 100L)	
#define BIG_RATE   (200L * 1000000L)	

void doAdjustment(int cb, int currIdx, Clock *arrv, Clock *send, Clock *pdeliv)
{
	Int32 x;
	int pw1, pw2;
	int dft0, dft;
	Int32 yoH; // High 32 bits
	Uint32 yoL; // Low 32 bits
	Int32 aprev;
	Int32 tmp1, tmp2;
	
	Int32 tmp64_hi, tmp64_hi_2, tmp64_hi_3, dl_hi, y_hi, a_hi, fdl_hi;
	Uint32 tmp64_lo, tmp64_lo_2, tmp64_lo_3, dl_lo, y_lo, a_lo, fdl_lo;
	int nbits;

	int jt;

        /// Compute source timestamp gap ds(n)
	tmp2 = timeDiff_in(send, &cbInfo[cb].prev_send);
	x = tmp2;

        /// Compute arrival timestamp gap da(n)
	tmp1 = timeDiff_in(arrv, &cbInfo[cb].prev_arrv);

	// detect jitter
	{
            // extern far LOG_Obj jitLog;
            /// -das(n)
            jt = x - tmp1;
	}

	if( (jt <= BIG_JIT) && (jt > -BIG_JIT) ) {
	
        /// accumulated drift  d(n)
	// Implement dl = cbInfo[cb].prev_dl + tmp1 - tmp2;
	int32ToInt64(tmp1-tmp2, &tmp64_hi, &tmp64_lo);
	int64AddInt64(tmp64_hi, tmp64_lo, cbInfo[cb].prev_dl_hi, 
			cbInfo[cb].prev_dl_lo, &dl_hi, &dl_lo);
	
        /// f(n): gap difference smoother
	// Implement y = ((dl << IIR_SCALE) - cbInfo[cb]. prev_fdl) >> ABS_EXPO_ALPHA;
	int64Lshift(dl_hi, dl_lo, IIR_SCALE, &tmp64_hi, &tmp64_lo);
	int64SubInt64(tmp64_hi, tmp64_lo, cbInfo[cb].prev_fdl_hi, 
			cbInfo[cb].prev_fdl_lo, &tmp64_hi_2, &tmp64_lo_2);
	int64RshiftRnd(tmp64_hi_2, tmp64_lo_2, ABS_EXPO_ALPHA, &y_hi, &y_lo);

	// Implement fdl = cbInfo[cb]. prev_fdl + y;
	int64AddInt64(cbInfo[cb].prev_fdl_hi, cbInfo[cb].prev_fdl_lo, y_hi, y_lo,
			&fdl_hi, &fdl_lo);

	// if(cb==0) LOG_printf(&trcPoll, "~~~~~~~~~~==========>fdl_hi=%d fdl_lo=%d ", fdl_hi, fdl_lo);

	/*
	 * Note: 32-lmbd(x) and  ceil(log2(x)) is not the same when x=2^m,
	 * In other case, they are the same.
	 */	
	pw2 = lmbdInt32(x); // do my own normalization
	
	// Implement: Normalization of Int64
	// rate is assumed to be scaled by 2^EXPO_ASCALE
	// pw1 will be negative if it's beyond 32-bit range
	int64Normalize(cbInfo[cb].prev_rate_hi, cbInfo[cb].prev_rate_lo, &pw1, &aprev);

	Mul32(aprev, x<<pw2, &yoH, &yoL);

	// Implement the following calculations:
	// tmp1 = RshiftInt64(yoH, yoL, ABS_EXPO_BETA + pw2 + pw1 - IIR_SCALE );
	// tmp2 = Lshift(y, EXPO_ASCALE + EXPO_BETA ); // rshift if neg num
	// a = aprev + ( (tmp2 - tmp1) >> (pw2 + IIR_SCALE) );

	// dft = RshiftInt64(yoH, yoL, EXPO_ASCALE + pw2 + pw1);
	nbits = ABS_EXPO_BETA + pw2 + pw1 - IIR_SCALE;
	if(nbits >= 0) {
		int64RshiftRnd(yoH, yoL, nbits, &tmp64_hi, &tmp64_lo);
	} else {
		int64Lshift(yoH, yoL, -nbits, &tmp64_hi, &tmp64_lo);
	}
	int64Lshift(y_hi, y_lo, EXPO_ASCALE + EXPO_BETA, &tmp64_hi_2, &tmp64_lo_2);
	int64SubInt64(tmp64_hi_2, tmp64_lo_2, tmp64_hi, tmp64_lo, 
			&tmp64_hi_3, &tmp64_lo_3);
	int64RshiftRnd(tmp64_hi_3, tmp64_lo_3, IIR_SCALE, &tmp64_hi, &tmp64_lo);
	int64AddInt64(cbInfo[cb].prev_rate_hi, cbInfo[cb].prev_rate_lo, tmp64_hi, tmp64_lo,  
			&a_hi, &a_lo);

	// to compensate fraction dft for very frequent PCRs
	#define N_FRACT   16			

	nbits = EXPO_ASCALE + pw2 + pw1 - N_FRACT;			
	if(nbits >= 0 ) {
		int64RshiftRnd(yoH, yoL, nbits, &tmp64_hi, &tmp64_lo);
	} else {
		int64Lshift(yoH, yoL, -nbits, &tmp64_hi, &tmp64_lo);
	}
	dft0 = tmp64_lo; //tmp64_hi should be all sign bits
	
	dft0 = dft0 + cbInfo[cb].prev_deliv_fract;
	// dft0 = dft0 + prev_deliv_fract;

	dft = dft0 >> N_FRACT;
	cbInfo[cb].prev_deliv_fract = dft0 - (dft << N_FRACT);
	// prev_deliv_fract = dft0 - (dft << N_FRACT);


	// calculate the deliv time;
	pcrAddTick_in( &cbInfo[cb].prev_deliv, x+dft, pdeliv); 		 		
	// update state variables
	cbInfo[cb].prev_dl_hi = dl_hi;
	cbInfo[cb].prev_dl_lo = dl_lo;

	cbInfo[cb].prev_rate_hi = a_hi;
	cbInfo[cb].prev_rate_lo = a_lo;
	
	cbInfo[cb]. prev_fdl_hi = fdl_hi;
	cbInfo[cb]. prev_fdl_lo = fdl_lo;
	
	//==========================================================
	} else { // program wrap-around

	cbInfo[cb].prev_deliv_fract = 0;
	// prev_deliv_fract = 0;

    		CLOCK_ASSIGN(*pdeliv, *arrv);

	cbInfo[cb].prvPcr_idx = INVALID_PCR_INDEX;

	cbInfo[cb].prev_dl_hi = 0;
	cbInfo[cb].prev_dl_lo = 0;
	
	cbInfo[cb]. prev_fdl_hi = 0;
	cbInfo[cb]. prev_fdl_lo = 0;
	
	}

	{
	int diff;
	
	if( pcrGTpcr_in(pdeliv, arrv) ) {
                diff = timeDiff_in(pdeliv, arrv);
	} else {
                diff = timeDiff_in(arrv, pdeliv);
                diff = -diff;
	}
	
	// if(cb==0) LOG_printf(&trcPoll, "========>count_idx =%d currI = %d <=========== ", cbInfo[cb].count_idx, currIdx );
	LOG_printf(&trcPoll, "ch = %d  pdeliv - arrv = %d", cb, diff);
	LOG_printf(&trcPoll, "============>ch = %d  rate_lo = %d", cb, cbInfo[cb].prev_rate_lo);
	
        }
	}
}
