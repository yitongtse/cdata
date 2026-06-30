h31872
s 00004/00003/00173
d D 2.2 02/02/28 13:41:56 ytse 3 2
c update
e
s 00000/00000/00176
d D 2.1 00/08/21 11:04:24 ytse 2 1
c Before supporting Windows
e
s 00176/00000/00000
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
#include "util.h"
#include "bitbuf.h"
#include "writer.h"
#include "vlc.h"
#include "mpeg2.h"
#include "info.h"
#include "puthdr.h"


void putStartCode(Writer *out, int SC)
{
    byteAlignWriter(out);
    putBits(out, 24, 1, "start code prefix");
    putByte(out, SC, "start code");
}

void putQuantMatrix(Writer *out, short *quantMatrix)
{
    int i;
    int echoFlag = out->echoFlag;
I 3
    short *scanTab = zigzagScanTab;
E 3
    setWriterEcho(out, 0, NULL);
    for (i=0; i<64; i++)
D 3
        putBits(out, 8, *quantMatrix++, "quant matrix");
E 3
I 3
        putBits(out, 8, quantMatrix[*scanTab++], "quant matrix");
E 3
    setWriterEcho(out, echoFlag, NULL);
}


void putSeqHdr(PicInfo *pic, Writer *out)
{
    commentWriter(out, "\n\nSequence header:");
    putStartCode(out, SEQ_START_CODE);
    putBits(out, 12, pic->horizontal_size & 0x0FFF, "horizontal_size_value");
    putBits(out, 12, pic->vertical_size & 0x0FFF, "vertical_size_value");
    putBits(out, 4, pic->aspect_ratio_information, "aspect_ratio_information");
    putBits(out, 4, pic->frame_rate_code, "frame_rate_code");
    putBits(out, 18, pic->bit_rate & 0x3FFFF, "bit_rate_value");
    putBits(out, 1, 1, "marker");
    putBits(out, 10, pic->vbv_buffer_size, "vbv_buffer_size_value");
    putBits(out, 1, pic->constrained_parameters_flag,
            "constrained_parameters_flag");
    putBits(out, 1,pic->load_intra_quantiser_matrix,
            "load_intra_quantiser_matrix");
    if (pic->load_intra_quantiser_matrix)
D 3
        putQuantMatrix(out, pic->intra_quantiser_matrix);
E 3
I 3
        putQuantMatrix(out, intraQmatCustom);
E 3
    putBits(out, 1, pic->load_non_intra_quantiser_matrix,
            "load_non_intra_quantiser_matrix");
    if (pic->load_non_intra_quantiser_matrix)
D 3
        putQuantMatrix(out, pic->non_intra_quantiser_matrix);
E 3
I 3
        putQuantMatrix(out, interQmatCustom);
E 3
}


void putSeqExt(PicInfo *pic, Writer *out)
{
    commentWriter(out, "\n\nSequence extension:");
    putStartCode(out, EXT_START_CODE);
    putBits(out, 4, SEQ_EXT, "extension_start_code_identifier");
    putBits(out, 8, pic->profile_and_level_indication,
            "profile_and_level_indication");
    putBits(out, 1, pic->progressive_sequence, "progressive_sequence");
    putBits(out, 2, pic->chroma_format, "chroma_format");
    putBits(out, 2, (pic->horizontal_size>>12)&3, "horizontal_size_extension");
    putBits(out, 2, (pic->vertical_size>>12)&3, "vertical_size_extension");
    putBits(out, 12, (pic->bit_rate>>18)&0x0FFF, "bit_rate_extension");
    putBits(out, 1, 1, "marker");
    putBits(out, 8, (pic->vbv_buffer_size>>18)&0xFF,
            "vbv_buffer_size_extension");
    putBits(out, 1, pic->low_delay, "low_delay");
    putBits(out, 2, pic->frame_rate_extension_n, "frame_rate_extension_n");
    putBits(out, 2, pic->frame_rate_extension_d, "frame_rate_extension_d");
}


void putSeqDispExt(PicInfo *pic, Writer *out)
{
    commentWriter(out, "\n\nSequence display extension:");
    putStartCode(out, EXT_START_CODE);
    putBits(out, 4, SEQ_DISP_EXT, "extension_start_code_identifier");
    putBits(out, 3, pic->video_format, "video_format");
    putBits(out, 1, pic->color_description, "color_description");
    if (pic->color_description) {
        putByte(out, pic->color_primaries, "color_primaries");
        putByte(out, pic->transfer_characteristics,
                "transfer_characteristics");
        putByte(out, pic->matrix_coefficients, "matrix_coefficients");
    }
    putBits(out, 14, pic->display_horizontal_size, "display_horizontal_size");
    putBits(out, 1, 1, "marker");
    putBits(out, 14, pic->display_vertical_size, "display_vertical_size");
}


