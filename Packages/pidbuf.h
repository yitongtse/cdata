/* @(#)pidbuf.h	1.2  09/27/99  @(#) */
#ifndef _PIDBUF_H_
#define _PIDBUF_H_

#include "utils.h"

/********** Definition **********/

#define TP_SZ                   188
#define PKT_HDR_SZ              12
#define PKT_TRL_SZ              56
#define XFER_SZ                 (PKT_HDR_SZ+TP_SZ)    /* xfer data size */
#define PKT_SZ                  (PKT_HDR_SZ+TP_SZ+PKT_TRL_SZ)   /* 256 */
#define TP_HDR_OFFSET           (-(PKT_SZ+4-PKT_HDR_SZ))
#define PIDBUF_LINK_OFFSET      (PKT_SZ - 8)

#define PIDBUF_PTR_OFFSET   (sizeof(PidBuf) - sizeof(QueElem))
#define PIDBUF_PTR_WOFFSET  (PIDBUF_PTR_OFFSET >> 2)



/********** Data Type **********/

typedef struct pidbuf
{
  Uint32  rmHdr[3];           /* ratemux packet header              */
  Uint32  tpHdr;              /* TP header                          */
  Uint32  tpPayload[46];      /* TP payload                         */
  Uint32  audioDTS;           /* audio DTS for the cur access unit  */
  Uint32  descriptor;         /* packet status description          */
  Uint32  offset;             /* start code locations               */
  Uint32  offset2;            /* more start code locations          */
  Uint32  pcr;                /* 32-bit PCR value                   */

  /* Demux specific variables */
  Uint16  quantSum;           /* sum of slice quant                 */
  Uint16  sliceCnt;           /* # of collected slice quant         */
  Uint16  payloadLength;      /* payloadLength                      */
  Int16   esHdrOffset;        /* offset for the first ES_Start_Code */
  Uint16  payloadSz;          /* payload size in bytes              */
  Uint16  esOffset;           /* ES offset from TP header           */

  Uint16  usedFlag;           /* 0: free, 1: used                   */
  Int16   chanId;             /* channel ID, or 100 for message     */
  Uint16  picNo;              /* picture or message no.             */
  Uint16  picIdx;             /* TP idx within picture              */
  Uint32  inIdx;              /* input TP idx                       */
    
  Uint32  fpPose;             /* reserved                           */

  QueElem link;
}
PidBuf;



/* Get PidBuf pointer from link pointer */
__INLINE PidBuf* getPidBuf(QueElem *linkPtr)
{
    return (PidBuf*)(((Uint32*)linkPtr) - PIDBUF_LINK_OFFSET/4);
}

/* Get pointer to next PidBuf */
__INLINE PidBuf* getNextPidBuf(PidBuf *pidBufPtr)
{
    return getPidBuf(pidBufPtr->link.next);
}
 
/* Get pointer to previous PidBuf */
__INLINE PidBuf* getPrevPidBuf(PidBuf *pidBufPtr)
{
    return getPidBuf(pidBufPtr->link.prev);
}



/********** Definition **********/

#define FIELD_TpPesPyld    offset
#define FIELD_AfExpSize    offset2



/********** Global Function **********/

extern void findTpPesLoc( Uint8 *pt8_tp, Uint32 resetFlag,
			  Uint32 *pt_tpPyldLoc, Uint32 *pt_pesPyldLoc );



/********** Inline Function **********/

__INLINE Uint32 getTpPyldLoc( PidBuf *pt_pidBuf )
{
  return( extU( pt_pidBuf->FIELD_TpPesPyld, 0, 16 ) );
}

__INLINE Uint32 getPesPyldLoc( PidBuf *pt_pidBuf )
{
  return( extU( pt_pidBuf->FIELD_TpPesPyld, 16, 16 ) );
}

__INLINE void setTpPyldLoc( PidBuf *pt_pidBuf, Uint32 tpPyldLoc )
{
  pt_pidBuf->FIELD_TpPesPyld = (tpPyldLoc << 16) +
                               extU( pt_pidBuf->FIELD_TpPesPyld, 16, 16 );
}

__INLINE void setPesPyldLoc( PidBuf *pt_pidBuf, Uint32 pesPyldLoc )
{
 pt_pidBuf->FIELD_TpPesPyld = (extU( pt_pidBuf->FIELD_TpPesPyld, 0, 16 ) << 16) + 
                              extU( pesPyldLoc, 16, 16 );
}

__INLINE void copyAfExpSize( PidBuf *pt_pidBufFrom, PidBuf *pt_pidBufTo )
{
  pt_pidBufTo->FIELD_AfExpSize = pt_pidBufFrom->FIELD_AfExpSize;
}

__INLINE Uint32 getAfExpSize( PidBuf *pt_pidBuf )
{
  return( pt_pidBuf->FIELD_AfExpSize );
}

__INLINE void setAfExpSize( PidBuf *pt_pidBuf, Uint32 afExpSize )
{
  pt_pidBuf->FIELD_AfExpSize = afExpSize;
}


#endif /* _PIDBUF_H_ */
