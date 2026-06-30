/* @(#)audTpProc.c	1.2  09/27/99  @(#) */
#include <stdio.h>
#include <stdlib.h>

#include "mydefs.h"
#include "que.h"
#include "pidbuf.h"

#include "audio.h"



Uint8           audInSync;       /* TRUE or FALSE */
Int32           audBuff;

static Uint8    audStrmType;     /* 0 -> MPEG audio stream, 1 -> AC3 audio stream */
static Uint8    audDtsRead;      /* DTS_NOT_READ  -> hasn't read present TP's DTS,
				    DTS_BEEN_READ -> had read present TP's DTS */
static Uint8    audDtsCary;      /* because 'audFramTime' is in 90 kHz and DTS is in 45 KHz */ 
static Uint32   audDtsNext;      /* next comming audio frame's DTS (from PES or calculation) */
static Uint32   audDtsNow;       /* DTS of present audio frame */
static Uint16   audFramSize;     /* size of present audio frame */
static Uint16   audFramTime;     /* time period of audio frame (in 90 kHz ticks) */
static Uint8    audFramStrt;     /* location of the first byte of present audio frame in TP,
				    (it indicates the audio frame boundary in TP) */
static Uint16   audFramScan;     /* number of scanned/processed byte of present audio frame */
static Uint32   audPesDtsPcr;
static Uint32   audPesDtsFlag;

static Uint32   pesDtsFlag;
static Uint32   pesDtsPcr;

static Uint32   pesNewDtsFlag;
static Uint32   pesNewDtsPcr;

static Uint32   audErorFlag;


static void dump( Uint32 framSizeByte );



static void estmNextDts( Uint32 framTime )
{
  audDtsCary = (audDtsCary & 0x1) + (framTime & 0x1);
  audDtsNext += ((framTime >> 1) + (audDtsCary >> 1));
}


static Uint32 pesDtsProcess( QueElem *pt_srcQue, PidBuf *pt_pidBuf )
{
  Uint32   rslt, dts;

  rslt = ptsFetch( pt_srcQue, pt_pidBuf, &dts );
  if ( (rslt & 0xff) == SUCCESS )
  {
    audDtsNext = dts;

    pesDtsFlag = 1;
    pesDtsPcr  = pt_pidBuf->pcr;

    if ( audErorFlag && audFramScan == 0 )
      audErorFlag = 0;

    /* remove the PES header byte */
    audBuff -= (getPesPyldLoc( pt_pidBuf ) - getTpPyldLoc( pt_pidBuf ) );
  }
  return( rslt & 0xff );
}


