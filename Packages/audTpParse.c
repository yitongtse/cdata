/* @(#)audTpParse.c	1.1  09/24/99  @(#) */
#include   <stdio.h>
#include   <stdlib.h>
#include   <string.h>

#include   "mydefs.h"
#include   "que.h"
#include   "pidbuf.h"

#include   "audio.h"


extern Uint32  audBuff;
extern Uint8   audInSync;


void audTpParse( Uint32 *tp, Uint32 tpIndx, Uint32 pcr45K )
{
  static QueElem   pidbufQueHead;
  static Uint32    startFlag = 0;
  static Uint32    pcr45KPrev;
  PidBuf          *pt_pidBuf;
  Uint32           tpPyldLoc, pesPyldLoc;

  if ( startFlag == 0 )
  {
    startFlag = 1;
    queInit( &pidbufQueHead );
    audInSync = FALSE;
    findTpPesLoc( (Uint8 *) NULL, 1, &tpPyldLoc, &pesPyldLoc );
  }

  processAudio( &pidbufQueHead, stdout, pcr45K, pcr45KPrev );

  pt_pidBuf = (PidBuf *) calloc( 1, sizeof( PidBuf ) );
  memByteCopy( (Uint8 *) &(pt_pidBuf->tpHdr), 188, (Uint8 *) &(tp[ 0 ]),
	       (Uint32) &(pt_pidBuf->tpHdr) & 0x3, (Uint32) &(tp[ 0 ]) & 0x3 );
  quePut( &pidbufQueHead, &(pt_pidBuf->link) );
  pt_pidBuf->rmHdr[0] = tpIndx;
  pt_pidBuf->pcr = pcr45K;

  findTpPesLoc( (Uint8 *) &(pt_pidBuf->tpHdr), 0, &tpPyldLoc, &pesPyldLoc );
  setTpPyldLoc( pt_pidBuf, tpPyldLoc );
  setPesPyldLoc( pt_pidBuf, pesPyldLoc );

  /* add the PES data size to the buffer */
  audBuff += (188 - getTpPyldLoc( pt_pidBuf ));
  if ( audBuff > 2848 )
  {
    fprintf( stdout, " pcr: 0x%08x   buf: %4i    ", pcr45K, audBuff );
    fprintf( stdout, "X ERROR: %d byte from tp %d", (188 - getTpPyldLoc( pt_pidBuf )), tpIndx );
    fprintf( stdout, ", cause buffer overflow (> 2848)\n" );
  }
  else
  {
    myPrintf( VERBOSE, " pcr: 0x%08x   buf: %4i    ", pcr45K, audBuff );
    myPrintf( VERBOSE, "  %d byte from tp %d", (188 - getTpPyldLoc( pt_pidBuf )), tpIndx );
    myPrintf( VERBOSE, "\n" );
  }

  pcr45KPrev = pcr45K;
}
