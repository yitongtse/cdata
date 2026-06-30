/* @(#)audio.h	1.1  09/24/99  @(#) */
#include  <stdio.h>


/********** Definition **********/

/* value defined and used for processing result */
#define    FAIL               0
#define    NEED_MORE_DATA     1
#define    SUCCESS            2

/* value defined for PtsReadStat */
#define    PTS_NOT_READ       0
#define    PTS_BEEN_READ      1

/* value defined for audio stream type */
#define    MPEG_AUDIO         0
#define    AC3_AUDIO          1


#define    UNKNOWN            255

#define    SHOW_AUDIO_BUF     8
#define    VERBOSE            0x8000

/********** Data Type **********/

typedef struct
{
  Uint8    queSyncStat;
  /* -------------------------------------------- */
  /* the two variable shall be merged to cleanCut */
  Uint8    pesHedrSize;
  Uint8    pesHedrScan;
  /* -------------------------------------------- */
  Uint8    ptsStat;            /* used for storeing the audio type, .... */
  Uint32   ptsNext;            /* PTS (from PES header or estimate) of next frame */
  Uint32   pts;                /* PTS of being processed audio frame */
  Uint16   framSize;           /* size of one audio frame */
  Uint16   framTime;           /* time period of one audio frame (in 90 kHz ticks) */
  Uint16   framStrt;           /* location of the first byte of being processed
				  audio frame in the present (or incomming) TP */
  Uint16   framScan;           /* number of processed byte of being processed
				  audio frame */
  /* the following is for new audio splicing */
  Uint32   newPid;
  QueElem  outLink;            /* for temporary PidBuf hanging */
  Uint16   outCunter;          /* for continue counter correction */
  Uint16   pesPcktSize;        /* should be big enough */
  Uint32   splcStat;
} AudioStat;



/********** Global Variable **********/

extern Uint32          framTimeTablMPEG[ 4 ];
extern Uint32          framTimeTablAC3[ 4 ];



/********** Subroutine Declaration **********/

                /* audPidBufSync.c */
extern Uint32   syncAudPidBuf( QueElem *, Uint32 *, Uint32 *, Uint32 *, Uint32 *, Uint32 * );
                /* audTpProc.c */
extern void     processAudio( QueElem *, FILE *, Uint32, Uint32 );
                /* audFramSize.c */
extern Uint32   calcFramSizeAC3( Uint32 );
extern Uint32   calcFramSizeMPEG( Uint32 );
extern Uint32   srchFrmeSizeByte( QueElem *, PidBuf *, Uint32, Uint32 * );
                /* audTools.c */
extern Uint32   ptsFetch( QueElem *pt_queHead, PidBuf *pt_pidBuf, Uint32 *pt_pts );

extern void myPrintf( Uint32 cases, char * fmt, ...);



/********** Inline Function **********/

__INLINE void removePidBuf( PidBuf *pt_pidBuf )
{
  free( (void *) getPidBuf( queGet( pt_pidBuf->link.prev ) ) );
}

