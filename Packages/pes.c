/* @(#)pes.c	1.1  07/15/98  @(#) */
/*
 *  ======== pes ========
 *
 *  parse pes header and save the PES information
 */
#include "mydefs.h"
#include "pes.h"

#define MAX_PES_INFO_QUE_LENGTH      64

/*
 *  starting with the PES header item definition
 *
 *  bit field has to aligned on the 32-bit word boundary
 *  this is why PesHdrSize is boundled with PTS and DTS
 */

typedef struct timeStampAndPesHdrSize
{
    /************    WORD 3  *****************/
    Uint32  PES_header_data_size : 8;
    Uint32  filler1              : 4;
    Uint32  PTS_32_30            : 3;
    Uint32  marker1              : 1;
    Uint32  PTS_29_15            : 15;
    Uint32  marker2              : 1;

    /************    WORD 4  *****************/
    Uint32  PTS_14_0             : 15;
    Uint32  marker3              : 1;
    Uint32  filler2              : 4;
    Uint32  DTS_32_30            : 3;
    Uint32  marker4              : 1;
    Uint32  DTS_29_22            : 8;

    /************    WORD 5  *****************/
    Uint32  DTS_21_15            : 7;
    Uint32  marker5              : 1;
    Uint32  DTS_14_0             : 15;
    Uint32  marker6              : 1;
    Uint32  filler3              : 8;
} PesHdrSizeAndTimeStamps;

#if 0
typedef struct escr
{
    Uint32 reserve         : 2;
    Uint32 ESCR_base_32_30 : 3;
    Uint32 marker1         : 1;
    Uint32 ESCR_base_29_20 : 10;
    Uint32 ESCR_base_19_15 : 5;
    Uint32 marker2         : 1;
    Uint32 ESCR_base_14_0  : 15;
    Uint32 marker3         : 1;
    Uint32 ESCR_extension  : 9;
    Uint32 marker4         : 1;
} PesESCR;

typedef struct esRate {
    Uint32 marker1 : 1;
    Uint32 ES_rate : 22;
    Uint32 marker2 : 1;
} PesESRate;

typedef union {
    struct {
        Uint32  trick_mode_control   : 3;
        Uint32  field_id             : 2;
        Uint32  intra_slice_refresh  : 1;
        Uint32  frequency_truncation : 2;
    } fast_forward;
    struct {
        Uint32  trick_mode_control   : 3;
        Uint32  rep_cntrl            : 5;
    } slow_motion;
    struct {
        Uint32  trick_mode_control   : 3;
        Uint32  field_id             : 2;
        Uint32  reserved             : 3;
    } freeze_frame;
    struct {
        Uint32  trick_mode_control   : 3;
        Uint32  field_id             : 2;
        Uint32  intra_slice_refresh  : 1;
        Uint32  frequency_truncation : 2;
    } fast_reverse;
    struct {
        Uint32  trick_mode_control   : 3;
        Uint32  rep_cntrl            : 5;
    } slow_reverse;
    struct {
        Uint32  trick_mode_control   : 3;
        Uint32  reserved             : 5;
    } reserved;
} PesTrickModeControl;

typedef struct {
    Uint32  marker                   : 1;
    Uint32  additional_copy_info     : 7;
} PesAdditionalCopyInfo;


/*
 * PES header extension field, not completed right now
 */
typedef struct pes_extension {
    Uint32    PES_private_data_flag                : 1;
    Uint32    pack_header_field_flag               : 1;
    Uint32    program_packet_sequence_counter_flag : 1;
    Uint32    PSTD_bufer_flag                      : 1;
    Uint32    reserved                             : 3;
    Uint32    PES_extension_flag_2                 : 1;
} PesExtHdr;

typedef struct {
    Uint32     marker_bit1                         : 1;
    Uint32     program_packet_sequence_counter     : 7;
    Uint32     marker_bit2                         : 1;
    Uint32     MPEG1_MPEG2_identifier              : 1;
    Uint32     orginal_stuff_length                : 6;
} PesExtProgramPacketSequenceCounter;

typedef struct {
    Uint32     reserved                            : 2;
    Uint32     PSTD_buffer_scale                   : 1;
    Uint32     PSTD_buffer_size                    : 13;
} PesExtPSTDBuffer;

typedef struct {
    Uint8       pack_field_length;
    Uint8       pack_header[36];        /* incomplete */
} PesPackHeader;

typedef struct {
    Uint32      marker_bit                         : 1;
    Uint32      PES_extension_field_length         : 7;
    Uint32      reserved[128];     /** straddles word boundary !!! **/
} PesExtension2;

typedef struct {
    PesExtHdr           extHeader;
    Uint8               PES_private_data[7];
    PesPackHeader       packHdr;
    PesExtProgramPacketSequenceCounter  ppSeqCnter;
    PesExtPSTDBuffer    pstdBufCtrl;
    PesExtension2       pesExt2;
} PesExtension;
#endif

/*
 *  ======== pes header definition ========
 *
 *  defintion of the whole PES header when streamId is at normal audio and video case.
 */
typedef struct pesHeader {
    /************    WORD 1  *****************/
    Uint32      packet_start_code_prefix  : 24;
    Uint32      streamId                  : 8;
    /************    WORD 2  *****************/
    Uint32      PES_packet_length         : 16;
    Uint32      reserve                   : 2;
    Uint32      PES_scrambling_control    : 2;
    Uint32      PES_priority              : 1;
    Uint32      data_alignment_indicator  : 1;
    Uint32      copyright                 : 1;
    Uint32      original_or_copy          : 1;
    Uint32      PTS_DTS_flags             : 2;
    Uint32      ESCR_flag                 : 1;
    Uint32      ES_rate_flag              : 1;
    Uint32      DSM_trick_mode_flag       : 1;
    Uint32      additional_copy_info_flag : 1;
    Uint32      PES_CRC_flag              : 1;
    Uint32      PES_extension_flag        : 1;

    /********** 32 bit word boundary  ********/
    PesHdrSizeAndTimeStamps hdrSize_PTS_DTS;

} PesPacketHeader;

/*
 *  ======== pesGetDTS ========
 *
 *  parse pes header, pesBuf is a pointer points to a pes buffer.
 *  and this function returns the 32MSBs of DTS. Therefore, there is a
 *  round-off error in the resulting DTS.
 */
Uint32  pesGetDTS ( Uint8 * pesBuf )
{
    PesPacketHeader * pesHdr = (PesPacketHeader *)pesBuf;
    PesHdrSizeAndTimeStamps dts;
    Uint32  result;

    /* set all the PES_packet_length = 0 */
    pesHdr->PES_packet_length = 0;

    switch ( pesHdr->PTS_DTS_flags )
    {
        case 2:
            /* PTS and DTS are the same */
            dts = pesHdr->hdrSize_PTS_DTS;

            result = ( ( dts.PTS_32_30 << 29 ) |
                       ( dts.PTS_29_15 << 14 ) |
                       ( dts.PTS_14_0 >> 1   ) );
            break;

        case 3:
            dts = pesHdr->hdrSize_PTS_DTS;

            result = ( ( dts.DTS_32_30 << 29 ) |
                       ( dts.DTS_29_22 << 21 ) |
                       ( dts.DTS_21_15 << 14 ) |
                       ( dts.DTS_14_0 >> 1   ) );
            break;

        default:
            result = 0;
            break;
    }

    return ( result );
}
