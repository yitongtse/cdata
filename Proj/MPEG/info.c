#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "imageio.h"
#include "bitbuf.h"
#include "reader.h"
#include "mpeg2.h"
#include "info.h"
//#include "encode.h"
#include "decode.h"

/* Rui_B */
//static char *COMP[] = { "y", "u", "v" };	/* Y, Cb, Cr */

static char COMP[] = { 'y', 'u', 'v' };	/* Y, Cb, Cr */
/* Rui_E */

void initPicInfo(PicInfo *pic)
{
    pic->horizontal_size = 0;
    pic->vertical_size = 0;
    pic->aspect_ratio_information = 0;
    pic->frame_rate_code = 0;
    pic->bit_rate = 0;
    pic->vbv_buffer_size = 0;
    pic->constrained_parameters_flag = 0;
    pic->profile_and_level_indication = 0;
    pic->progressive_sequence = 0;
    pic->chroma_format = 0;
    pic->low_delay = 0;
    pic->frame_rate_extension_n = 0;
    pic->frame_rate_extension_d = 0;
    pic->video_format = 0;
    pic->color_description = 0;
    pic->color_primaries = 0;
    pic->transfer_characteristics = 0;
    pic->matrix_coefficients = 0;
    pic->display_horizontal_size = 0;
    pic->display_vertical_size = 0;
    pic->load_intra_quantiser_matrix = 0;
    pic->load_non_intra_quantiser_matrix = 0;
    pic->load_chroma_intra_quantiser_matrix = 0;
    pic->load_chroma_non_intra_quantiser_matrix = 0;
    pic->drop_frame_flag = 0;
    pic->time_code_hours = 0;
    pic->time_code_minutes = 0;
    pic->time_code_seconds = 0;
    pic->time_code_pictures = 0;
    pic->closed_gop = 0;
    pic->broken_link = 0;
    pic->temporal_reference = 0;
    pic->picture_coding_type = 0;
    pic->vbv_delay = 0;
    pic->full_pel_forward_vector = 0;
    pic->full_pel_backward_vector = 0;
    pic->intra_dc_precision = 0;
    pic->picture_structure = 0;
    pic->top_field_first = 0;
    pic->frame_pred_frame_dct = 0;
    pic->concealment_motion_vectors = 0;
    pic->q_scale_type = 0;
    pic->intra_vlc_format = 0;
    pic->alternate_scan = 0;
    pic->repeat_first_field = 0;
    pic->chroma_420_type = 0;
    pic->progressive_frame = 0;
    pic->composite_display_flag = 0;
    pic->v_axis = 0;
    pic->field_sequence = 0;
    pic->sub_carrier = 0;
    pic->burst_amplitude = 0;
    pic->sub_carrier_phase = 0;
    pic->copyright_flag = 0;
    pic->copyright_identifier = 0;
    pic->original_or_copy = 0;
    pic->copyright_number_1 = 0;
    pic->copyright_number_2 = 0;
    pic->copyright_number_3 = 0;
}


void initSeq(PicInfo *pic)
{
    pic->nCol = (pic->horizontal_size+15) & 0x3FF0;
    pic->nRow = pic->progressive_sequence? (pic->vertical_size+15) & 0x3FF0 :
                                           (pic->vertical_size+31) & 0x3FE0;
}


void initGop(PicInfo *pic)
{
    pic->trBase = pic->nextTrBase;
}


void initPic(PicInfo *pic)
{
    /* Note: need to handle wrap around later! */
    pic->frmNo = pic->trBase + pic->temporal_reference;
    if (pic->picture_coding_type!=B_PIC)  pic->nextTrBase = pic->frmNo + 1;

    pic->nMbCol = pic->nCol >> 4;
    pic->nMbRow = pic->nRow >> 4;
    pic->nMb = pic->nMbCol * pic->nMbRow;
}


void initSlc(PicInfo *pic, MbInfo *mb)
{
    mb->mbaRef = (mb->slice_vertical_position-1) * pic->nMbCol - 1;
    resetMvPred(pic, mb);
    resetDcDctPred(pic, mb);
}


void initMb(PicInfo *pic, MbInfo *mb)
{
    /* set up macroblock position based on its MBA */
    mb->pos.x = (mb->index % pic->nMbCol) << 4;
    mb->pos.y = (mb->index / pic->nMbCol) << 4;
}


