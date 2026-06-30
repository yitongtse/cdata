/* @(#)audTools.c	1.1  09/24/99  @(#) */
#include <stdio.h>
#include <stdlib.h>

#include "mydefs.h"
#include "que.h"
#include "pidbuf.h"

#include "audio.h"



Uint32     framTimeTablMPEG[ 4 ] = { 2351, 2160, 3240, 0 };
Uint32     framTimeTablAC3[ 4 ]  = { 2880, 3135, 4320, 0 };



Uint32 calcFramSizeAC3( Uint32 framSizeByte )
{
  static Uint16 framSizeTablAC3[ 19 ][ 3 ] =
  {
    {    64,   69,   96}, 
    {    80,   87,  120}, 
    {    96,  104,  144}, 
    {   112,  121,  168}, 
    {   128,  139,  192}, 
    {   160,  174,  240}, 
    {   192,  208,  288}, 
    {   224,  243,  336}, 
    {   256,  278,  384}, 
    {   320,  348,  480}, 
    {   384,  417,  576}, 
    {   448,  487,  672}, 
    {   512,  557,  768}, 
    {   640,  696,  960}, 
    {   768,  835, 1152}, 
    {   896,  975, 1344}, 
    {  1024, 1114, 1536}, 
    {  1152, 1253, 1728}, 
    {  1280, 1393, 1920}
  };
  Uint32   smplRateCode, framSizeCode, framSize;

  smplRateCode = extU( framSizeByte, 24, 30 );
  framSizeCode = extU( framSizeByte, 26, 26 );
  if ( smplRateCode > 2 || framSizeCode > 37 )
    return( 0 );

  framSize = framSizeTablAC3[ framSizeCode >> 1 ][ smplRateCode ];
  if ( smplRateCode == 1 && (framSizeCode & 0x1) )
    framSize += 1;

  return( (framSize << 1) );
}



Uint32 calcFramSizeMPEG( Uint32 framSizeByte )
{
  static Uint16 framSizeTablMPEG[ 15 ][ 3 ] =
  {
    {   0,    0,    0}, 
    { 104,   96,  144}, 
    { 156,  144,  216}, 
    { 182,  168,  252},
    { 208,  192,  288}, 
    { 261,  240,  360}, 
    { 313,  288,  432}, 
    { 365,  336,  504}, 
    { 417,  384,  576}, 
    { 522,  480,  720}, 
    { 626,  576,  864}, 
    { 731,  672, 1008}, 
    { 835,  768, 1152}, 
    {1044,  960, 1440}, 
    {1253, 1152, 1728}
  };
  Uint32   smplFreqCode, rateIndxCode, framSize;

  smplFreqCode = extU( framSizeByte, 28, 30 );
  rateIndxCode = extU( framSizeByte, 24, 28 );
  if ( smplFreqCode > 2 || rateIndxCode > 14 )
    return( 0 );

  framSize = framSizeTablMPEG[ rateIndxCode ][ smplFreqCode ];
  /* the padding bit which will add one more byte for 44.1 kHz */
  if ( smplFreqCode == 0 && extU( framSizeByte, 30, 31 ) )
    framSize += 1;

  return( framSize );
}



Uint32 srchFrmeSizeByte( QueElem *pt_queHead, PidBuf *pt_pidBuf, Uint32 framByteLoc,
			 Uint32 *pt_framSizeByte )
{
  while ( framByteLoc >= 188 )
  {
    /* have to get the next PidBuf, but no more PidBuf on the que */
    if ( pt_pidBuf->link.next == pt_queHead )
      return( NEED_MORE_DATA );

    /* get the next PidBuf */
    pt_pidBuf = getNextPidBuf( pt_pidBuf );
    framByteLoc = getPesPyldLoc( pt_pidBuf ) + (framByteLoc - 188);
  }

  /* now, the audio frame size is available */
  *pt_framSizeByte = extU( *(&(pt_pidBuf->tpHdr) + (framByteLoc >> 2)),
			   (framByteLoc & 0x3) << 3, 24 );
  return( SUCCESS );
}




