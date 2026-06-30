/* @(#)tp.h	1.5  09/14/99  @(#) */
/*
 *  ======== tp.h ========
 *
 *  transport packet header file.
 */
#ifndef _TP_H_
#define _TP_H_

#define TP_SIZE			    188
#define TP_SIZE_IN_WORDS            47
#define TP_SIZE_IN_BITS             1504

#define PAT_PID                     0x0
#define CAT_PID                     0x1
#define NULL_PID                    0x1FFF

#define RESERVED_PID                0x2
#define TP_SYNC_BYTE                0x47

#define TP_PCR_CLOCK_RATE_IN32BITS  45000

#define	AUDIO_BUF_FULLNESS_UPPER_BOUND	3584

typedef enum {
    UNINITIALIZED,
    READ_PES_HEADER,
    READ_MORE_PES_HEADER,
    READ_PES_PAYLOAD
} TpParseState;

/*
 *  transport header definition, see MPEG2 system manual Page 20
 */
typedef struct TpHeader {
    Uint32  sync_byte                    : 8;
    Uint32  transport_error_indicator    : 1;
    Uint32  payload_unit_start_indicator : 1;
    Uint32  transport_priority           : 1;
    Uint32  PID                          : 13;
    Uint32  transport_scrambling_control : 2;
    Uint32  adaptation_field_control     : 2;
    Uint32  continuity_counter           : 4;
} TpHeader;

/*
 *  transport header adaptation definition, see MPEG2 system manual Page 22
 */
typedef struct TpAdaptationField {
    Uint32  adaptation_field_length                : 8;
    Uint32  discontinuity_indicator                : 1;
    Uint32  random_access_indicator                : 1;
    Uint32  elementary_stream_priority_indicator   : 1;
    Uint32  PCR_flag                               : 1;
    Uint32  OPCR_flag                              : 1;
    Uint32  splicing_point_flag                    : 1;
    Uint32  transport_private_data_flag            : 1;
    Uint32  adaptation_field_extension_flag        : 1;
    Uint32  program_clock_reference_base_32_17     : 16;
    Uint32  program_clock_reference_base_16_0      : 17;
    Uint32  reserved                               : 6;
    Uint32  program_clock_reference_base_extension : 9;
} TpAdaptationField;

typedef struct TpPESHeader {
    Uint8   *tpPesBuf;    /* points to the pes header memory                */
    Uint8   *curPesHdrPtr;/* current PES header pointer                     */
    Uint16  byteCnt;      /* number of bytes has been read by PES header buf*/
    Uint16  pesHdrSize;   /* PES header size in bytes                       */
    Uint32  firstPCR;     /* record the first PCR                           */
    Uint32  curPcr;       /* record the current PCR                         */
    Uint32  lastPcr;      /* record the last PCR in the packet              */
    Uint32  lastLoc;      /* record the last PCR in the packet              */
    Uint32  pktCounter;   /* counting the number of packets has arrived     */
    Uint32  curDTS;       /* record the current frame DTS                   */
    Uint32  repField;     /* repeated field flag                            */
    Uint32  picCount;     /* record the picture count                       */
    Int32   bufFullness;  /* the buffer fullness for the video channel      */
    Int32   pktFullness;  /* the buffer fullness for video in pkts          */
    Int32   audioBufFullness;  /* the buffer fullness for the audio channel */
    TpParseState state;   /* record the state of the state machine          */
    float   pcrDelta;     /* PCR diff between two TPs, only floating number */
    void    *audframe;	  /* remember the current audio frame		    */
    FILE    *outFp;	  /* out put video elementrary file pointer	    */
    FILE    *outAudioFp;  /* out put audio elementrary file pointer	    */
} TpPESHeader;

extern TpPESHeader  tpPesHdr;

extern void   tpPesHdrInit ( void );
extern void   tpParse    ( Uint32 * buf, Uint32 header );
extern Uint32 tpAFGetPCR ( Uint32 * buf );

#endif  /* _TP_H_ */
