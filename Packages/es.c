/* @(#)es.c	1.13  09/20/99  @(#) */
/*
 *  ======== Video Elementrary Stream Layer ========
 *
 *
 */
#include <stdio.h>
#include "mydefs.h"
#include "que.h"
#include "es.h"
#include "tp.h"

enum {
    NOT_SYNC        = 0x0,
    FIRST_ZERO      = 0xff,
    SECOND_ZERO     = 0xfe,
    SYNC            = 0xfd,
    ES_HEADER       = 0xfc
};

enum {
    PICTURE_HEADER  = 0x00,
    SLICE_STARTING  = 0x01,
    USER_DATA       = 0xb2,
    SEQUENCE_HEADER = 0xb3,
    SEQUENCE_ERROR  = 0xb4,
    EXTENSION_CODE  = 0xb5,
    SEQUENCE_END    = 0xb7,
    GOP_HEADER      = 0xb8
};

enum {  /* extension start code identifier */
    SEQUENCE_EXT_ID                 = 1,
    SEQUENCE_DISPLAY_EXT_ID         = 2,
    QUANT_MATRIX_EXT_ID             = 3,
    COPYRIGHT_EXT_ID                = 4,
    SEQUENCE_SCALABLE_EXT_ID        = 5,
    PICUTRE_DISPLAY_EXT_ID          = 7,
    PICTURE_CODING_EXT_ID           = 8,
    PICTURE_SPATIAL_SCALABLE_EXT_ID = 9,
    PICTURE_TEMPORAL_SCALABLE_EXT_ID= 10
};

QueElem		statFreeQueHead;
QueElem		statFullQueHead;

QueElem		audioFreeFrames;
QueElem		audioFullFrames;

static StatElem	statQue[ MAX_FRAMES_WITHOUT_OVERFLOW ];
static Uint8	esHdrBuf[ MAX_ESHDR_LENGTH ];
static ESCtrl	esCtrl;
static Uint32	printFlag = 0;

static AudFrame audioFrames[ MAX_AUDIO_FRAMES_WO_OVERFLOW ];

static void	parseESHdr( void );
static Uint32	getBits( Uint8 *p, Uint32 offset, Uint32 nbits );

void	checkFullness ( void );

static char	picType[]="xIPBDxxx";

Uint32		dtsInterval = 1501;

/*
 *  ======== esInit ========
 *
 *
 */
void	esInit( void )
{
    Int32   i;

    queInit ( &statFullQueHead );
    queInit ( &statFreeQueHead );

    for ( i = 0; i < MAX_FRAMES_WITHOUT_OVERFLOW; i++ ) {
	memset( &statQue[i], 0, sizeof(StatElem) );
	quePut( &statFreeQueHead, (QueElem *)&statQue[i].link );
    }
    memset( &esCtrl, 0, sizeof(ESCtrl) );

    esCtrl.esHdrPtr = esCtrl.esHdrBuf = esHdrBuf;

    queInit ( &audioFreeFrames );
    queInit ( &audioFullFrames );

    for ( i = 0; i < MAX_AUDIO_FRAMES_WO_OVERFLOW; i++ ) {
	memset( &audioFrames[i], 0, sizeof(AudFrame) );
	quePut( &audioFreeFrames, (QueElem *)&audioFrames[i].link );
    }
}
	

/*
 *  ======== esProcess ========
 *
 *  Process the Elementary Stream.
 */