void putSeqEndCode(Writer *out)
{
    putStartCode(out, SEQ_END_CODE);
}


void putGopHdr(PicInfo *pic, Writer *out)
{
    commentWriter(out, "\n\nGOP header:");
    putStartCode(out, GOP_START_CODE);

    /* Time code */
    putBits(out, 1, pic->drop_frame_flag, "drop_frame_flag");
    putBits(out, 5, pic->time_code_hours, "time_code_hours");
    putBits(out, 6, pic->time_code_minutes, "time_code_minutes");
    putBits(out, 1, 1, "marker");
    putBits(out, 6, pic->time_code_seconds, "time_code_seconds");
    putBits(out, 6, pic->time_code_pictures, "time_code_pictures");

    putBits(out, 1, pic->closed_gop, "closed_gop");
    putBits(out, 1, pic->broken_link, "broken_link");
}


void putPicHdr(PicInfo *pic, Writer *out)
{
    commentWriter(out, "\n\nPicture header:");
    putStartCode(out, PIC_START_CODE);
    putBits(out, 10, pic->temporal_reference, "temporal_reference");
    putBits(out, 3, pic->picture_coding_type, "picture_coding_type");
    putBits(out, 16, pic->vbv_delay, "vbv_delay");
    if (pic->picture_coding_type==2 || pic->picture_coding_type==3) {
        putBits(out, 1, pic->full_pel_forward_vector, "full_pel_forward_vector");
        putBits(out, 3, pic->mpeg2Flag? 7:pic->f_code[0][0], "forward_f_code");
    }
    if (pic->picture_coding_type==3) {
        putBits(out, 1, pic->full_pel_backward_vector, "full_pel_backward_vector");
        putBits(out, 3, pic->mpeg2Flag? 7:pic->f_code[1][0], "backward_f_code");
    }
    putBits(out, 8, 0, "extra_information_picture");	/* no extra info */
}


void putPicCodeExt(PicInfo *pic, Writer *out)
{
    commentWriter(out, "\n\nPicture coding extension:");
    putStartCode(out, EXT_START_CODE);
    putBits(out, 4, PIC_CODE_EXT, "extension_start_code_identifier");
    putBits(out, 4, pic->f_code[0][0], "f_code[0][0]");
    putBits(out, 4, pic->f_code[0][1], "f_code[0][1]");
    putBits(out, 4, pic->f_code[1][0], "f_code[1][0]");
    putBits(out, 4, pic->f_code[1][1], "f_code[1][1]");
    putBits(out, 2, pic->intra_dc_precision, "intra_dc_precision");
    putBits(out, 2, pic->picture_structure, "picture_structure");
    putBits(out, 1, pic->top_field_first, "top_field_first");
    putBits(out, 1, pic->frame_pred_frame_dct, "frame_pred_frame_dct");
    putBits(out, 1, pic->concealment_motion_vectors, "concealment_motion_vectors");
    putBits(out, 1, pic->q_scale_type, "q_scale_type");
    putBits(out, 1, pic->intra_vlc_format, "intra_vlc_format");
    putBits(out, 1, pic->alternate_scan, "alternate_scan");
    putBits(out, 1, pic->repeat_first_field, "repeat_first_field");
    putBits(out, 1, pic->chroma_420_type, "chroma_420_type");
    putBits(out, 1, pic->progressive_frame, "progressive_frame");
    putBits(out, 1, pic->composite_display_flag, "composite_display_flag");
    if (pic->composite_display_flag) {
        putBits(out, 1, pic->v_axis, "v_axis");
        putBits(out, 3, pic->field_sequence, "field_sequence");
        putBits(out, 1, pic->sub_carrier, "sub_carrier");
        putBits(out, 7, pic->burst_amplitude, "burst_amplitude");
        putBits(out, 8, pic->sub_carrier_phase, "sub_carrier_phase");
    }
}


void putSlcHdr(MbInfo *mb, Writer *out)
{
    /* Note: Assuming vertical_size <= 2800 and non-scalable bitstream here */
    /* Assume start code has already been read */
    commentWriter(out, "\n\nSlice header:");
    putStartCode(out, mb->slice_vertical_position);
    putBits(out, 5, mb->quantiser_scale_code, "quantiser_scale_code");
    putBits(out, 1, 0, "extra_bit_slice");		/* no extra info */
}

E 1
