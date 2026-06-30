/* @(#)audPidBufSync.c	1.1  09/24/99  @(#) */
#include <stdio.h>
#include <stdlib.h>

#include "mydefs.h"
#include "que.h"
#include "pidbuf.h"

#include "audio.h"



static QueElem   *pt_queHead;

static Uint8      syncStrmType;
static Uint8      syncFramStrt;
static Uint16     syncFramSize;
static Uint16     syncFramTime;
static Uint32     syncPtsNow;
static Uint32     syncPesPtsPcr;
static Uint32     syncFramSizeByte;

static Uint32     pesByteDrop;


static Uint32 srchPesPidBuf( PidBuf **pt_pesPidBuf )
{
  PidBuf   *pt_pidBuf;

  while ( pt_queHead->next != pt_queHead )
  {
    /* get the PidBuf */
    pt_pidBuf = getPidBuf( pt_queHead->next );
    /* if flag "payload_unit_start_indicator" is set */
    if ( pt_pidBuf->tpHdr & BIT22 )
    {
      *pt_pesPidBuf = pt_pidBuf;
      return( SUCCESS );
    }

    /* move the PidBuf back to the free pool */
    pesByteDrop += (188 - getTpPyldLoc( pt_pidBuf ) );
    free( (void *) getPidBuf( queGet( pt_queHead ) ) );
  }

  return( NEED_MORE_DATA );
}



static Uint32 getAudSyncTailLoc( Uint8 *pt8_tp, Uint32 chekStrt, Uint32 *pt_mtch16,
				 Uint32 strtCode )
{
  Uint32   mtch16, pose;
  Uint32   bias, word;

  mtch16 = *pt_mtch16;
  if ( chekStrt < 188 )
    word = *((Uint32 *) (pt8_tp + (chekStrt & 0xfffffffc)));
  else
    return( chekStrt );

  for ( pose = chekStrt; pose < 188; ++pose )
  {
    /* the address bias to word boundary */
    bias = pose & 0x3;
    /* if 'pose' is on word boundary, update the word for checking */
    if ( bias == 0 )
      word = *((Uint32 *) (pt8_tp + pose));

    mtch16 = (mtch16 << 8) + extU( word, (bias << 3), 24 );
    /* if the audio SYNC code is found */
    if ( extU( mtch16, 16, 32 - 15 ) == strtCode )
      break;
  }

  *pt_mtch16 = mtch16;
  return( pose );
}



static Uint32 syncVerifier( PidBuf *pt_pidBuf, Uint32 syncLoc )
{
  Uint32     syncByte, n;
  Uint32     strtHead, strtTail;
  Uint32     framSizeByte, framSize;
  Uint32     rslt;


  strtHead =  (syncStrmType == MPEG_AUDIO) ? 0xff : 0x0b;
  strtTail = ((syncStrmType == MPEG_AUDIO) ? 0xfc : 0x77) >> 1;

  for ( n = 0; n < 1; ++n )
  {
    while ( syncLoc >= 188 )
    {
      /* have to get the next PidBuf, but no more PidBuf on the que */
      if ( pt_pidBuf->link.next == pt_queHead )
	return( NEED_MORE_DATA );

      /* get the next PidBuf */
      pt_pidBuf = getNextPidBuf( pt_pidBuf );
      syncLoc = getPesPyldLoc( pt_pidBuf ) + (syncLoc - 188);
    }
    syncByte = extU( *(&(pt_pidBuf->tpHdr) + (syncLoc >> 2)),
		     (syncLoc & 0x3) << 3, 32 - 8 );
    if ( syncByte != strtHead )
      return( FAIL );

    /* get the next audio frame sync tail location */
    syncLoc++;
    while ( syncLoc >= 188 )
    {
      /* have to get the next PidBuf, but no more PidBuf on the que */
      if ( pt_pidBuf->link.next == pt_queHead )
	return( NEED_MORE_DATA );

      /* get the next PidBuf */
      pt_pidBuf = getNextPidBuf( pt_pidBuf );
      syncLoc = getPesPyldLoc( pt_pidBuf ) + (syncLoc - 188);
    }
    syncByte = extU( *(&(pt_pidBuf->tpHdr) + (syncLoc >> 2)),
		     (syncLoc & 0x3) << 3, 32 - 7 );
    if ( syncByte != strtTail )
      return( FAIL );

    /* now, we have to locate the frame size byte */
    rslt = srchFrmeSizeByte( pt_queHead, pt_pidBuf,
			     (syncLoc - 1 + ((syncStrmType == MPEG_AUDIO) ? 2 : 4)),
			     &framSizeByte );
    if ( rslt == NEED_MORE_DATA )
      return( NEED_MORE_DATA );

    /* calculate the audio frame size in byte */
    if ( syncStrmType == MPEG_AUDIO )
      framSize = calcFramSizeMPEG( framSizeByte );
    else
      framSize = calcFramSizeAC3( framSizeByte );
    if ( framSize == 0 )
      return( FAIL );

    /* get the next audio frame sync head location */
    syncLoc = (syncLoc - 1) + framSize;
  }

  return( SUCCESS );
}