void esProcess ( Uint8 *esSrc, Uint32 payloadSize )
{
    Int32   i = payloadSize;
    Uint8   byte;
    StatElem	*statQue;

    if ( queEmpty(&statFreeQueHead) ) {
	printf("Error: there is too many frames staying in the decoder buffer\n");
	exit(1);
    }

    /* buffer fullness is checked here */
    tpPesHdr.bufFullness += payloadSize;
    tpPesHdr.pktFullness++;

    if ( esCtrl.statQue != NULL ) {
	esCtrl.statQue->packets++;
    }
    
    checkFullness();

    while ( i-- > 0 )
    {
        byte = *esSrc++;

        switch ( esCtrl.esState )
        {
            case NOT_SYNC:
                esCtrl.esState = ( byte == 0 ) ? FIRST_ZERO : NOT_SYNC;

                if ( esCtrl.statQue != NULL )
                {
                    if ( esCtrl.esState1 == ES_HEADER )
                    {
                        *esCtrl.esHdrPtr++ = byte;
                    }
                    else
                    {
                        esCtrl.statQue->rcPicSize++;
                    }
                    esCtrl.statQue->picSize++;
                }
                break;

            case FIRST_ZERO:
                esCtrl.esState = ( byte == 0 ) ? SECOND_ZERO : NOT_SYNC;

                if ( esCtrl.statQue != NULL )
                {
                    if ( esCtrl.esState1 == ES_HEADER )
                    {
                        *esCtrl.esHdrPtr++ = byte;
                    }
                    else
                    {
                        esCtrl.statQue->rcPicSize++;
                    }
                    esCtrl.statQue->picSize++;
                }
                break;

            case SECOND_ZERO:
                if ( byte == 1 )
                {
                    esCtrl.esState = SYNC;
                }
                else if ( byte != 0 )
                {
                    esCtrl.esState = NOT_SYNC;
                }
                if ( esCtrl.statQue != NULL )
                {
                    if ( esCtrl.esState1 == ES_HEADER )
                    {
                        *esCtrl.esHdrPtr++ = byte;
                    }
                    else
                    {
                        esCtrl.statQue->rcPicSize++;
                    }
                    esCtrl.statQue->picSize++;
                }
                break;

            case SYNC:
            {   /* found the sync, reset the esState flag */

                esCtrl.esState = NOT_SYNC;

                if ( (byte > PICTURE_HEADER ) && ( byte < USER_DATA ) )
                {   /* must be slice start code */
                    if ( esCtrl.esHdrPtr - esCtrl.esHdrBuf >= MAX_ESHDR_LENGTH ) {
                        printf( "Error: ES hdr exceeds buffer capacity\n");
                        exit ( 1 );
                    }

		    /* reach first slice starting code */
                    if ( byte == SLICE_STARTING && esCtrl.statQue != NULL ) {
			esCtrl.esState1 = NOT_SYNC;
                        esCtrl.statQue->picSize++;
                        esCtrl.statQue->rcPicSize = 4;

			/* resync the DTS with the latest infomation, because
			 * in GI's bitstream, picture start code could occur
			 * before the PES header
			 */
			esCtrl.statQue->dts = tpPesHdr.curDTS - dtsInterval;
			esCtrl.statQue->repField = tpPesHdr.repField;

                        /* correct the Pointer back */
                        esCtrl.esHdrPtr -= 3;

                        /* start to parse out the info contained in the
                         *  entire header buffer.
                         */
                        parseESHdr();

                        /* reset the es buffer */
                        esCtrl.esHdrPtr = esCtrl.esHdrBuf;
                    }
                    else if ( esCtrl.statQue != NULL )
                    {   /* add the slice header size */
                        esCtrl.statQue->picSize++;
                        esCtrl.statQue->rcPicSize++;
                    }
                }
                else if ( byte == 0xb6 )
                {
                    myPrintf( VERBOSE, "Bad header 000001b6\n" );
                }
                else
                {   /* must be one of the other ES start codes. */
                    if ( esCtrl.esState1 == NOT_SYNC )
                    {
                        esCtrl.esState1            = ES_HEADER;
                        *(Uint32 *)esCtrl.esHdrPtr = 0x00000100;
                        *(esCtrl.esHdrPtr + 3)     = byte;
                        esCtrl.esHdrPtr           += 4;

                        /* put the old statQue to the correct place */
                        if ( esCtrl.statQue != NULL )
                        {
                            esCtrl.statQue->rcPicSize -= 3;
                            esCtrl.statQue->picSize   -= 3;

                            myPrintf(SHOW_ES_HEADER,"Picture size is %d, %d is reducable\n",
                                      esCtrl.statQue->picSize, esCtrl.statQue->rcPicSize);

                            quePut( &statFullQueHead, (QueElem *)(esCtrl.statQue) );

			    checkFullness();
			}

                        /* get a new statQue, start a new picture */
                        esCtrl.statQue = (StatElem *)queGet( &statFreeQueHead );
                        esCtrl.statQue->picCnt = tpPesHdr.picCount++;
                        esCtrl.statQue->picSize   = 4;
                        esCtrl.statQue->rcPicSize = 0;
                        esCtrl.statQue->dts = tpPesHdr.curDTS;

			/* added by FW */
			esCtrl.statQue->repField = tpPesHdr.repField;
			esCtrl.statQue->packets = 0;
			esCtrl.statQue->pcr = tpPesHdr.curPcr;

			/* underflow checking */
			if ( tpPesHdr.curPcr >= tpPesHdr.curDTS ) {
			    printf("Error: at %dth picture, %dth packets, underflow (PCR: 0x%x, DTS: 0x%x delta: %d)\n",
				   (tpPesHdr.picCount - 1), tpPesHdr.pktCounter, tpPesHdr.curPcr, tpPesHdr.curDTS, (tpPesHdr.curPcr-tpPesHdr.curDTS));
			    esCtrl.statQue->underFlow = 1;
			}
			else {
			    esCtrl.statQue->underFlow = 0;
			}

			/* update the tpPesHdr, curDTS becomes as next frame DTS */
			tpPesHdr.curDTS += dtsInterval;
		    }
                    else
                    {
                        esCtrl.statQue->picSize++;
                        *esCtrl.esHdrPtr++ = byte;
                    }
                }
                break;
            }
            default: {
                break;
	    }
        } /* end of switch */
    } /* end of while */
} /* end of esProcess */

