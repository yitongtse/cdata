h02637
s 00004/00000/00370
d D 2.7 02/03/21 13:28:41 ytse 10 9
c Backup for NEW2
e
s 00001/00000/00369
d D 2.6 02/03/04 15:38:23 ytse 9 8
c Update
e
s 00009/00017/00360
d D 2.5 02/02/28 13:41:56 ytse 8 7
c update
e
s 00014/00000/00363
d D 2.4 02/01/21 11:02:33 ytse 7 6
c Update
e
s 00001/00000/00362
d D 2.3 01/01/15 12:39:13 ytse 6 5
c update
e
s 00008/00003/00354
d D 2.2 00/08/21 11:23:55 ytse 5 4
c Added support of Windows
e
s 00000/00000/00357
d D 2.1 00/08/21 11:04:20 ytse 4 3
c Before supporting Windows
e
s 00004/00005/00353
d D 1.3 99/11/17 18:18:55 yitong 3 2
c Fixed a bug in parsing intra_slice syntax
e
s 00003/00003/00355
d D 1.2 99/11/03 15:39:10 yitong 2 1
c Minor changes
e
s 00358/00000/00000
d D 1.1 99/10/29 15:58:08 yitong 1 0
c date and time created 99/10/29 15:58:08 by yitong
e
u
U
f e 0
t
T
I 1
#include <stdio.h>
I 5
/* Rui_B */
#include <stdlib.h>
/* Rui_E */
E 5
#include "util.h"
#include "bitbuf.h"
#include "reader.h"
#include "vld.h"
#include "mpeg2.h"
#include "info.h"
#include "gethdr.h"
I 9
#include "block.h"
E 9


