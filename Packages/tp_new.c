/* @(#)tp.c	1.11  09/21/99  @(#) */
/*
 *  ======== Tranport Packet ========
 *
 *  handle transport packet
 */
#include <stdio.h>
#include <string.h>
#include "mydefs.h"
#include "pes.h"
#include "tp.h"
#include "utils.h"

#include "es.h"

/* global varible */
TpPESHeader tpPesHdr;

static void readPesHdr     ( Uint32 * buf, Uint32 tpDataSize );
static void readMorePesHdr ( Uint32 * buf, Uint32 tpDataSize );
static void dumpPayload	   ( Uint8 * src, Uint32 payloadSize );

static Uint8	pesBuf[PES_MAX_HEADER_SIZE];

#ifdef DEBUG
void  tpAFParse ( Uint32 *afBuf, Uint16 afSize );
#endif

extern Uint32	dtsInterval;
extern Uint32	showFlag;

/*
 *  ======== tpPesHdrInit ========
 *
 *  initialize the tpPesHdr.
 */
void	tpPesHdrInit ( void )
{
    Int32  size = (sizeof(TpPESHeader) >> 2);
    Uint32 *ptr = (Uint32 *)&tpPesHdr;

    for( ; size > 0; size-- ) *ptr++ = 0;
    tpPesHdr.state = UNINITIALIZED;
}

/*
 *  ======== findPayload ========
 *
 *  find where the pay load starts, return the offset from the beginning.
 */
__inline Uint16 findPayload( Uint32 adaptation_field_control, Uint32 *buf )
{
    if ( adaptation_field_control & 1 )	{   /* case '01' or case '11' */
        if ( adaptation_field_control != 1 )
        {   /* case '10' */
            Uint8 adaptation_field_length = extU ( *(buf + 1), 0, 24 );

#ifdef DEBUG
            tpAFParse( (buf+1), adaptation_field_length );
#endif
            return ( 183 - adaptation_field_length );
        }
        else
        {
            return ( 184 );
        }
    }
#ifdef DEBUG
    else {
        tpAFParse( (buf+1), 183 );
        return(0);
    }
#else
    else {
        return 0;
    }
#endif
}

/*
 *  ======== tpParse ========
 *
 *  parse the transport packet, see the MVP documentation for the code diagram.
 */
void tpParse ( Uint32 * buf, Uint32 header )
{
    Uint32 adaptation_field_control;
    Uint32 tpDataSize;

    /*** header word:

            |----sync byte (0x47)
            |     |------TEI, PUSI, TP (3 bits total)
            |                      ______ scrambling control
            |                       | ___ AF_control
            |                       | |
       |<======>|<=>               ||||
       |XXXXXXXX|XXXXXXXX|XXXXXXXX|XXXXXXXX|
                    ^            ^
                    |            |
                    |<-  PID   ->|
    ***/

    adaptation_field_control = extU(header, 32 - 4 - 2, 32 - 2);

    /* payload_unit_start_indicator = 1, READ_PES_HEADER   */
    if ( extU (header, 32 - 22 - 1, 32 - 1) )
    {
        tpPesHdr.tpPesBuf     = pesBuf;
        tpPesHdr.curPesHdrPtr = pesBuf;
        tpPesHdr.byteCnt      = 0;
        tpPesHdr.pesHdrSize   = 0;
        tpPesHdr.state        = READ_PES_HEADER;

        if ( (tpDataSize = findPayload( adaptation_field_control, buf )) > 0 )
        {
            readPesHdr( buf, tpDataSize );
        }
    }
    else
    {   /*
         * payload_unit_start_indicator = 0, NO_PES_HEADER emerge in this
         * tranport packet
         */
        if ((tpDataSize = findPayload (adaptation_field_control, buf)) <= 0)
        {
            return;
        }

        switch ( tpPesHdr.state )
        {
            case READ_PES_HEADER:
            {
                readPesHdr( buf, tpDataSize );
                break;
            }
            case READ_MORE_PES_HEADER:
            {
                readMorePesHdr( buf, tpDataSize );
                break;
            }
            case READ_PES_PAYLOAD:
            {
                dumpPayload ( (Uint8 *) buf + TP_SIZE - tpDataSize,
                              tpDataSize );
                break;
            }
            case UNINITIALIZED:
            {
		myPrintf(VERBOSE, "Warning: Discarded %dth TP packets\n", tpPesHdr.pktCounter);

		break;
            }
            default:
            {
                printf( "Bad PES header state %d\n", tpPesHdr.state );
                break;
            }
        } /* switch */
    } /* end of pay_load_unit_start_indicator = 0 case */
}

