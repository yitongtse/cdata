h49919
s 00008/00008/00066
d D 2.2 02/02/25 17:53:45 ytse 3 2
c Update
e
s 00000/00000/00074
d D 2.1 00/08/21 11:04:24 ytse 2 1
c Before supporting Windows
e
s 00074/00000/00000
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
#include "reader.h"
#include "writer.h"
#include "pes.h"


extern void getMarkerBit(Reader *in);


/* Read transport packet header
   Returns 0 if successful.  Otherwise return the error code.
   On return, the input port pointer will point to the first byte after
   the transport packet header.
   Note: Assumes that the input pointer is right before the stream_id
*/
int getPesHdr(PesInfo* pes, Reader* in)
{
    int optFldSz = 0;

    pes->stream_id = getByte(in, "stream_id");
    pes->PES_packet_length = getBits(in, 16, "PES_packet_length");

    /* Note: need to check stream_id here! */
    if (getBits(in, 2, "10") != 2)
        printf("Expected 10 here!\n");

    pes->PES_scrambling_control = getBits(in, 2, "PES_scrambling_control");
    pes->PES_priority = getBits(in, 1, "PES_priority");
    pes->data_alignment_indicator = getBits(in, 1, "data_alignment_indicator");
    pes->copyright = getBits(in, 1, "copyright");
    pes->original_or_copy = getBits(in, 1, "original_or_copy");
    pes->PTS_DTS_flags = getBits(in, 2, "PTS_DTS_flag");
    pes->ESCR_flag = getBits(in, 1, "ESCR_flag");
    pes->ES_rate_flag = getBits(in, 1, "ES_rate_flag");
    pes->DSM_trick_mode_flag = getBits(in, 1, "DSM_trick_mode_flag");
    pes->additional_copy_info_flag =
        getBits(in, 1, "additional_copy_info_flag");
    pes->PES_CRC_flag = getBits(in, 1, "PES_CRC_flag");
    pes->PES_extension_flag = getBits(in, 1, "PES_extension_flag");
    pes->PES_header_data_length = getByte(in, "PES_header_data_length");

    /* Read optional fields */
    if (pes->PTS_DTS_flags & 2) {
        /* Get PTS */
        getBits(in, 4, "0011");
D 3
        pes->PTS[1] = getBits(in, 1, "PTS[32]");
        pes->PTS[0] = getBits(in, 2, "PTS[31-30]") << 30;
E 3
I 3
        pes->PTS[0] = getBits(in, 3, "PTS[32-30]") << 29;
E 3
        getMarkerBit(in);
D 3
        pes->PTS[0] |= getBits(in, 15, "PTS[29-15]") << 15;
E 3
I 3
        pes->PTS[0] |= getBits(in, 15, "PTS[29-15]") << 14;
E 3
        getMarkerBit(in);
D 3
        pes->PTS[0] |= getBits(in, 15, "PTS[14-0]");
E 3
I 3
        pes->PTS[0] |= getBits(in, 14, "PTS[14-1]");
        pes->PTS[1] = getBits(in, 1, "PTS[0]");
E 3
        getMarkerBit(in);
        optFldSz += 5;

        /* Get DTS */
        if (pes->PTS_DTS_flags & 1) {
            getBits(in, 4, "0011");
D 3
            pes->DTS[1] = getBits(in, 1, "DTS[32]");
            pes->DTS[0] = getBits(in, 2, "DTS[31-30]") << 30;
E 3
I 3
            pes->DTS[0] = getBits(in, 3, "DTS[32-30]") << 29;
E 3
            getMarkerBit(in);
D 3
            pes->DTS[0] |= getBits(in, 15, "DTS[29-15]") << 15;
E 3
I 3
            pes->DTS[0] |= getBits(in, 15, "DTS[29-15]") << 14;
E 3
            getMarkerBit(in);
D 3
            pes->DTS[0] |= getBits(in, 15, "DTS[14-0]");
E 3
I 3
            pes->DTS[0] |= getBits(in, 14, "DTS[14-1]");
            pes->DTS[1] = getBits(in, 1, "DTS[0]");
E 3
            getMarkerBit(in);
            optFldSz += 5;
        }
    }

    /* Skip the rest of the optional fields */
    skipBytes(in, pes->PES_header_data_length-optFldSz,
              "other optional fields");
    return in->errFlag;
}

E 1
