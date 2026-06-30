/* @(#)std.c	1.9  09/14/99  @(#) */
/*
 *  ======== main ========
 *
 *  main routine.
 */
#include <stdio.h>
#include <string.h>

#include "mydefs.h"
#include "tp.h"
#include "utils.h"

#define NUM_SYNC	    4
#define SYNC_RANGE	    ((NUM_SYNC + 1) * TP_SIZE)

enum {
    NOT_SYNC	    = -1,
    FIRST_ZERO	    = -2,
    SECOND_ZERO	    = -3,
    SYNC	    = -4,
    PICTURE_HEADER  = 0x00,
    SLICE_STARTING  = 0x01,
    USER_DATA	    = 0xb2,
    SEQUENCE_HEADER = 0xb3,
    EXTENSION_CODE  = 0xb5,
    SEQUENCE_END    = 0xb7,
    GOP_HEADER	    = 0xb8
};

enum {	/* extension start code identifier */
    SEQUENCE_EXT_ID		    = 1,
    SEQUENCE_DISPLAY_EXT_ID	    = 2,
    QUANT_MATRIX_EXT_ID		    = 3,
    COPYRIGHT_EXT_ID		    = 4,
    SEQUENCE_SCALABLE_EXT_ID	    = 5,
    PICUTRE_DISPLAY_EXT_ID	    = 7,
    PICTURE_CODING_EXT_ID	    = 8,
    PICTURE_SPATIAL_SCALABLE_EXT_ID = 9,
    PICTURE_TEMPORAL_SCALABLE_EXT_ID= 10
};

static char *usage =
    "Usage: std [ options ] [ input transport stream file name ]\n"
    "       -b  shows every frame's buffer fullness\n"
    "       -ab shows every audio frame's buffer fullness\n"
    "       -v  shows up all the warning messages\n"
    "       -es shows up all the video elementary stream information\n"
    "       -vpid [video pid, default: no video ]\n"
    "       -apid [audio pid, default: no audio ]\n"
    "       -pcr  [pcr pid, default is: the same as video pid]\n"
    "       -vo output video elementrary stream file name\n"
    "       -ao output audio elementrary stream file name\n";

Uint32 showFlag = 0;

#if WAYNE
extern void audTpParse( Uint32 *tp, Uint32 tpIndx, Uint32 pcr45K );
#endif


/*
 *  ======== main ========
 *
 *
 */
