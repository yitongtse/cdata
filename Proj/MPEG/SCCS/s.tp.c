h33011
s 00010/00010/00233
d D 2.3 02/02/25 17:53:44 ytse 5 4
c Update
e
s 00007/00001/00236
d D 2.2 00/08/21 11:23:54 ytse 4 3
c Added support of Windows
e
s 00000/00000/00237
d D 2.1 00/08/21 11:04:26 ytse 3 2
c Before supporting Windows
e
s 00024/00001/00213
d D 1.2 99/11/01 16:15:35 yitong 2 1
c Backup
e
s 00214/00000/00000
d D 1.1 99/10/29 15:58:11 yitong 1 0
c date and time created 99/10/29 15:58:11 by yitong
e
u
U
f e 0
t
T
I 1
#include <stdio.h>
I 4
#include <stdlib.h>
E 4
#include <string.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"
#include "writer.h"
#include "tp.h"


void initTp(TpInfo *tp)
{
    memset(tp, 0, sizeof(TpInfo));
}


D 2
/* Seek for transport packet
E 2
I 2
/* Seek for transport packet (with FILE)
   Return 0 if successful.  Return EOF if end of file reached.
*/
D 5
int findTp(FILE *fp, int syncCnt)
E 5
I 5
int findTp(FILE *fp, int tpSize, int syncCnt)
E 5
{
    int i;

    while (1) {
        // Find the first SYNC candidate
        while (fgetc(fp) != TP_SYNC) {
            if (feof(fp))  return EOF;
        }

        for (i=1; i<syncCnt; i++) {
D 5
            fseek(fp, TP_SIZE-1, SEEK_CUR);
E 5
I 5
            fseek(fp, tpSize-1, SEEK_CUR);
E 5
            if (fgetc(fp) != TP_SYNC)  break;
        }
        fseek(fp, -1, SEEK_CUR);
        return 0;
    }
}


/* Seek for transport packet (with Reader)
E 2
   Returns after (syncCount) count of consecutive transport packets with
   correct SYNC bytes have been read.
   On return, the input port pointer will point right AFTER the last SYNC
   byte detected.
   Returns EOF if end of stream is encountered.  Otherwise returns 0.
*/
D 5
int seekTp(TpInfo *tp, Reader *in, int syncCount)
E 5
I 5
int seekTp(TpInfo *tp, Reader *in, int tpSize, int syncCount)
E 5
{
    int i;
    byteAlignReader(in);

    do {
        /* Look for first SYNC */
        while (getByte(in,"sync search")!=TP_SYNC && !in->errFlag);
        if (in->errFlag)  return EOF;

        for (i=1; i<syncCount; i++) {
D 5
            skipBytes(in, TP_SIZE-1, "transport packet");
E 5
I 5
            skipBytes(in, tpSize-1, "transport packet");
E 5
            if (getByte(in, "sync_search") != TP_SYNC)  break;
        }
    } while (i<syncCount);

    return 0;
}


/* Read transport packet header
   Returns 0 if successful.  Otherwise return the error code.
   On return, the input port pointer will point to the first byte after
   the transport packet header.
   Note: should be called just after the SYNC byte is read.
*/
int getTpHdr(TpInfo *tp, Reader *in)
{
    tp->transport_error_indicator = getBits(in, 1, "transport_error_indicator");
    tp->payload_unit_start_indicator = getBits(in, 1,
        "payload_unit_start_indicator");
    tp->transport_priority = getBits(in, 1, "transport_priority");
    tp->PID = getBits(in, 13, "PID");
    tp->transport_scrambling_control = getBits(in, 2,
        "transport_scrambling_control");
    tp->adaptation_field_control = getBits(in, 2, "adaptation_field_control");
    tp->continuity_counter = getBits(in, 4, "continuity_counter");
    tp->nbytes = 4;
    return in->errFlag;
}


D 5
int skipTp(TpInfo *tp, Reader *in)
E 5
I 5
int skipTp(TpInfo *tp, Reader *in, int tpSize)
E 5
{
D 5
    skipBytes(in, TP_SIZE-tp->nbytes, "transport packet payload");
E 5
I 5
    skipBytes(in, tpSize-tp->nbytes, "transport packet payload");
E 5
    return in->errFlag;
}