/*
 *  ======== parseESHdr ========
 *
 *  parse ES header and display the result. No real processing is done here.
 */
static void parseESHdr( void )
{
    Uint32  hdrBitSize, esHdrSize;
    Uint32  type, temp, i;
    Uint8   *ptr = esCtrl.esHdrBuf;
    extern  Uint32 showFlag;

    /* now print out all of the bits in this ES hdr buf */
    esHdrSize = esCtrl.esHdrPtr - esCtrl.esHdrBuf;

    myPrintf(SHOW_ES_HEADER, "\n\n\tES Header length %d, Display Entire Header:\n",
	     esCtrl.esHdrPtr - esCtrl.esHdrBuf);

    for ( i = 0; i < esHdrSize; i++ )
    {
	if ((i&3) == 0 ) myPrintf(SHOW_ES_HEADER, "0x" );

	if ( showFlag & SHOW_ES_HEADER )    /* some how vprintf will not work with %02X */
	    printf( "%02X", *(esCtrl.esHdrBuf + i));

	if ( ((i+1) & 31) == 0 ) myPrintf(SHOW_ES_HEADER, "\n" );
        else if ( ((i+1) & 3) == 0 ) myPrintf(SHOW_ES_HEADER, " " );
    }

    if ( i & 31 ) myPrintf(SHOW_ES_HEADER, "\n" );

    do
    {   /* scan through the entire ES header buffer */
        if ( *ptr == 0 && *(ptr+1) == 0 && *(ptr+2) == 1 )
        {   /* found start code prefix 0x000001 */
            switch ( *(ptr+3) )
            {
                case PICTURE_HEADER:
                {
                    myPrintf(SHOW_ES_HEADER, "PICHDR for %dth Pic,", tpPesHdr.picCount );
		    printFlag = 0;  /* reset the printFlag */

                    /* seek past picture start code and temp reference (42 bits) */
                    type = getBits( ptr, 42, 3 );

                    /* also include pic_coding_type and vbv_delay */
                    hdrBitSize = 32+10+3+16;

                    if ( type == 2 )
                    {   /* P picture, include full_pel_forward_vector
                         * and forward_f_code
                         */
                        hdrBitSize += 4;
                    }
                    else if ( type == 3 )
                    {   /* B picture, also include full_pel_backward_vector
                         * and backward_f_code
                         */
                        hdrBitSize += 8;
                    }

                    while ( getBits ( ptr, hdrBitSize, 1 ) != 0 )
                    {   /* extra_bit_picture and extra_infor_picture bits */
                        hdrBitSize += 9;
                    }

                    /* extra_bit_picture (value 0) */
                    hdrBitSize += 1;

                    /* byte align */
                    if ( ( temp = (hdrBitSize & 0x7) ) != 0 )
                    {
                        hdrBitSize += (8 - temp);
                    }

                    myPrintf(SHOW_ES_HEADER,"type %c with header length %d bits\n",
                              picType[type], hdrBitSize );
                    ptr += (hdrBitSize >> 3);

                    break;
                }
                case SEQUENCE_HEADER:
                {
		    Uint32  hsize, vsize, frame_rate_code;

                    myPrintf(SHOW_ES_HEADER, "Sequence Header at %dth Pic,", tpPesHdr.picCount);

		    hsize = getBits ( ptr, 32, 12 );
		    vsize = getBits ( ptr, 32 + 12, 12 );
		    frame_rate_code = getBits ( ptr, 32 + 12 + 12 + 4, 4 );

		    if ( (frame_rate_code == 3) && (vsize > 480) ) {
			/* this is a PAL source */
			dtsInterval = 1800;
		    }
                    myPrintf( SHOW_ES_HEADER, "horizontal_size %d, vertical_size %d, frame_rate_code %d\n",
			     hsize, vsize, frame_rate_code );

                    hdrBitSize = 32+12+12+4+4+18+1+10+1+1;  /* 95 */

                    if ( getBits( ptr, 94, 1 ) )
                    {
                        myPrintf(SHOW_ES_HEADER, "Intra Quan Matrix");
                        hdrBitSize += 8*64;
                    }

                    if ( getBits( ptr, hdrBitSize, 1 ) )
                    {
                        myPrintf(SHOW_ES_HEADER, "nonIntra Quan Matrix");
                        hdrBitSize += 8*64;
                    }
                    hdrBitSize += 1;
                    myPrintf(SHOW_ES_HEADER, "hdrSize %d bits\n", hdrBitSize);
                    ptr += (hdrBitSize >> 3);
                    break;
                }
                case EXTENSION_CODE:
                {
                    temp = (Uint32)(*(ptr + 4) >> 4);
                    myPrintf(SHOW_ES_HEADER, "Extension code %d\n", temp);
                    ptr += 4;

                    switch ( temp )
                    {
                        case SEQUENCE_EXT_ID:
                            break;
                        case SEQUENCE_DISPLAY_EXT_ID:
                            break;
                        case QUANT_MATRIX_EXT_ID:
                            break;
                        case COPYRIGHT_EXT_ID:
                            break;
                        case PICUTRE_DISPLAY_EXT_ID:
                            break;

                        case PICTURE_CODING_EXT_ID:
                            if ( getBits( ptr, 65, 1) )
                            {
                                /* extra 2 bits make header byte aligned */
                                hdrBitSize = 66 + 20 + 2;
                            }
                            else
                            {  /* 6 is to make header byte aligned */
                                hdrBitSize = 66 + 6;
                            }
                            ptr += ( (hdrBitSize >> 3) - 4 );
                            myPrintf(SHOW_ES_HEADER, "Picture coding extention with %d bits\n", hdrBitSize);
                            break;

                        case PICTURE_SPATIAL_SCALABLE_EXT_ID:
                            break;

			default: {
                            myPrintf(VERBOSE, "Not supported or bad ext header code 0x%x\n", temp);
                            break;
			}
                    }
                    break;
                }
                case USER_DATA:
                {
                    ptr += 4;
                    myPrintf(SHOW_ES_HEADER, "User data\n");
                    break;
                }
                case GOP_HEADER:
                {
                    ptr += 4;
                    myPrintf(SHOW_ES_HEADER, "Gop Header\n");
                    break;
                }
                default:
                {
                    ptr += 4;
                    myPrintf(VERBOSE, "Unrecognizable header 0x000001%x\n", *(ptr+3) );
                    break;
                }
            } /** end switch **/
	}
        else
        {   /* skip unsupported content parse */
            ptr++;
        }   /* end if ( *ptr == 0x000001 ) */
    } while ( ptr < esCtrl.esHdrPtr );
}