void main ( int argc, char **argv )
{
    FILE    *infp;
    Int32   i;
    Uint32  tpSize, tpHdr, justHasPCRFlag = 0;
    Uint32  emptyness, pid, pcr;
    Uint32  pktSize = TP_SIZE, numPkts = 0, vidPktCnt = 0, audPktCnt = 0;
    Uint32  vpid = 8888, apid = 8888, pcrpid = 8888;
    char    *tpPtr, *p;

    /* parse the command argument */
    if ( argc < 2 || argc > 14 ) {
	printf("%s", usage);
	exit(1);
    }

    /* initialize the tpPesHdr */
    tpPesHdrInit();
    esInit();

    for ( i = 1; i < (argc - 1); i++ ) {
	if ( argv[i][0] == '-' ) {
	    p = argv[i]+1;

	    if ( strcmp(p,"b") == 0 )
		showFlag |= SHOW_BUF_INFO;
	    else if ( strcmp(p,"ab") == 0 )
		showFlag |= SHOW_AUDIO_BUF;
	    else if ( strcmp(p,"es") == 0 )
		showFlag |= SHOW_ES_HEADER;
	    else if ( strcmp(p,"v") == 0 )
		showFlag |= VERBOSE;
	    else if ( strcmp(p,"vpid") == 0 ) {
		i++;
		vpid = atoi(argv[i]);
	    }
	    else if ( strcmp(p,"pcr") == 0 ) {
		i++;
		pcrpid = atoi(argv[i]);
	    }
	    else if ( strcmp(p,"apid") == 0 ) {
		i++;
		apid = atoi(argv[i]);
	    }
	    else if ( strcmp(p,"vo") == 0 ) {
		i++;

		if ( (tpPesHdr.outFp = fopen(argv[i], "wb")) == NULL ) {
		    printf("Error: Can't open file %s\n", argv[i]);
		    exit(1);
		}
	    }
	    else if ( strcmp(p,"ao") == 0 ) {
		i++;

		if ( (tpPesHdr.outAudioFp = fopen(argv[i], "wb")) == NULL ) {
		    printf("Error: Can't open file %s\n", argv[i]);
		    exit(1);
		}
	    }
	    else {
		printf("Error: bad command argument\n");
		printf("%s", usage);
		exit(1);
	    }
	}
    }

    if ( (vpid != 8888) && (pcrpid == 8888) ) {
	/* default setting for pcrpid */
	pcrpid = vpid;
    }
    
    if ( (infp = fopen(argv[i], "rb")) == NULL ) {
	printf("Can't open file %s\n", argv[i]);
	exit(1);
    }

    if ( (tpPtr = (char *)malloc(TP_SIZE)) == NULL ) {
	printf("Can't alloc memory\n");
	exit(1);
    }

    /* at first, need to find sync, then inspects each packet */
    findSync( infp );

    while ( fread ( (Uint8 *)tpPtr, 1, TP_SIZE, infp ) == TP_SIZE ) {
        tpHdr = *(Uint32 *)tpPtr;
        pid = extU(tpHdr, (32 - 8 - 13), (32 - 13));

	tpPesHdr.pktCounter++;

	if ( (tpHdr >> 24) != 0x47 )
        {
            printf("Bad transport sync byte 0x%x\n", (tpHdr >> 24) );
	    exit(0);
	}

	/* need to find out the first PCR before continue */
	if ( !tpPesHdr.firstPCR ) {
            if ( !(pcr = tpAFGetPCR( (Uint32 *)tpPtr )) ) {
		/* there is no PCR in this packet, simply continue */
                continue;
            }

            /* did find the first PCR, but need to confirm that this PCR
             * belongs to the program timebase we are working on.
             */
            if ( pid == pcrpid && tpPesHdr.state == UNINITIALIZED ) {
		/* indeed found the right first PCR */
		tpPesHdr.firstPCR   = pcr;
                tpPesHdr.curDTS     = 0;
                tpPesHdr.lastPcr    = pcr;
                tpPesHdr.curPcr     = pcr;
		tpPesHdr.lastLoc    = tpPesHdr.pktCounter;
		tpPesHdr.pcrDelta   = 0.0;
            }
            else {
		/* this PCR belongs to someone else's timebase, keep looking */
		continue;
            }
	}
	else if ( pid == pcrpid ) {
	    /* find the closest PCR before the first PES emerges */
	    if ( (pcr = tpAFGetPCR( (Uint32 *)tpPtr )) ) {

		tpPesHdr.pcrDelta = ((float)(pcr - tpPesHdr.lastPcr))
			/ (float)(tpPesHdr.pktCounter - tpPesHdr.lastLoc);

		tpPesHdr.lastLoc = tpPesHdr.pktCounter;
		if ( (Int32)( pcr - tpPesHdr.lastPcr ) < 0 ) {
		    printf("Error: At %d packets, PCR is set backward, old:0x%x becomes 0x%x\n",
			   tpPesHdr.pktCounter, tpPesHdr.lastPcr, pcr );
		}
		tpPesHdr.lastPcr = pcr;
		tpPesHdr.curPcr  = pcr;
		justHasPCRFlag = 1;
	    }
	}

	if ( tpPesHdr.pcrDelta != 0.0 && !justHasPCRFlag ) {
	    tpPesHdr.curPcr = tpPesHdr.lastPcr +
		(Uint32)( (float)(tpPesHdr.pktCounter - tpPesHdr.lastLoc) * tpPesHdr.pcrDelta );
	}
	justHasPCRFlag = 0;

	if ( pid == vpid ) {
	    if ( vidPktCnt == 0 ) {
		vidPktCnt = tpHdr & 0x0F;
	    }
	    else if ( ((tpHdr & 0x30) == 0x10) || ((tpHdr & 0x30) == 0x30) ) {
		vidPktCnt++;
	    }
		
	    if ( (vidPktCnt & 0xF) != (tpHdr & 0x0F) ) {
		printf("Error: video lost %d packet at packets %d\n",
		      ((tpHdr & 0x0f) - (vidPktCnt & 0xf)), tpPesHdr.pktCounter );
		vidPktCnt = (vidPktCnt & 0xfffffff0) + (tpHdr & 0xf);
	    }

	    tpParse( (Uint32 *)tpPtr, tpHdr );
	}
	else if ( pid == apid ) {

	    if ( audPktCnt == 0 ) {
		audPktCnt = tpHdr & 0x0F;
	    }
	    else if ( ((tpHdr & 0x30) == 0x10) || ((tpHdr & 0x30) == 0x30) ) {
		audPktCnt++;
	    }
		
	    if ( (audPktCnt & 0xF) != (tpHdr & 0x0F) ) {
		printf("Error: audio lost %d packet at packets %d\n",
		      ((tpHdr & 0x0f) - (audPktCnt & 0xf)), tpPesHdr.pktCounter );
		audPktCnt = (audPktCnt & 0xfffffff0) + (tpHdr & 0xf);
	    }
#if !WAYNE
	    tpAudioParse( tpPtr, tpHdr );
#else
	    audTpParse( (Uint32 *) tpPtr, tpPesHdr.pktCounter, tpPesHdr.curPcr );
#endif
	}
    }

    free(tpPtr);
    fclose ( infp );

    if ( tpPesHdr.outFp != NULL ) {
	fclose ( tpPesHdr.outFp );
    }

    if ( tpPesHdr.outAudioFp != NULL ) {
	fclose ( tpPesHdr.outAudioFp );
    }
}