/* Read adaptation field
   The caller should make sure that the adaptation field exists
   in the current transport packet.
*/
int getTpAf(TpInfo *tp, Reader *in)
{
    tp->adaptation_field_length = getByte(in, "adaptation_field_length");
    tp->nbytes++;

    if (tp->adaptation_field_length>0) {
        tp->discontinuity_indicator = getBits(in, 1, "discontinuity_indicator");
        tp->random_access_indicator = getBits(in, 1, "random_access_indicator");
        tp->elementary_stream_priority_indicator = getBits(in, 1,
            "elementary_stream_priority_indicator");
        tp->PCR_flag = getBits(in, 1, "PCR_flag");
        tp->OPCR_flag = getBits(in, 1, "OPCR_flag");
        tp->splicing_point_flag = getBits(in, 1, "splicing_point_flag");
        tp->transport_private_data_flag = getBits(in, 1,
            "transport_private_data_flag");
        tp->adaptation_field_extension_flag = getBits(in, 1,
            "adaptation_field_extension_flag");
        tp->nbytes++;

        if (tp->PCR_flag) {
            tp->PCR[0] = getBits(in, 32, "PCR[0-31]");
D 5
            tp->PCR[1] = getBits(in, 1, "PCR[32]") << 9;
E 5
I 5
            tp->PCR[1] = getBits(in, 1, "PCR[32]") * 300;
E 5
            skipBits(in, 6, "reserved");
D 5
            tp->PCR[1] |= getBits(in, 9, "PCR[33-41]");
E 5
I 5
            tp->PCR[1] += getBits(in, 9, "PCR[33-41]");
E 5
            tp->nbytes += 6;
        }

        if (tp->OPCR_flag) {
            tp->OPCR[0] = getBits(in, 32, "OPCR[0-31]");
D 5
            tp->OPCR[1] = getBits(in, 1, "OPCR[32]") << 9;
E 5
I 5
            tp->OPCR[1] = getBits(in, 1, "OPCR[32]") * 300;
E 5
            skipBits(in, 6, "reserved");
D 5
            tp->OPCR[1] |= getBits(in, 9, "OPCR[33-41]");
E 5
I 5
            tp->OPCR[1] += getBits(in, 9, "OPCR[33-41]");
E 5
            tp->nbytes += 6;
        }

        if (tp->splicing_point_flag) {
            tp->splice_countdown = getByte(in, "splice_countdown");
            tp->nbytes++;
        }

        /* Skip the rest of the adaptation field */
        skipBytes(in, tp->adaptation_field_length+5-tp->nbytes, "stuffing");
        tp->nbytes = 5 + tp->adaptation_field_length;
    }
I 4

	/* Rui_B */
	return(0);
	/* Rui_E */

E 4
}


/* Skip adaptation field
   The caller should make sure that the adaptation field exists
   in the current transport packet.
*/
int skipTpAf(TpInfo *tp, Reader *in)
{
    tp->adaptation_field_length = getByte(in, "adaptation_field_length");
    skipBytes(in, tp->adaptation_field_length, "adaptation field");
    tp->nbytes += tp->adaptation_field_length + 1;
    return in->errFlag;
}


/* Write transport packet header
*/
void putTpHdr(TpInfo *tp, Writer *out)
{
    commentWriter(out, "\n\nTP header:");
    putByte(out, TP_SYNC, "sync_byte");
    putBits(out, 1, tp->transport_error_indicator, "transport_error_indicator");
    putBits(out, 1, tp->payload_unit_start_indicator,
        "payload_unit_start_indicator");
    putBits(out, 1, tp->transport_priority, "transport_priority");
    putBits(out, 13, tp->PID, "PID");
    putBits(out, 2, tp->transport_scrambling_control,
        "transport_scrambling_control");
    putBits(out, 2, tp->adaptation_field_control, "adaptation_field_control");
    putBits(out, 4, tp->continuity_counter, "continuity_counter");
    tp->nbytes = 4;
}


/* Write adaptation field
   Note: to be completed later!
*/
void putTpAf(TpInfo *tp, Writer *out)
{
    int i;

    commentWriter(out, "\nAdaptation field:");
    putByte(out, tp->adaptation_field_length, "adaptation_field_length");

    /* Pre-compute adaptation_field_length */
    /* Note: User should set adaptation_field_length before calling.
             Adaptation_field_length should be large enough to include
             all the optional fields selected.
    */
    if (tp->adaptation_field_length) {
        tp->stuffSz = tp->adaptation_field_length - 1;
        if (tp->PCR_flag)  tp->stuffSz -= 6;
        if (tp->OPCR_flag)  tp->stuffSz -= 6;
        if (tp->splicing_point_flag)  tp->stuffSz--;
        if (tp->stuffSz<0) {
            printf("Error: adaptation_field_length less than "\
	           "optional field size\n");
D 4
            exit(-1);
E 4
I 4
            exit(-23);
E 4
        }

        putBits(out, 1, tp->discontinuity_indicator, "discontinuity_indicator");
        putBits(out, 1, tp->random_access_indicator, "random_access_indicator");
        putBits(out, 1, tp->elementary_stream_priority_indicator,
            "elementary_stream_priority_indicator");
        putBits(out, 1, tp->PCR_flag, "PCR_flag");
        putBits(out, 1, tp->OPCR_flag, "OPCR_flag");
        putBits(out, 1, tp->splicing_point_flag, "splicing_point_flag");
        putBits(out, 1, tp->transport_private_data_flag,
            "transport_private_data_flag");
        putBits(out, 1, tp->adaptation_field_extension_flag,
            "adaptation_field_extension_flag");

        if (tp->PCR_flag) {
            putBits(out, 32, tp->PCR[0], "PCR[0-31]");
            putBits(out, 1, tp->PCR[1]>>9, "PCR[32]");
            putBits(out, 6, 0, "reserved");
            putBits(out, 9, tp->PCR[1] & 0x1FF, "PCR[33-41]");
        }

        if (tp->OPCR_flag) {
            putBits(out, 32, tp->OPCR[0], "OPCR[0-31]");
            putBits(out, 1, tp->OPCR[1]>>9, "OPCR[32]");
            putBits(out, 6, 0, "reserved");
            putBits(out, 9, tp->OPCR[1] & 0x1FF, "OPCR[33-41]");
        }

        if (tp->splicing_point_flag)
            putByte(out, tp->splice_countdown, "splice_countdown");

        /* Stuff bytes */
        for (i=tp->stuffSz; i>0; i--)
            putByte(out, 0xFF, NULL);
    }
}
E 1