void initQuantMatrices(PicInfo *pic)
{
    pic->intra_quantiser_matrix = pic->load_intra_quantiser_matrix?
                                      intraQmatCustom : intraQmatDefault;
    pic->non_intra_quantiser_matrix = pic->load_non_intra_quantiser_matrix?
                                          interQmatCustom : interQmatDefault;
}


void setMbAttrib(MbInfo* mb, int idx)
{
    mb->quant = mbAttrib[idx][0];
    mb->motion_forward = mbAttrib[idx][1];
    mb->motion_backward = mbAttrib[idx][2];
    mb->pattern = mbAttrib[idx][3];
    mb->intra = mbAttrib[idx][4];
}


// Check whether the macroblock is skipped
//
int isSkippedMb(MbInfo *mb)
{
    return (!(mb->quant | mb->motion_forward | mb->motion_backward
                      | mb->pattern | mb->intra));
}


// Set macroblock type according to the attributes 
//   Note: assume MB is not skipped!
//   Return -1 if appropriate mode not found 
// 
int setMbType(MbInfo *mb, int pic_coding_type)
{
    int i, i1, i2;

    switch (pic_coding_type) {
        case I_PIC: i1 = 1;   i2 = 2;   break;
        case P_PIC: i1 = 3;   i2 = 9;   break;
        case B_PIC: i1 = 10;  i2 = 20;  break;
        default: return -1;
    } 
    
    for (i=i1; i<=i2; i++) {
        if (mb->quant == mbAttrib[i][0] 
                && mb->motion_forward == mbAttrib[i][1]
                && mb->motion_backward == mbAttrib[i][2]
                && mb->pattern == mbAttrib[i][3]
                && mb->intra == mbAttrib[i][4]) {
            mb->type = i - i1;
            return 0;
        }
    }
    return -1;
}


/* Reset motion vector predictor */
void resetMvPred(PicInfo *pic, MbInfo *mb)
{
    if (pic->picture_coding_type!=I_PIC) {
        setCoor(&mb->pmv[0][0], 0, 0);
        setCoor(&mb->pmv[0][1], 0, 0);
    }
    if (pic->picture_coding_type==B_PIC) {
        setCoor(&mb->pmv[1][0], 0, 0);
        setCoor(&mb->pmv[1][1], 0, 0);
    }
}


/* Set default motion parameters
  (For skipped macroblocks in P- or B-pictures,
   or DPCM macroblocks in P-picture)
*/
void setDefaultMotion(PicInfo *pic, MbInfo *mb)
{
    if (pic->picture_structure==FRAME)
        mb->motion_type = FRAME_FRAME_BASED;
    else {
        mb->motion_type = FIELD_FIELD_BASED;
        mb->fldSel[0][0] = mb->fldSel[1][0] =
            (pic->picture_structure==BOTTOM_FIELD);
    }
}


/* Reset forward motion vectors (for P_DPCM) */
void resetFwdMv(MbInfo *mb)
{
    setCoor(&mb->mv[0][0], 0, 0);
    setCoor(&mb->mv[0][1], 0, 0);
}


/* Reset DC DCT predictors */
void resetDcDctPred(PicInfo *pic, MbInfo *mb)
{
    mb->dc_dct_pred[Y] = mb->dc_dct_pred[Cb] = mb->dc_dct_pred[Cr]
        = 128 << pic->intra_dc_precision;
}


int addRl(RlInfo *rl, int run, int level)
{
    if (rl->count>63)  return 1;
    rl->run[rl->count] = run;
    rl->level[rl->count++] = level;
    return 0;
}


void printSeqInfo(PicInfo *pic)
{
    printf("Resolution: %d x %d\n", pic->horizontal_size, pic->vertical_size);
    printf("Aspect ratio info: %d\n", pic->aspect_ratio_information);
    printf("Frame rate code: %d\n", pic->frame_rate_code);
    printf("Bit rate: %d\n", pic->bit_rate);
    printf("VBV buffer size: %d\n", pic->vbv_buffer_size);
}


void printGopInfo(PicInfo *pic)
{
}


void printPicInfo(PicInfo *pic)
{
}


void printSlcInfo(MbInfo *mb)
{
}


