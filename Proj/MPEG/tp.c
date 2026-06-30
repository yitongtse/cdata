#include <stdio.h>
#include <stdlib.h>
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


/* Seek for transport packet (with FILE)
   Return 0 if successful.  Return EOF if end of file reached.
*/
int findTp(FILE *fp, int tpSize, int syncCnt)
{
    int i;

    while (1) {
        // Find the first SYNC candidate
        while (fgetc(fp) != TP_SYNC) {
            if (feof(fp))  return EOF;
        }

        for (i=1; i<syncCnt; i++) {
            fseek(fp, tpSize-1, SEEK_CUR);
            if (fgetc(fp) != TP_SYNC)  break;
        }
        fseek(fp, -1, SEEK_CUR);
        return 0;
    }
}


/* Seek for transport packet (with Reader)
   Returns after (syncCount) count of consecutive transport packets with
   correct SYNC bytes have been read.
   On return, the input port pointer will point right AFTER the last SYNC
   byte detected.
   Returns EOF if end of stream is encountered.  Otherwise returns 0.
*/
int seekTp(TpInfo *tp, Reader *in, int tpSize, int syncCount)
{
    int i;
    byteAlignReader(in);

    do {
        /* Look for first SYNC */
        while (getByte(in,"sync search")!=TP_SYNC && !in->errFlag);
        if (in->errFlag)  return EOF;

        for (i=1; i<syncCount; i++) {
            skipBytes(in, tpSize-1, "transport packet");
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


int skipTp(TpInfo *tp, Reader *in, int tpSize)
{
    skipBytes(in, tpSize-tp->nbytes, "transport packet payload");
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
            tp->PCR[1] = getBits(in, 1, "PCR[32]") * 300;
            skipBits(in, 6, "reserved");
            tp->PCR[1] += getBits(in, 9, "PCR[33-41]");
            tp->nbytes += 6;
        }

        if (tp->OPCR_flag) {
            tp->OPCR[0] = getBits(in, 32, "OPCR[0-31]");
            tp->OPCR[1] = getBits(in, 1, "OPCR[32]") * 300;
            skipBits(in, 6, "reserved");
            tp->OPCR[1] += getBits(in, 9, "OPCR[33-41]");
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

	/* Rui_B */
	return(0);
	/* Rui_E */

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
            exit(-23);
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
