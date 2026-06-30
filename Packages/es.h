/* @(#)es.h	1.4  09/15/99  @(#) */
/*
 *  ======== es.h ========
 *
 *
 */
#ifndef _ES_H_
#define _ES_H_

#include "que.h"

#define MAX_ESHDR_LENGTH		1024

#define	MAX_FRAMES_WITHOUT_OVERFLOW	256
#define	MAX_AUDIO_FRAMES_WO_OVERFLOW	64

#define	BUF_FULLNESS_UPPER_BOUND	(1835086 >> 3)

typedef struct statistics {
    QueElem link;
    Uint32  picCnt;	/* picture counter			      */
    Uint32  dts;	/* current DTS for this picture		      */
    Uint32  pcr;	/* current PCR for this frame                 */
    Uint32  localTS;	/* current local time stamp for this frame    */
    Uint32  picSize;	/* picture size including everything	      */
    Uint32  rcPicSize;	/* rate convertable picture size, first slice */
    Uint32  repField;	/* repeated field indicator */
    Uint32  packets;	/* number of packets for the current frame    */
    Uint32  underFlow;	/* indicate whether this frame underflows or not */
} StatElem;

typedef struct audioStatistics {
    QueElem link;
    Uint32  counter;	/* audio frame counter			    */
    Uint32  dts;	/* current DTS for this picture		    */
    Uint32  size;	/* audio frame size			    */
    Uint32  packets;	/* number of packets for the current frame  */
} AudFrame;

typedef struct {
    Uint16  esState;
    Uint16  esState1;	    /* just tell whether it is in ES header state or not */
    Uint8   *esHdrPtr;
    Uint8   *esHdrBuf;
    StatElem *statQue;
} ESCtrl;

typedef struct {
    Uint32  fullPkts;
    Uint32  fullness;
} BufLevel;

extern QueElem	statFreeQueHead;
extern QueElem	statFullQueHead;

extern QueElem	audioFreeFrames;
extern QueElem	audioFullFrames;

extern void	esInit( void );
extern void	esProcess ( Uint8 *esSrc, Uint32 payloadSize );

#endif  /* _ES_H_ */