void processAudio( QueElem *pt_srcQue, FILE *fp_out, Uint32 pcrNow, Uint32 pcrPrev )
{
  PidBuf   *pt_pidBuf;
  Uint32    syncByte, framSizeByte;


RE_SYNC_REQUEST:
  /* if subroutine lost the audio sync */
  if ( audInSync == FALSE )
  {
    Uint32   tpStTm;  /* packed data, contain (1) type, (2) start, and (3) time */
    Uint32   pesByteDrop;
    Uint32   tmpFramSizeByte;

    if ( syncAudPidBuf( pt_srcQue, &tpStTm, &audDtsNext, &pesByteDrop,
			&pesDtsPcr, &tmpFramSizeByte ) == FAIL )
    {
      audBuff = audBuff - pesByteDrop;
      return;
    }
    audBuff = audBuff - pesByteDrop;

    /* set the audio fram/stream basic data */
    audInSync = TRUE;

    audStrmType = extU( tpStTm,  0, 24 );
    audFramStrt = extU( tpStTm,  8, 24 );
    audFramTime = extU( tpStTm, 16, 16 );
    audFramScan = 0;

    pesDtsFlag = 1;
    audDtsRead = PTS_BEEN_READ;

    audErorFlag = 0;

    myPrintf( VERBOSE, " pcr: 0x%08x+  buf: %4i    s drop %d byte, Re-Sync %s audio ",
	      pcrPrev, audBuff, pesByteDrop, (audStrmType == MPEG_AUDIO) ? "MPEG" : "AC3" );
    dump( tmpFramSizeByte );
  }

  /* main loop */
  while ( 1 )
  {
    /* if audio que is empty */
    if ( pt_srcQue->next == pt_srcQue )
    {
      if ( audFramScan != 0 || pcrNow >= audDtsNext )
      {
	if ( !audErorFlag )
	{
	  fprintf( fp_out, " pcr: 0x%08x   buf: %4i    X ERROR: underflow (%i byte)",
		   audDtsNow, audBuff, (audFramScan - audFramSize) );
	  if ( audPesDtsFlag )
	    fprintf( fp_out, ", DTS_arrival_time (DAT) is 0x%08x, DTS-DAT = %i",
		     audPesDtsPcr, (Int32) (audDtsNow - audPesDtsPcr) );
	  fprintf( fp_out, "\n" );
	}
	audErorFlag = 1;
      }
      break;
    }
    /* get PidBuf from source audio que head */
    pt_pidBuf = getPidBuf( pt_srcQue->next );

    /* set the first byte location of the audio frame in this new TP */
    if ( audFramStrt == (Uint8) UNKNOWN )
      audFramStrt = getPesPyldLoc( pt_pidBuf );

    /* if TP carries first byte of PES header, and haven't been touched yet */
    if ( (pt_pidBuf->tpHdr & BIT22) && audDtsRead == PTS_NOT_READ )
    {
      /* fetch the DTS from PES header */
      if ( pesDtsProcess( pt_srcQue, pt_pidBuf ) == NEED_MORE_DATA )
      {
	if ( !audErorFlag )
	{
	  fprintf( fp_out, " pcr: 0x%08x   buf: %4i    X ERROR: underflow (%i byte)",
		   audDtsNow, audBuff, (audFramScan - audFramSize) );
	  if ( audPesDtsFlag )
	    fprintf( fp_out, ", DTS_arrival_time (DAT) is 0x%08x, DTS-DAT = %i",
		     audPesDtsPcr, (Int32) (audDtsNow - audPesDtsPcr) );
	  fprintf( fp_out, "\n" );
	}
	audErorFlag = 1;
	break;
      }
      /* set this PidBuf as touched */ 
      audDtsRead = PTS_BEEN_READ;
    }

  SCAN_AUD_FRAME:
    /* if the audio frame hasn't been scanned before */
    if ( audFramScan == 0 && audFramStrt < 188 )
    {
      /* if the current PCR is bigger then the candidate frame's DTS */
      if ( (Int32) (audDtsNext - pcrNow) > 0 )
	break;
      /* update the DTS of present audio frame */
      audDtsNow = audDtsNext;
      

      /* check the first byte of SYNC in new audio frame */
      syncByte = extU( *(&(pt_pidBuf->tpHdr) + (audFramStrt >> 2)),
		       (audFramStrt & 0x3) << 3, 24 );

      /* now, we have to locate the frame size byte */
      if ( srchFrmeSizeByte( pt_srcQue, pt_pidBuf,
			     (audFramStrt + ((audStrmType == MPEG_AUDIO) ? 2 : 4)),
			     &framSizeByte ) == NEED_MORE_DATA )
      {
	if ( !audErorFlag )
	{
	  fprintf( fp_out, " pcr: 0x%08x   buf: %4i    X ERROR: underflow (%i byte)",
		   audDtsNow, audBuff, (audFramScan - audFramSize) );
	  if ( audPesDtsFlag )
	    fprintf( fp_out, ", DTS_arrival_time (DAT) is 0x%08x, DTS-DAT = %i",
		     audPesDtsPcr, (Int32) (audDtsNow - audPesDtsPcr) );
	  fprintf( fp_out, "\n" );
	}
	audErorFlag = 1;
	break;
      }
      /* calculate the audio frame size in byte */
      if ( audStrmType == MPEG_AUDIO )
      {
	audFramSize = calcFramSizeMPEG( framSizeByte );
	audFramTime = framTimeTablMPEG[ extU( framSizeByte, 28, 30 ) ];
      }
      else
      {
	audFramSize = calcFramSizeAC3( framSizeByte );
	audFramTime = framTimeTablAC3[ extU( framSizeByte, 24, 30 ) ];
      }

      /* if something is wrong, ask for re-sync */
      if ( syncByte != ((audStrmType == MPEG_AUDIO) ? 0xff : 0x0b) ||
	   audFramSize == 0 )
      {
	audInSync = FALSE;
	fprintf( fp_out, " pcr: 0x%08x+  buf: %4i    X ERROR: wrong sync header, re-sync!!",
		   audDtsNow, audBuff );
	goto RE_SYNC_REQUEST;
      }

      /* estimate the next audio frame DTS */
      estmNextDts( audFramTime );

      audPesDtsFlag = (pesDtsFlag ? 1 : 0);
      audPesDtsPcr  =  pesDtsPcr;
      pesDtsFlag = 0;
    }

    audFramScan += (188 - audFramStrt);
    if ( audFramScan >= audFramSize )
    {
      /* update the buffer fullness */
      audBuff -= ((188 - (audFramScan - audFramSize)) - audFramStrt);
      if ( audErorFlag )
      {
	myPrintf( VERBOSE, " pcr: 0x%08x+  buf: %4i    * Decode accomplished\n",
		  pcrPrev, audBuff, audFramSize );
      }
      else
      {
	myPrintf( (VERBOSE | SHOW_AUDIO_BUF), " pcr: 0x%08x   buf: %4i    * Decode %d byte",
		  audDtsNow, audBuff, audFramSize );
	if ( audPesDtsFlag )
	    myPrintf( (VERBOSE | SHOW_AUDIO_BUF), ", DTS_arrival_time (DAT) is 0x%08x, DTS-DAT = %i",
		      audPesDtsPcr, (Int32) (audDtsNow - audPesDtsPcr) );
	myPrintf( (VERBOSE | SHOW_AUDIO_BUF), "\n" );
      }

      audErorFlag = 0;

      /* set the first byte location of the new audio frame */
      audFramStrt = 188 - (audFramScan - audFramSize);
      if ( audFramScan > audFramSize )
      {
	/* reset the scan counter */
	audFramScan = 0;
	goto SCAN_AUD_FRAME;
      }
      else
      {
	/* reset the scan counter */
	audFramScan = 0;
	audDtsNow = audDtsNext;
      }
    }
    else
    {
      /* update the buffer fullness */
      audBuff -= (188 - audFramStrt);
    }

    free( getPidBuf( queGet( pt_srcQue )) );
    audDtsRead  = PTS_NOT_READ;
    audFramStrt = (Uint8) UNKNOWN;

    if ( (Int32) (audDtsNext - pcrNow) >= 0 && audFramScan == 0 )
      break;
  }

  return;
}



