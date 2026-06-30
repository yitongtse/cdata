h14032
s 00002/00002/00031
d D 2.2 02/02/25 17:53:45 ytse 3 2
c Update
e
s 00000/00000/00033
d D 2.1 00/08/21 11:04:24 ytse 2 1
c Before supporting Windows
e
s 00033/00000/00000
d D 1.1 99/10/29 15:58:14 yitong 1 0
c date and time created 99/10/29 15:58:14 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef PES_H
#define PES_H


/* MPEG-2 PES packet info
*/
typedef struct
{
    int stream_id;
    int PES_packet_length;
    int PES_scrambling_control;
    int PES_priority;
    int data_alignment_indicator;
    int copyright;
    int original_or_copy;
    int PTS_DTS_flags;
    int ESCR_flag;
    int ES_rate_flag;
    int DSM_trick_mode_flag;
    int additional_copy_info_flag;
    int PES_CRC_flag;
    int PES_extension_flag;
    int PES_header_data_length;
D 3
    uint PTS[2];		// [0]: bits 0-31, [1]: bit 32, right aligned
    uint DTS[2];		// [0]: bits 0-31, [1]: bit 32, right aligned
E 3
I 3
    uint PTS[2];		// [0]: bits 32-1, [1]: bit 0, right aligned
    uint DTS[2];		// [0]: bits 32-1, [1]: bit 0, right aligned
E 3
}
PesInfo;


int getPesHdr(PesInfo* pes, Reader* in);


#endif
E 1