/* Name:
 *   ptsFetch()
 *
 * Functions:
 *   - subroutine will try to fetch and return the PTS in PES header.
 *   - it will only return the 32 MSB bits in the PTS.  It will also return the
 *     search result as SUCCESS, FAIL, or NEED_MORE_DATA.
 *   - subroutine is memoryless, it will check the input PidBuf and the followed
 *     PidBufs if required.  It will stop when (1) a PTS is acquired (SUCCESS), (2)
 *     No PTS in the PES (FAIL), or (3) it reach the end of PidBuf que (NEED_MORE_
 *     DATA).
 *
 * Inputs:
 *   - pt_queHead               the que head of the processed PidBuf.
 *   - pt_pidBuf                points to the target PidBuf.
 *
 * Outputs and Return:
 *   - return                   the 32 MSB bits of the found PTS.
 *   - pt_rslt                  the fetch result (SUCCESS, FAIL, or NEED_MORE_DATA).
 *
 * Global Variable Used:
 *
 * Global Variable Modified:
 *
 * Inline Function Used:
 *   - getTpPyldLoc             get the TP payload location.
 *   - getPesPyldLoc            get the PES payload location.
 *   - getNextPidBuf            get next PidBuf in the que.
 *
 * Subroutines Used:
 *   - memByteCopy              byte by byte copy subroutine.
 */
Uint32 ptsFetch( QueElem *pt_queHead, PidBuf *pt_pidBuf, Uint32 *pt_pts )
{
  Uint8   *pt8_tp;
  Uint32   ptsPesHedrBuf[ 4 ];
  Uint32   tpPyldLoc, pesPyldLoc;
  Uint32   byteCopy, hedrSize, copySize;
  Uint32   pts;


  byteCopy = 0;

  while ( 1 )
  {
    tpPyldLoc  = getTpPyldLoc( pt_pidBuf );
    pesPyldLoc = getPesPyldLoc( pt_pidBuf );
    hedrSize = pesPyldLoc - tpPyldLoc;

    /* try to copy the first 14 bytes in PES header */
    if ( hedrSize > 0 && byteCopy < 14 )
    {
      copySize = 14 - byteCopy;
      copySize = (copySize > hedrSize) ? hedrSize : copySize;

      pt8_tp = (Uint8 *) &(pt_pidBuf->tpHdr);
      memByteCopy( (Uint8 *) ptsPesHedrBuf + byteCopy, copySize,
		   pt8_tp + tpPyldLoc,
		   (Uint32) ptsPesHedrBuf & 0x3, (Uint32) pt8_tp & 0x3 );
      byteCopy += copySize;
    }

    if ( byteCopy > 7 )
    {
      /* if there is "NO" PTS in this PES header */
      if ( extU( *(ptsPesHedrBuf + 1), 24, 31 ) == 0 )
	return( FAIL );

      /* PTS exists! if the first 14 bytes of PES header have been collected  */
      if ( byteCopy >= 14 )
      {
	pts  = extU( ptsPesHedrBuf[ 2 ], 12, 32 -  3) << (32 - 3);
	pts += extU( ptsPesHedrBuf[ 2 ], 16, 32 - 15) << (32 - 3 - 15);
	pts += extU( ptsPesHedrBuf[ 3 ],  0, 32 - 14);

	*pt_pts = pts;
	return( (SUCCESS | (extU( ptsPesHedrBuf[ 0 ], 24, 24 ) << 16)) );
      }
    }

    /* if the PES header ends in this TP (something wrong with the bitstream) */
    if ( pesPyldLoc < 188 )
      return( FAIL );

    /* if no more PidBuf in the que */
    if ( pt_pidBuf->link.next == pt_queHead )
      return( NEED_MORE_DATA );
    /* get the next PidBuf */
    pt_pidBuf = getNextPidBuf( pt_pidBuf );
  }
}