static Uint32 srchAudSync( PidBuf *pt_pidBufSrch, PidBuf **pt_pidBuf )
{
  PidBuf    *pt_pidBufSync;
  Uint32     syncLoc, syncLocTail, srchStrt;
  Uint32     framSizeByte;
  Uint32     mtch16, rslt, strtCode;


  /* locate the PidBuf and the byte location of audio sync's tail-byte */
  mtch16 = 0;
  srchStrt = getPesPyldLoc( pt_pidBufSrch );
  strtCode = ((syncStrmType == MPEG_AUDIO) ? 0xfffc : 0x0b77) >> 1;

  while ( 1 )
  {
    while ( 1 )
    {
      /* scan the TP's PES paylaod for audio sync */
      syncLocTail = getAudSyncTailLoc( (Uint8 *) &(pt_pidBufSrch->tpHdr), srchStrt,
				       &mtch16, strtCode );
      /* if we found the audio sync pattern */
      if ( syncLocTail < 188 )
	break;

      /* have to get the next PidBuf, but no more PidBuf on the que */
      if ( pt_pidBufSrch->link.next == pt_queHead )
	/* signal that need more data */
	return( NEED_MORE_DATA );
      /* get the next PidBuf */
      pt_pidBufSrch = getNextPidBuf( pt_pidBufSrch );
      /* if a new PES header starts in this TP */
      if ( pt_pidBufSrch->tpHdr & BIT22 )
	/* fail to locate the audio sync before next PES header */
	return( FAIL );

      srchStrt = getPesPyldLoc( pt_pidBufSrch );
    }
    srchStrt = syncLocTail + 1;

    /* locate the PidBuf and the byte location of audio sync's head-byte */
    pt_pidBufSync = pt_pidBufSrch;
    syncLoc = syncLocTail - 1;
    while ( syncLoc < getPesPyldLoc( pt_pidBufSync ) )
    {
      syncLoc = 188 - (getPesPyldLoc( pt_pidBufSync ) - syncLoc);
      pt_pidBufSync = getPrevPidBuf( pt_pidBufSync );
    }

    /* now, we have to locate the frame size byte */
    rslt = srchFrmeSizeByte( pt_queHead, pt_pidBufSync,
			     (syncLoc + ((syncStrmType == MPEG_AUDIO) ? 2 : 4)),
			     &framSizeByte );
    if ( rslt == NEED_MORE_DATA )
      return( NEED_MORE_DATA );

    syncFramSizeByte = framSizeByte;
    /* calculate the audio frame size in byte */
    if ( syncStrmType == MPEG_AUDIO )
    {
      syncFramSize = calcFramSizeMPEG( framSizeByte );
      syncFramTime = framTimeTablMPEG[ extU( framSizeByte, 28, 30 ) ];
    }
    else
    {
      syncFramSize = calcFramSizeAC3( framSizeByte );
      syncFramTime = framTimeTablAC3[ extU( framSizeByte, 24, 30 ) ];
    }
    /* if the audio frame size is zero */
    if ( syncFramSize == 0 )
      continue;

    /* verify the 2 byte sync word we found */
    rslt = syncVerifier( pt_pidBufSync, (syncLoc + syncFramSize) );
    if ( rslt == NEED_MORE_DATA )
      return( NEED_MORE_DATA );
    else if ( rslt == SUCCESS )
      break;
  }

  syncFramStrt = syncLoc;
  *pt_pidBuf = pt_pidBufSync;  
  return( SUCCESS );
}