static void dump( Uint32 framSizeByte )
{
  static Uint32 MPGrateIndxTabl[ 16 ] = {   0,  32,  48,  56,  64,  80,  96, 112,
					  128, 160, 192, 224, 256, 320, 384,   0 };
  static Uint32 MPGsmplFreqTabl[ 4 ]  = { 44100, 48000, 32000, 0 };
  static Uint32 MPGframTimeTabl[ 4 ]  = {  2351,  2160,  3240, 0 };

  static Uint32 AC3rateIndxTabl[ 19 ] = {  32,  40,  48,  56,  64,  80,  96, 112,
					  128, 160, 192, 224, 256, 320, 384, 448,
                                          512, 576, 640};
  static Uint32 AC3smplFreqTabl[ 4 ]  = { 48000, 44100, 32000, 0 };
  static Uint32 AC3framTimeTabl[ 4 ]  = {  2880,  3135,  4320, 0 };
  Uint32        smplFreqCode, rateIndxCode;

  if ( audStrmType == MPEG_AUDIO )
  {
    smplFreqCode = extU( framSizeByte, 28, 30 );
    rateIndxCode = extU( framSizeByte, 24, 28 );
    myPrintf( VERBOSE, "(%5i Hz, %3i kbps, %i 90kHz ticks)\n",
	      MPGsmplFreqTabl[ smplFreqCode ], MPGrateIndxTabl[ rateIndxCode ],
	      MPGframTimeTabl[ smplFreqCode ] );
  }
  else
  {
    smplFreqCode = extU( framSizeByte, 24, 30 );
    rateIndxCode = extU( framSizeByte, 26, 26 );
    myPrintf( VERBOSE, "(%5i Hz, %3i kbps, %i 90kHz ticks)\n",
	      AC3smplFreqTabl[ smplFreqCode ], AC3rateIndxTabl[ rateIndxCode >> 1 ],
	      AC3framTimeTabl[ smplFreqCode ] );
  }
}