/*
 *  ======== tpAFParse ========
 *
 *  transport parse adaptation field.
 */

#ifdef DEBUG
void tpAFParse ( Uint32 * afBuf, /* address of af_field_length (start of AF) */
                 Uint16 afSize   /* AF size in number of bytes !!! **/ )
{
#if 0
    /* FW_TODO: incomplete right now */
    printf( "Error: code not written yet.\n" );
#endif
}
#endif

/*
 *  ======== tpAFGetPCR ========
 *
 *  extract a PCR from a transport packet adaptation field.
 *
 *  buf holds the starting byte position of the transport packet.
 *  In big endian mode, this byte is 0x47, in little endian mode,
 *  this byte is byte 3 of the packet. But on a 32-bit word, the
 *  sync byte 0x47 is always assumed to be the MS Byte.
 *
 */
Uint32 tpAFGetPCR ( Uint32 * buf /** start address of TP header **/ )
{
    Uint32 pcr = 0;
    Uint32 afWord1;
    Uint32 afWord2;

    if ( extU ( *buf, 26, 31 ) )    /* extract the bit 6 */
    {   /* has AF */
        afWord1 = *(buf + 1); /** bytes 4-7 of TP **/

        if ( !extU(afWord1, 0, 24) )
        {
	    myPrintf( VERBOSE, "Warning: af_length==0 !! \n" );
	    return ( pcr );
        }

	if ( extU(afWord1, 11, 31) )
        {   /* pcr_flag = 1 */
            afWord2 = *(buf + 2);		/** bytes 8-11 of TP **/
            pcr  = (afWord1 << 16);		/** PCR_base(bits32-17) **/
            pcr |=  extU( afWord2, 0,  16 );	/** PCR_base(bits16-1) **/
        }
    }
    return (pcr);
}

/*
 *  ======== readPesHdr ========
 *
 *  The function is called in state READ_PES_HEADER
 *  IMPORTANT NOTE: TP payload is assumed to be word aligned. In other words,
 *  tpDataSize is assumed to be multiple of 4. This is not
 *  required by the MPEG spec, but appears to be what every one uses.
 *  In addition, dmaByteCopy can not copy data not word aligned.
 *  If non-word aligned data situation indeed occurs in some stream,
 *  this program and all progarms that use 'buf' as input must be modified.
 */
static void readPesHdr( Uint32 * buf,     /** start address of TP header **/
                        Uint32 tpDataSize /** data size in number of bytes **/ )
{
#if 1
  Uint32   tpPyldLoc, hedrLgthLoc;

  tpPyldLoc = TP_SIZE - tpDataSize;
  hedrLgthLoc = tpPyldLoc + 8;

  if ( hedrLgthLoc < 188 )
  {
    tpPesHdr.pesHdrSize = (extU( *(buf + (hedrLgthLoc >> 2)), ((hedrLgthLoc & 0x3) << 3), 24 ) +
			   PES_HEADER_MINIMUM_SIZE);
#if 0
    printf( "............... tpDataSize: %d, pesHedrSize: %d\n", tpDataSize, tpPesHdr.pesHdrSize );
#endif
    tpPesHdr.state = READ_MORE_PES_HEADER;
    readMorePesHdr( buf, tpDataSize );
  }
  else
  {
    memByteCopy( (Uint8 *) tpPesHdr.curPesHdrPtr, tpDataSize, (Uint8 *) buf + tpPyldLoc,
		 (Uint32) tpPesHdr.curPesHdrPtr & 0x3, (Uint32) buf & 0x3 );
    /* update PES buffer pointer */
    tpPesHdr.curPesHdrPtr += tpDataSize;
    tpPesHdr.byteCnt      += tpDataSize;
  }

#else
    int i;
    Uint32  *dst;
    Uint32  *src;
    Uint32  *tpPayloadAddr;
    Uint32   tpPyldLoc = buf + ((TP_SIZE - tpDataSize) >> 2);

    if ( tpDataSize >= (PES_HEADER_MINIMUM_SIZE - tpPesHdr.byteCnt) )
    {	/*
	 * Enough bytes in the TP payload to complete the first 9 PES hdr
         * bytes. Therefore, we can obtain the PES header size from the
         * PES_header_data_length field.
         */
        /* pesHdrSize is in number of bytes */

	tpPesHdr.pesHdrSize = extU( *(tpPayloadAddr + ((PES_HEADER_MINIMUM_SIZE - 1)>>2)), 0, 24 ) +
	                      PES_HEADER_MINIMUM_SIZE;

        tpPesHdr.state = READ_MORE_PES_HEADER;
        readMorePesHdr( buf, tpDataSize );
    }
    else
    {   /* Not enough bytes in the TP payload to complete the first 9 PES
         * hdr bytes. Just copy the entire TP payload into the PES header
         * array. In this case, clearly tpDataSize < 9.
         */
        memcpy ( (Uint8 *) tpPesHdr.curPesHdrPtr,(Uint8 *) tpPayloadAddr, tpDataSize );

        /* update PES buffer pointer */
        tpPesHdr.curPesHdrPtr += tpDataSize;
        tpPesHdr.byteCnt      += tpDataSize;
    }
#endif
}