Uint32 syncAudPidBuf( QueElem *pt_queHeadIn,
		      Uint32 *pt_tpStTm, Uint32 *pt_ptsNow, Uint32 *pt_pesByteDrop,
		      Uint32 *pt_pesPtsPcr, Uint32 *pt_audFramSizeByte )
{
  PidBuf   *pt_pidBufHead, *pt_pidBuf;
  Uint32    rslt;


  pesByteDrop = 0;
  pt_queHead = pt_queHeadIn;

  while ( 1 )
  {
    /* try to search the PES header */
    rslt = srchPesPidBuf( &pt_pidBufHead );
    /* if none of the audio PidBuf carries the first PES header byte */
    if ( rslt == NEED_MORE_DATA )
    {
      *pt_pesByteDrop = pesByteDrop;
      return( FAIL );
    }

    /* has Locked the PES header, start to grab the PTS and the stream ID */
    rslt = ptsFetch( pt_queHead, pt_pidBufHead, &syncPtsNow );
    /* if NO PTS in this PES header */
    if ( rslt == FAIL )
    {
      removePidBuf( pt_pidBufHead );
      continue;
    }
    /* if we need more PidBuf to get the PTS, but no more PidBuf */
    if ( rslt == NEED_MORE_DATA )
    {
      *pt_pesByteDrop = pesByteDrop;
      return( FAIL );
    }
    syncPesPtsPcr = pt_pidBufHead->pcr;

    /* if success, the stream type is embedded in rslt */
    if ( (rslt >> 16) == 0xbd )
      syncStrmType = AC3_AUDIO;
    else if ( 0xc0 <= (rslt >> 16) && (rslt >> 16) <= 0xdf )
      syncStrmType = MPEG_AUDIO;
    else
    {
      removePidBuf( pt_pidBufHead );
      continue;
    }

    /* now try to lock the audio frame, search/verify the first audio sync before
       the next PES header */
    rslt = srchAudSync( pt_pidBufHead, &pt_pidBuf );
    /* if NO audio sync before next PES header */
    if ( rslt == FAIL )
    {
      removePidBuf( pt_pidBufHead );
      continue;
    }
    /* if we need more PidBuf to find/verify the audio sync, but no more PidBuf */
    if ( rslt == NEED_MORE_DATA )
    {
      *pt_pesByteDrop = pesByteDrop;
      return( FAIL );
    }

    /* Now the PTS and the corresponding audio frame are found and confirmed */
    break;
  }

  /* remove the PidBufs before the PidBuf carries audio sync */
  while ( pt_queHead->next != &(pt_pidBuf->link) )
  {
    /* move the PidBuf back to the free pool */
    pesByteDrop += (188 - getTpPyldLoc( getPidBuf( pt_queHead->next) ) );
    free( (void *) getPidBuf( queGet( pt_queHead ) ) );
  }
  pesByteDrop += (syncFramStrt - getTpPyldLoc( pt_pidBuf ));

  *pt_tpStTm = ( syncStrmType << 24) | (syncFramStrt << 16) | syncFramTime;
  *pt_ptsNow = syncPtsNow;
  *pt_pesByteDrop = pesByteDrop;
  *pt_pesPtsPcr = syncPesPtsPcr;
  *pt_audFramSizeByte = syncFramSizeByte;
  return( SUCCESS );
}