/*
 *  ======== findSync ========
 *
 */
Int32  findSync( FILE * fp )
{
    Uint8   *syncBuf, *ptr;
    Uint32  findFlag = 0;   /* 1, sync found, = 0, not found. */
    Int32   i, offset = 0;

    if ( ( syncBuf = (Uint8 *)malloc ( SYNC_RANGE ) ) == NULL )
    {
	printf( "ERROR in findSync: Not enough memory!\n");
	return (-1);
    }

    if ( fread ( syncBuf, 1, SYNC_RANGE, fp ) != SYNC_RANGE )
    {
	printf ( "ERROR: file is too short to sync %d tpHdr\n", NUM_SYNC );
	return (-1);
    }

    ptr = syncBuf;

    do {
	while ( *ptr++ != TP_SYNC_BYTE ) {
	    offset++;

	    if ( ( ptr - syncBuf ) > TP_SIZE )
	    {
		printf ("Error: sync byte not found \n");
		free(syncBuf);  /* error: for no sync byte */
		return(-1);
	    }
	}

	ptr--;  /* point to the sync byte */
	offset = ptr - syncBuf; /* the first potential sync byte */

	for ( i = 1; i < NUM_SYNC; i++ ) {
	    if ( *(ptr + TP_SIZE * i) != TP_SYNC_BYTE ) {
		findFlag = 0;

		ptr++;
		offset++;
		break;
	    }
	}

	if ( i == NUM_SYNC ){
	    findFlag = 1;
	}
    } while ( !findFlag );

    /*  rewind to where the sync byte was found */
    if ( fseek ( fp, offset, 0 ) != 0 )
    {
	printf ( "Error: Could not seek on stream \n");
    }

    free ( (Uint8 *) syncBuf );

    myPrintf (VERBOSE, "find transport tpHdr at byte %d\n", offset );
    return( offset );
}