void printMbInfo(PicInfo *pic, MbInfo *mb)
{
    int temp;

    printf("MBA: %d    Pos: (%d,%d)\n", mb->index, mb->pos.x, mb->pos.y);
    switch (pic->picture_coding_type) {
      case I_PIC: temp = 1;  break;
      case P_PIC: temp = 3;  break;
      case B_PIC: temp = 10;  break;
    }
    printf("Type: %s", MB_TYPE[mb->type+temp]);
    if (mb->motion_forward || mb->motion_backward)
        printf(" \tMotion type: %s",
               PRED_TYPE[mb->motion_type+(pic->picture_structure==FRAME?0:4)]);
    if (mb->motion_forward) {
        printf("\nForward mv: (%d,%d), (%d,%d)", mb->pmv[0][0].x,
               mb->pmv[0][0].y, mb->pmv[0][1].x, mb->pmv[0][1].y);
        printf(" \tField select: %d %d", mb->fldSel[0][0], mb->fldSel[0][1]);
    }
    if (mb->motion_backward) {
        printf("\nBackward mv: (%d,%d), (%d,%d)", mb->pmv[1][0].x,
               mb->pmv[1][0].y, mb->pmv[1][1].x, mb->pmv[1][1].y);
        printf(" \tField select: %d %d", mb->fldSel[1][0], mb->fldSel[1][1]);
    }
    printf("\nQS code: %d\n", mb->quantiser_scale_code);
    printf("DCT type: %d\n", mb->dct_type);
    if (mb->pattern)  printf("CBP: 0x%3x\n", mb->cbp);
    printf("Dc predictors: %d, %d, %d\n", mb->dc_dct_pred[0],
           mb->dc_dct_pred[1], mb->dc_dct_pred[2]);
}


void printRlInfo(RlInfo *rl)
{
    int i;
    for (i=0; i<rl->count; i++)
        printf("%2d:  Run=%2d  Level=%4d\n", i, rl->run[i], rl->level[i]);
}


/* Note: this should be called after initSeq() has been called */
void initBuffers(PicInfo *pic, uchar *frmBuf[])
{
    int frmSz = pic->nCol * pic->nRow;

    if (frmBuf[Y])  free(frmBuf[Y]);
    if (frmBuf[Cb])  free(frmBuf[Cb]);
    if (frmBuf[Cr])  free(frmBuf[Cr]);

    if ((frmBuf[Y]=(uchar*)malloc(frmSz))==NULL ||
        (frmBuf[Cb]=(uchar*)malloc(frmSz>>2))==NULL ||
        (frmBuf[Cr]=(uchar*)malloc(frmSz>>2))==NULL) {
            printf("Error: Failed to allocate frame buffers.\n");
            exit(-7235);
        }

#if 1
    // Zero out buffer content
    memset(frmBuf[Y], 0, frmSz);
    memset(frmBuf[Cb], 0, frmSz>>2);
    memset(frmBuf[Cr], 0, frmSz>>2);
#endif
}


void swapBuffers(uchar *curFrm[], uchar *newRefFrm[], uchar *oldRefFrm[])
{
    int cc;
    uchar *tmpBuf;

    for (cc=0; cc<3; cc++) {
        tmpBuf = oldRefFrm[cc];
        oldRefFrm[cc] = newRefFrm[cc];
        newRefFrm[cc] = curFrm[cc];
        curFrm[cc] = tmpBuf;
    }
}


void readFrm(char *seqName, PicInfo *pic, uchar *frmBuf[])
{
    int cc, xShift, yShift, width, height, maxValue;
    char filename[128];
    FILE *fp;

    for (cc=0; cc<3; cc++) {
        xShift = (cc && pic->chroma_format!=CHROMA_FORMAT_444)? 1:0;
        yShift = (cc && pic->chroma_format==CHROMA_FORMAT_420)? 1:0;
        sprintf(filename, seqName, pic->frmNo, COMP[cc]);
        if ((fp = fopen(filename, "rb")) == NULL) {
            printf("Error: Failed to open input file %s!\n", filename);
            exit(-4267);
        }
        readPgmHdr(fp, &width, &height, &maxValue);
        if (width!=pic->horizontal_size>>xShift ||
            height!=pic->vertical_size>>yShift) {
            printf("Error: Input frame %s is of different size!\n", filename);
            exit(-459);
        }
        readPgmData(fp, frmBuf[cc], width, height, pic->nCol>>xShift);
        fclose(fp);
    }
}


void saveFrm(char *seqName, PicInfo *pic, uchar *frmBuf[])
{
    int cc, shift;
    char filename[128];

    for (cc=0; cc<3; cc++) {
        shift = cc? 1:0;	/* Note: assume 4:2:0 here! */
        sprintf(filename, seqName, pic->frmNo, COMP[cc]);
        writePgm(filename, frmBuf[cc], pic->horizontal_size>>shift,
                 pic->vertical_size>>shift, 255, pic->nCol>>shift);
    }
}

