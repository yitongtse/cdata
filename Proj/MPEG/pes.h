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
    uint PTS[2];		// [0]: bits 32-1, [1]: bit 0, right aligned
    uint DTS[2];		// [0]: bits 32-1, [1]: bit 0, right aligned
}
PesInfo;


int getPesHdr(PesInfo* pes, Reader* in);


#endif