/*
 *  ======== readMorePesHdr ========
 *
 *  The function is called in state READ_MORE_PES_HEADER, assume that PES
 *  header length has been found already.
 *  IMPORTANT NOTE: TP payload is assumed to be word aligned. In other words,
 *  tpDataSize is assumed to be multiple of 4. This is not
 *  required by the MPEG spec, but appears to be what every one uses.
 *  In addition, dmaByteCopy can not copy data that is not word aligned.
 *  If non-word aligned data situation indeed occurs in some streams,
 *  this program and all progarms that use 'buf' as input must be modified.
 */
static void readMorePesHdr ( Uint32 * buf, /* start address of TP header */
                             Uint32   tpDataSize   /* in number of bytes */)
{
    int	     i;
    Uint32  *src;
    Uint32  *dst;
    Uint32   dts;
    Int32    nHdrBytesLeft      = tpPesHdr.pesHdrSize - tpPesHdr.byteCnt;
    Int32    nESPayloadLeftInTP = tpDataSize - nHdrBytesLeft;
#if 1
    Uint32   tpPyldLoc          = TP_SIZE - tpDataSize;
    Uint8   *tpPayloadAddr      = (Uint8 *) buf + (TP_SIZE - tpDataSize);
#else
    Uint32  *tpPayloadAddr      = buf + ((TP_SIZE - tpDataSize) >> 2);
#endif

    if ( nESPayloadLeftInTP >= 0 )
    {   /* at least the PES header is complete. */
        tpPesHdr.state = READ_PES_PAYLOAD;
#if 1
	memByteCopy( (Uint8 *) tpPesHdr.curPesHdrPtr, nHdrBytesLeft, (Uint8 *) buf + tpPyldLoc,
		     (Uint32) tpPesHdr.curPesHdrPtr & 0x3, (Uint32) buf & 0x3 );
#else
        memcpy(tpPesHdr.curPesHdrPtr, tpPayloadAddr, nHdrBytesLeft);
#endif

        tpPesHdr.curPesHdrPtr = NULL;
        tpPesHdr.byteCnt      = 0;
        tpPesHdr.pesHdrSize   = 0;

	dts = pesGetDTS( (Uint8 *) tpPesHdr.tpPesBuf );
#if 0
	printf( "................ DTS 0x%08x (0x%08x)\n", dts, (tpPesHdr.curDTS - dtsInterval) );
#endif

        if ( dts == 0 )
        {   /* does not contain DTS, throw the PES header away */
            if ( tpPesHdr.picCount == 0 )
            {
                tpPesHdr.state = UNINITIALIZED;
            }
        }
        else
        {
            if ( tpPesHdr.picCount == 0 )
            {   /* the very first DTS */
                tpPesHdr.curDTS = dts;
                tpPesHdr.picCount = 1;

                tpPesHdr.firstPCR += (Uint32)((float)(tpPesHdr.pktCounter - 1)
					      * tpPesHdr.pcrDelta);
            }
            else
            {
                Uint32	fieldCnt, picNum;
		int	dtsDiff = (int) (dts - tpPesHdr.curDTS + dtsInterval);
		/*
                 * calculate the picture counter for PES header, and use
                 * it to align the PES header with the output picture
                 */
		tpPesHdr.repField = 0;

		if ( dtsDiff < 0 ) {
		    printf("ERROR: at %d packets, %dth pic, dts goes back by %d, (dts 0x%x, prevDts 0x%x)\n",
			   tpPesHdr.pktCounter, tpPesHdr.picCount, -dtsDiff, dts, tpPesHdr.curDTS );
		}
		else if ( dtsDiff > 2 * dtsInterval - 10 ) {
		    printf( "ERROR: illegal dts jumps(%d), (dts 0x%x, prevDts 0x%x)\n",
			    dtsDiff, dts, (tpPesHdr.curDTS - dtsInterval) );
		}
		else if ( dtsDiff < 20 ) {
		    /* Note: because of the GI's bitstream could have Picture Start Code
		     * before the PES header, which is illegal, it could confuse STD's
		     * DTS checking, we need to take care this case here.
		     */
		    if ( (showFlag & VERBOSE) != 0 )
			printf("WARNING: %dth picture, Picture header happens before the PES header\n",
			       tpPesHdr.picCount );
		    dts += dtsInterval;
		}
		else {
		    fieldCnt = ( (dtsDiff << 1) + (dtsInterval >> 2) ) / dtsInterval;

		    tpPesHdr.repField = ( fieldCnt == 2 ) ? 0 : 1;

		    /* we have already increment tpPesHdr.picCount in es.c, so we
		     * substract one here
		     */
		    picNum = tpPesHdr.picCount + (fieldCnt >> 1) - 1;
		    tpPesHdr.picCount = picNum;
		}
		tpPesHdr.curDTS = dts;
            }
        }

        if ( nESPayloadLeftInTP > 0 && tpPesHdr.state != UNINITIALIZED )
        {
            dumpPayload( (Uint8 *) tpPayloadAddr + nHdrBytesLeft,
                         nESPayloadLeftInTP );
        }
    }
    else
    {
#if 1
	memByteCopy( (Uint8 *) tpPesHdr.curPesHdrPtr, tpDataSize, (Uint8 *) buf + tpPyldLoc,
		     (Uint32) tpPesHdr.curPesHdrPtr & 0x3, (Uint32) buf & 0x3 );
#else
        /* no ES payload bytes, just copy the bytes into PES hdr and continue */
        memcpy( tpPesHdr.curPesHdrPtr, (Uint8 *) tpPayloadAddr, tpDataSize );
#endif

        tpPesHdr.curPesHdrPtr += tpDataSize;
        tpPesHdr.byteCnt      += tpDataSize;
    }
}