/* Seek for MPEG-2 start code (0x000001)
   Return 0 if successful.  Otherwise returns in->errFlag
   When successful, the input port pointer will point right before
   the last byte of the first found start code.
*/
int seekStartCode(Reader *in)
{
    int temp;
I 5

E 5
    byteAlignReader(in);
    temp = (getByte(in, "") << 16) | (getByte(in, "") << 8) | getByte(in, "");
D 5
    while (temp!=1 && !in->errFlag) {
E 5
I 5
    while (temp!=1 && !in->errFlag) 
	{
E 5
        temp = ((temp<<8) | getByte(in, "")) & 0x00ffffff;
    }
    return in->errFlag;
}


int getNextStartCode(Reader *in)
{
    return seekStartCode(in)? in->errFlag : getByte(in, "start_code");
}


void getMarkerBit(Reader *in)
{
    if (!getBits(in, 1, "marker_bit"))
        printf("Unexpected marker_bit value 0\n");
}


void getQuantMatrix(Reader *in, short *quantMatrix)
{
    int i;
    int echoFlag = in->echoFlag;
    short *scanTab = zigzagScanTab;
    setReaderEcho(in, 0, NULL);
    for (i=0; i<64; i++)
        quantMatrix[*scanTab++] = getBits(in, 8, "quant matrix");
    setReaderEcho(in, echoFlag, NULL);
I 7
D 8

#if 1
  {
    int j;
    short* ptr = quantMatrix;
    printf("\n\nQMAT:");
    for (i=0; i<8; i++) {
        printf("\n  ");
        for (j=0; j<8; j++)
            printf("%4d", *ptr++);
    }
    printf("\n");
  }
#endif
E 8
E 7
}


/* Return 0 if successful.  Otherwise returns in->errFlag
*/
int seekSeqHdr(Reader *in)
{
    int SC;
    do {
        SC = getNextStartCode(in);
    } while (SC!=SEQ_START_CODE);
    return in->errFlag;
}


void getSeqHdr(PicInfo *pic, Reader *in)
{
    commentReader(in, "\n\nSequence header:");
    pic->horizontal_size = getBits(in, 12, "horizontal_size_value");
    pic->vertical_size = getBits(in, 12, "vertical_size_value");
    pic->aspect_ratio_information = getBits(in, 4, "aspect_ratio_information");
    pic->frame_rate_code = getBits(in, 4, "frame_rate_code");
    pic->bit_rate = getBits(in, 18, "bit_rate_value");
    getMarkerBit(in);
    pic->vbv_buffer_size = getBits(in, 10, "vbv_buffer_size_value");
    pic->constrained_parameters_flag = getBits(in, 1,
        "constrained_parameters_flag");
    pic->load_intra_quantiser_matrix = getBits(in, 1,
        "load_intra_quantiser_matrix");
D 8
    if (pic->load_intra_quantiser_matrix)
E 8
I 8
    if (pic->load_intra_quantiser_matrix) {
E 8
        getQuantMatrix(in, intraQmatCustom);
I 10
#if 0
E 10
I 8
        printf("\n\nIntra QMAT:\n");
        printShortBlock(intraQmatCustom, 8, 8, 8);
I 10
#endif
E 10
    }
E 8
    pic->load_non_intra_quantiser_matrix = getBits(in, 1,
        "load_non_intra_quantiser_matrix");
D 8
    if (pic->load_non_intra_quantiser_matrix)
E 8
I 8
    if (pic->load_non_intra_quantiser_matrix) {
E 8
        getQuantMatrix(in, interQmatCustom);
I 10
#if 0
E 10
I 8
        printf("\n\nInter QMAT:\n");
        printShortBlock(interQmatCustom, 8, 8, 8);
I 10
#endif
E 10
    }
E 8

    pic->firstExtRead = 0;
}


/* Note: Assumes extension_start_code already read
*/
void getSeqExtension(PicInfo *pic, Reader *in)
{
    int extId;
    extId = getBits(in, 4, "extension_start_code_identifier");
    switch (extId) {
        case SEQ_EXT: getSeqExt(pic, in);  break;
        case SEQ_DISP_EXT: getSeqDispExt(pic, in);  break;
    }
}


void getSeqExt(PicInfo *pic, Reader *in)
{
    commentReader(in, "\n\nSequence extension:");
    pic->profile_and_level_indication = getBits(in, 8,
        "profile_and_level_indication");
    pic->progressive_sequence = getBits(in, 1, "progressive_sequence");
    pic->chroma_format = getBits(in, 2, "chroma_format");
    pic->horizontal_size |= getBits(in, 2, "horizontal_size_extension") << 12;
    pic->vertical_size |= getBits(in, 2, "vertical_size_extension") << 12;
    pic->bit_rate |= getBits(in, 12, "bit_rate_extension") << 18;
    getMarkerBit(in);
    pic->vbv_buffer_size |= getBits(in, 8, "vbv_buffer_size_extension") << 18;
    pic->low_delay = getBits(in, 1, "low_delay");
    pic->frame_rate_extension_n = getBits(in, 2, "frame_rate_extension_n");
    pic->frame_rate_extension_d = getBits(in, 2, "frame_rate_extension_d");

    pic->matrix_coefficients = 1;	/* set to default */
}


void getSeqDispExt(PicInfo *pic, Reader *in)
{
    commentReader(in, "\n\nSequence display extension:");
I 6
D 8
printf("Sequence display extension\n");
E 8
I 8
    printf("Sequence display extension\n");
E 8
E 6
    pic->video_format = getBits(in, 3, "video_format");
    pic->color_description = getBits(in, 1, "color_description");
    if (pic->color_description) {
        pic->color_primaries = getByte(in, "color_primaries");
        pic->transfer_characteristics = getByte(in, "transfer_characteristics");
        pic->matrix_coefficients = getByte(in, "matrix_coefficients");
    }
    pic->display_horizontal_size = getBits(in, 14, "display_horizontal_size");
    getMarkerBit(in);
    pic->display_vertical_size = getBits(in, 14, "display_vertical_size");
}


void getGopHdr(PicInfo *pic, Reader *in)
{
    commentReader(in, "\n\nGOP header:");
    /* Get time code */
    pic->drop_frame_flag = getBits(in, 1, "drop_frame_flag");
    pic->time_code_hours = getBits(in, 5, "time_code_hours");
    pic->time_code_minutes = getBits(in, 6, "time_code_minutes");
    getMarkerBit(in);
    pic->time_code_seconds = getBits(in, 6, "time_code_seconds");
    pic->time_code_pictures = getBits(in, 6, "time_code_pictures");

    pic->closed_gop = getBits(in, 1, "closed_gop");
    pic->broken_link = getBits(in, 1, "broken_link");
}


void getPicHdr(PicInfo *pic, Reader *in)
{
    commentReader(in, "\n\nPicture header:");
    pic->temporal_reference = getBits(in, 10, "temporal_reference");
    pic->picture_coding_type = getBits(in, 3, "picture_coding_type");
    pic->vbv_delay = getBits(in, 16, "vbv_delay");
D 2
    if (pic->picture_coding_type==2 || pic->picture_coding_type==3) {
E 2
I 2
    if (pic->picture_coding_type==P_PIC || pic->picture_coding_type==B_PIC) {
E 2
        pic->full_pel_forward_vector = getBits(in, 1,
                                               "full_pel_forward_vector");
        pic->f_code[0][0] = getBits(in, 3, "forward_f_code");
    }
    if (pic->picture_coding_type==3) {
        pic->full_pel_backward_vector = getBits(in, 1,
                                                "full_pel_backward_vector");
        pic->f_code[1][0] = getBits(in, 3, "backward_f_code");
    }
    while (getBits(in, 1, "extra_bit_picture"))
        getBits(in, 8, "extra_information_picture");

    /* Syntax checking for MPEG-2 */
    if (pic->mpeg2Flag) {
D 2
        if (pic->picture_coding_type==2 || pic->picture_coding_type==3) {
E 2
I 2
        if (pic->picture_coding_type==P_PIC || pic->picture_coding_type==B_PIC){
E 2
            if (pic->full_pel_forward_vector)
                printf("Syntax error: full_pel_forward_vector should be 0.\n");
            if (pic->f_code[0][0]!=7)
                printf("Syntax error: forward_f_code should be 7.\n");
        }
D 2
        if (pic->picture_coding_type==3) {
E 2
I 2
        if (pic->picture_coding_type==B_PIC) {
E 2
            if (pic->full_pel_backward_vector)
                printf("Syntax error: full_pel_backward_vector should be 0.\n");
            if (pic->f_code[1][0]!=7)
                printf("Syntax error: backward_f_code should be 7.\n");
        }
    }
    else {
        pic->f_code[0][1] = pic->f_code[0][0];
        pic->f_code[1][1] = pic->f_code[1][0];
    }
}


void getPicCodeExt(PicInfo *pic, Reader *in)
{
    commentReader(in, "\n\nPicture coding extension:");
    pic->f_code[0][0] = getBits(in, 4, "f_code[0][0]");
    pic->f_code[0][1] = getBits(in, 4, "f_code[0][1]");
    pic->f_code[1][0] = getBits(in, 4, "f_code[1][0]");
    pic->f_code[1][1] = getBits(in, 4, "f_code[1][1]");
    pic->intra_dc_precision = getBits(in, 2, "intra_dc_precision");
    pic->picture_structure = getBits(in, 2, "picture_structure");
    pic->top_field_first = getBits(in, 1, "top_field_first");
    pic->frame_pred_frame_dct = getBits(in, 1, "frame_pred_frame_dct");
    pic->concealment_motion_vectors = getBits(in, 1,
                                              "concealment_motion_vectors");
    pic->q_scale_type = getBits(in, 1, "q_scale_type");
    pic->intra_vlc_format = getBits(in, 1, "intra_vlc_format");
    pic->alternate_scan = getBits(in, 1, "alternate_scan");
    pic->repeat_first_field = getBits(in, 1, "repeat_first_field");
    pic->chroma_420_type = getBits(in, 1, "chroma_420_type");
    pic->progressive_frame = getBits(in, 1, "progressive_frame");
    pic->composite_display_flag = getBits(in, 1, "composite_display_flag");
    if (pic->composite_display_flag) {
        pic->v_axis = getBits(in, 1, "v_axis");
        pic->field_sequence = getBits(in, 3, "field_sequence");
        pic->sub_carrier = getBits(in, 1, "sub_carrier");
        pic->burst_amplitude = getBits(in, 7, "burst_amplitude");
        pic->sub_carrier_phase = getBits(in, 8, "sub_carrier_phase");
    }
}


void getPicExtension(PicInfo *pic, Reader *in)
{
}


int getUserData(Reader *in)
{
    int SC;
    int echoFlag = in->echoFlag;
    setReaderEcho(in, 0, NULL);
    SC = getNextStartCode(in);
    setReaderEcho(in, echoFlag, NULL);
    return SC;
}


/* SC is the start code that has already been read before the call */
void getSlcHdr(MbInfo *mb, Reader *in, int SC)
{
    /* Check whether the SC is within valid slice SC range */
    if (SC<MIN_SLICE_START_CODE || SC>MAX_SLICE_START_CODE) {
        printf("Invalid slice start code 0x000001%02x found!\n", SC);
D 5
        exit(-1);
E 5
I 5
        exit(-1078);
E 5
    }

    /* Note: Assuming vertical_size <= 2800 and non-scalable bitstream here */
    mb->slice_vertical_position = SC;
    commentReader(in, "\n\nSlice header:");
    mb->quantiser_scale_code = getBits(in, 5, "quantiser_scale_code");
D 3
    if (getBits(in, 1, "extra_bit_slice")) {
        if (getBits(in, 1, "intra_slice_flag")) {
            mb->intra_slice = getBits(in, 1, "intra_slice");
            getBits(in, 7, "reserved_bits");
        }
E 3
I 3
    if (getBits(in, 1, "intra_slice_flag")) {
        mb->intra_slice = getBits(in, 1, "intra_slice");
        getBits(in, 7, "reserved_bits");

E 3
        while (getBits(in, 1, "extra_bit_slice"))
            getBits(in, 8, "extra_information_slice");
    }
}


/* Parse sequence layer syntax
   Returns 0 if successful, 1 if error.
*/
int getSeqLayer(Reader *in, PicInfo *pic, int *SC)
{
    pic->mpeg2Flag = 0;
    getSeqHdr(pic, in);

    *SC = getNextStartCode(in);
    if (*SC==EXT_START_CODE) {
        /* MPEG-2 specific processing */
        int extId = getBits(in, 4, "extension_start_code_identifier");
        if (extId==SEQ_EXT) {
            getSeqExt(pic, in);
            pic->mpeg2Flag = 1;
            *SC = getNextStartCode(in);
        }
        else {
            printf("Error: Expected sequence extension not found!\n");
            return 1;
        }

        /* Read all other extensions and user data */
        while (1) {
            if (*SC==EXT_START_CODE) {
                getSeqExtension(pic, in);
                *SC = getNextStartCode(in);
            }
            else if (*SC==USER_DATA_START_CODE) {
                *SC = getUserData(in);
            }
            else break;
        }
    }    

    initSeq(pic);
    initQuantMatrices(pic);
    return 0;
}


/* Parse GOP layer syntax
   Returns next start code read
*/
int getGopLayer(Reader *in, PicInfo *pic, int *SC)
{
    getGopHdr(pic, in);
    *SC = getNextStartCode(in);
    while (*SC==USER_DATA_START_CODE)
        *SC = getNextStartCode(in);
    initGop(pic);
    return 0;
}


/* Parse picture layer syntax
   Note: Assume the picture header has just been read!
   Returns next start code read after picture layer
*/
int getPicLayer(Reader *in, PicInfo *pic, int *SC)
{
    getPicHdr(pic, in);
    *SC = getNextStartCode(in);

    if (pic->mpeg2Flag) {
        /* MPEG-2 specific processing */
        int extId = getBits(in, 4, "extension_start_code_identifier");
        if (*SC==EXT_START_CODE && extId==PIC_CODE_EXT) {
            getPicCodeExt(pic, in);
            *SC = getNextStartCode(in);
        }
        else {
            printf("Picture coding extension expected!\n");
D 5
            exit(-1);
E 5
I 5
            exit(-1234);
E 5
        }

        /* Read all other extensions and user data */
        while (1) {
            if (*SC==EXT_START_CODE) {
                getPicExtension(pic, in);
                *SC = getNextStartCode(in);
            }
            else if (*SC==USER_DATA_START_CODE) {
                *SC = getUserData(in);
            }
            else break;
        }
    }    

    initPic(pic);
    return 0;
}

E 1