/*
 *  ======== getBits ========
 *
 *  the function returns the value in a bit stream buffer
 *  with bitPostion "offset", and the value contains
 *  "nbits" bits. Obviously, nbits <= 32
 */
static Uint32 getBits( Uint8 *p, Uint32 offset, Uint32 nbits )
{
    Uint32  result;
    Uint8   c;
    static Uint8    hibits[] = {0xff,0x7f,0x3f,0x1f,0x0f,0x07,0x03,0x01};
    int            bitsthisbyte;

    result = 0L;
    p += offset >> 3;
    offset &= 7;

    while ( nbits )
    {
        c = *p++;
        c &= hibits[offset];
        bitsthisbyte = 8 - offset;

        if (bitsthisbyte > nbits)
        {
            c >>= bitsthisbyte - nbits;
            bitsthisbyte = nbits;
        }
        result <<= bitsthisbyte;
        result |= c;
        offset = 0;
        nbits -= bitsthisbyte;
    }
    return result;
}

/*
 *  ======== checkFullness ========
 *
 *
 */
void	checkFullness ( void )
{
    StatElem	*statQue;

    while ( !queEmpty(&statFullQueHead) ) {
	statQue = (StatElem *)statFullQueHead.next;
	
	if ( tpPesHdr.curPcr >= statQue->dts ) {

	    tpPesHdr.bufFullness -= statQue->picSize;
	    tpPesHdr.pktFullness -= statQue->packets;

	    if ( statQue->underFlow == 0 ) {
		myPrintf( SHOW_BUF_INFO, "at %dth pic, buffer fullness is %d bits, %d packets\n",
			 statQue->picCnt, tpPesHdr.bufFullness * 8, tpPesHdr.pktFullness );
		myPrintf( SHOW_BUF_INFO, "\t(PCR, DTS) = (0x%lx, 0x%lx)\n", statQue->pcr, statQue->dts);
	    }

	    statQue = (StatElem *)queGet(&(statFullQueHead));
	    quePut( &statFreeQueHead, (void *)statQue );
	}
	else {
	    break;
	}
    }

    if ( tpPesHdr.bufFullness > BUF_FULLNESS_UPPER_BOUND && !printFlag ) {
	printf("Error: at %dth packets, buffer overflow %d, %d pictures are in decoder buffer\n",
	       tpPesHdr.pktCounter, tpPesHdr.bufFullness, tpPesHdr.picCount - statQue->picCnt );
	printFlag = 1;
    }

    if ( tpPesHdr.bufFullness < 0  && !printFlag ) {
	printf("Error_FW: at %dth picture, %dth packets, buffer underflow %d\n",
	       tpPesHdr.picCount, tpPesHdr.pktCounter, tpPesHdr.bufFullness );

	printFlag = 1;
    }
}