/*
 *  ======== dumpPayload ========
 *
 *
 */
static void dumpPayload ( Uint8 * src, Uint32 payloadSize )
{
    esProcess ( src, payloadSize );

    if ( tpPesHdr.outFp != NULL ) {
	fwrite( src, 1, payloadSize, tpPesHdr.outFp );
    }
}

/*
 *  ======== tpAudioParse ========
 *
 *  Inspect the audio transport packet.
 *
 *  Assumption: In order to quickly figure out audio DTS,
 *  I assume that the audio PES header will end in the same
 *  transport packet. And the PES header starts at a word
 *  boundary condition. Since so far, we did not encounter any
 *  exception yet.
 */
void tpAudioParse ( Uint8 *tpbuf, Uint32 tpHdr )
{
    Uint32  afctrl, dts, tpDataSize, esPayloadSize, pesHdrSize, temp = 0;
    Uint8   *pesPtr, *esPtr;
    AudFrame	*audFrame = (AudFrame *)tpPesHdr.audframe;
    Uint8   pesBuf[32];

    afctrl = extU( tpHdr, 32 - 4 - 2, 32 - 2 );

    /* payload_unit_start_indicator = 1, has audio PES header */
    if ( extU( tpHdr, 32 - 22 - 1, 32 - 1 ) )
    {
        if ( (tpDataSize = findPayload(afctrl, (Uint32 *)tpbuf)) > (PES_HEADER_MINIMUM_SIZE + 5) )
        {
	    pesPtr = tpbuf + TP_SIZE - tpDataSize;

	    /* check this pointer whether it is aligned on word boundary */
	    if ( ((Uint32)pesPtr & 0x3) != 0 ) {
		/* check this pointer whether it is aligned on word boundary
		 * if not make it word aligned
		 */
		memcpy( (Uint8 *)pesBuf, pesPtr, 20 );
		pesPtr = (Uint8 *)pesBuf;
	    }

	    dts = pesGetDTS(pesPtr);

	    if ( (showFlag & VERBOSE) != 0 )
		myPrintf( SHOW_AUDIO_BUF, "Audio Frame: DTS-PCR = %ld\n",dts - tpPesHdr.curPcr);
	    
	    /* total PES header length = pes_header_length + 9 */
	    pesHdrSize = *(pesPtr + 8) + 9;
	    esPtr = tpbuf + TP_SIZE - tpDataSize + pesHdrSize;

	    tpDataSize -= pesHdrSize;

	    esPayloadSize = tpDataSize;

	    if ( tpPesHdr.outAudioFp != NULL ) {
		fwrite( esPtr, 1, esPayloadSize, tpPesHdr.outAudioFp );
	    }

	    if ( (int)(tpPesHdr.curPcr - dts) > 0 ) {
		/* underflow already */
		printf("Audio buffer underflows,\t(pcr, dts) = (0x%lx, 0x%lx), (PCR-DTS)=%ld\n",
		       tpPesHdr.curPcr, dts, (int)(tpPesHdr.curPcr - dts) );
		return;
	    }

	    do {
		if ( (*esPtr == 0x0b && *( esPtr + 1 ) == 0x77 &&
			(*( esPtr + 5 ) & 0xf8) == 0x40)
			|| ( (*esPtr == 0xff) && ((*(esPtr + 1) & 0xf0) == 0xf0)) ) {

		    /* find the audio ES sync code, starting a new frame */
		    if ( audFrame != NULL ) { /* not the first time come over */
			temp = audFrame->counter;
			quePut(&audioFullFrames, (QueElem *)audFrame);
		    }

		    audFrame = (AudFrame *)queGet(&audioFreeFrames);
		    audFrame->dts = dts;
		    audFrame->size = tpDataSize;
		    audFrame->packets = 1;
		    audFrame->counter = temp + 1;

		    tpPesHdr.audframe = (void *)audFrame;

		    tpDataSize = 0;
		}
		else {
		    /* to avoid the very first NULL audio frame pointer */
		    if ( audFrame != NULL ) audFrame->size++;
		    tpDataSize--;
		    esPtr++;
		}
	    } while ( tpDataSize != 0 );
        }

	else {
	    printf("Error: audio PES packet does not complete in one packet!\n");
	}
    }
    else if ( ( (esPayloadSize = findPayload(afctrl, (Uint32 *)tpbuf)) != 0 )
	     && ( audFrame != NULL ) ) {

	audFrame->size += esPayloadSize;
	audFrame->packets++;

	if ( tpPesHdr.outAudioFp != NULL ) {
	    esPtr = tpbuf + TP_SIZE - esPayloadSize;
	    fwrite( esPtr, 1, esPayloadSize, tpPesHdr.outAudioFp );
	}
    }

    /* buffer fullness is checked here */
    if ( audFrame != NULL  && tpPesHdr.curPcr != 0 ) {
	tpPesHdr.audioBufFullness += esPayloadSize;
    }

    if ( !queEmpty(&audioFullFrames) ) {
	audFrame = (AudFrame *)audioFullFrames.next;

	if ( tpPesHdr.curPcr >= audFrame->dts ) {
	    tpPesHdr.audioBufFullness -= audFrame->size;

	    myPrintf( SHOW_AUDIO_BUF, "at %dth audio frame, buffer fullness is %d, frame size = %d\n",
		     audFrame->counter, tpPesHdr.audioBufFullness, audFrame->size );

	    if ( (showFlag & VERBOSE) != 0 )
		myPrintf( SHOW_AUDIO_BUF, "\t(pcr, dts) = (0x%lx, 0x%lx) PCR-DTS = %ld\n",
			 tpPesHdr.curPcr, audFrame->dts, (tpPesHdr.curPcr-audFrame->dts) );

	    audFrame = (AudFrame *)queGet(&(audioFullFrames));
	    quePut( &audioFreeFrames, (QueElem *)audFrame );
	}

	if ( tpPesHdr.audioBufFullness > AUDIO_BUF_FULLNESS_UPPER_BOUND ) {
	    printf("Error: at %dth audio frame, %dth packets, buffer overflow %d\n",
		   audFrame->counter, tpPesHdr.pktCounter, tpPesHdr.audioBufFullness );
	}

	if ( tpPesHdr.audioBufFullness < 0 ) {
	    printf("Error: at %dth audio frame, %dth packets, buffer underflow %d\n",
		   audFrame->counter, tpPesHdr.pktCounter, tpPesHdr.audioBufFullness );
	}
    }
}
