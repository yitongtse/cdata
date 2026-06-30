h19792
s 00002/00000/00210
d D 2.3 02/03/04 15:38:23 ytse 4 3
c Update
e
s 00001/00000/00209
d D 2.2 02/02/26 17:12:53 ytse 3 2
c Improved MBA logic
e
s 00000/00000/00209
d D 2.1 00/08/21 11:04:22 ytse 2 1
c Before supporting Windows
e
s 00209/00000/00000
d D 1.1 99/10/29 15:58:14 yitong 1 0
c date and time created 99/10/29 15:58:14 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef INFO_H
#define INFO_H


/* Sequence, GOP and picture layer info
*/
typedef struct
{
    /* Sequence header syntax */
    int horizontal_size;
    int vertical_size;
    int aspect_ratio_information;
    int frame_rate_code;
    int bit_rate;
    int vbv_buffer_size;
    int constrained_parameters_flag;

    /* Sequence extension syntax */
    int profile_and_level_indication;
    int progressive_sequence;
    int chroma_format;
    int low_delay;
    int frame_rate_extension_n;
    int frame_rate_extension_d;

    /* Sequence display extension syntax */
    int video_format;
    int color_description;
    int color_primaries;
    int transfer_characteristics;
    int matrix_coefficients;
    int display_horizontal_size;
    int display_vertical_size;

    /* Quant matrix extension syntax */
    int load_intra_quantiser_matrix;
    int load_non_intra_quantiser_matrix;
    int load_chroma_intra_quantiser_matrix;
    int load_chroma_non_intra_quantiser_matrix;
    short* intra_quantiser_matrix;
    short* non_intra_quantiser_matrix;
    short* chroma_intra_quantiser_matrix;
    short* chroma_non_intra_quantiser_matrix;


    /* GOP header syntax */
    int drop_frame_flag;
    int time_code_hours;    
    int time_code_minutes;
    int time_code_seconds;  
    int time_code_pictures;
    int closed_gop;
    int broken_link;


    /* Picture header syntax */
    int temporal_reference;
    int picture_coding_type;
    int vbv_delay;
    int full_pel_forward_vector;
    int full_pel_backward_vector;

    /* Picture coding extension syntax */
    int f_code[2][2];		// 1st idx: 0-fwd, 1-bwd;  2nd idx: 0-x, 1-y
    int intra_dc_precision;
    int picture_structure;
    int top_field_first;
    int frame_pred_frame_dct;
    int concealment_motion_vectors;
    int q_scale_type;
    int intra_vlc_format;
    int alternate_scan;
    int repeat_first_field;
    int chroma_420_type;
    int progressive_frame;
    int composite_display_flag;
    int v_axis;
    int field_sequence;
    int sub_carrier;
    int burst_amplitude;
    int sub_carrier_phase;

    /* Picture display extension syntax */
    int frame_center_horizontal_offset[3];
    int frame_center_vertical_offset[3];

    /* Copyright extension */
    int copyright_flag;
    int copyright_identifier;
    int original_or_copy;
    int copyright_number_1;
    int copyright_number_2;
    int copyright_number_3;


    /* Internal variables */
    int mpeg2Flag;              // whether MPEG-2 syntax is used
    int firstExtRead;	        // whether first extension has been read
    int resetFlag;		// should be 1 at startup or after sequence
				// end code is read
    int nCol;			// number of columns (aligned picture width)
    int nRow;			// number of rows (aligned picture height)
    int nMbCol;			// number of macroblock columns
    int nMbRow;			// number of macroblock rows
    int nMb;			// number of macroblocks
    int mbCnt;			// number of macroblock processed so far
    int secondFld;		// second field picture?
    int trBase;			// temporal reference base
				// At beginning of GOP, set to frame number
				// of previous anchor frame
    int nextTrBase;		// Keep track of the last anchor frame number
				// (+1) for setting of trBase in next GOP
				// Note: BOTH trBase and nextTrBase should be
				// set to first frame number at startup
    int frmNo;			// current frame number
}
PicInfo;


/* Slice and Macroblock layer info

   Usage of mv, pmv in decoder:
       mv stores the value from the bitstream.
       pmv stores the value to be used in reconstruction

   Usage of mv, pmv in decoder:
       mv set to the motion vector values by motion estimation.
       After mv prediction, pmv is updated to the motion vector values,
       and mv the difference values.
*/
typedef struct
{
    /* Slice header syntax */
    int slice_vertical_position;
    int quantiser_scale_code;
    int intra_slice;

    int type;
    int motion_type;
    int dct_type;
    int mbaIncr;		/* (accumulated) mba increment */

    /* Flags */
    int intra;			/* intra(1) or inter(0) coded */
    int pattern;		/* cbp (and coefficients) sent? */
    int motion_forward;		/* use forward motion prediction? */
    int motion_backward;	/* use backward motion prediction? */
    int quant;			/* quant parameter sent? */

    int index;			/* macroblock index or address */
    Coor mv[2][2];		/* first index: 0: forward, 1: backward */
				/* second index: motion vector number */
                                /* Note: for dual-prime, mv[0][1] is dmv */
    int fldSel[2][2];		/* field select */
				/* first index: 0: forward, 1: backward */
				/* second index: motion vector number */

    int cbp;			/* coded block pattern */
				/* Note: bits in cbp is defined differently
                                         from MPEG-2:  bit (11-i) of cbp
					 indicates whether block i is coded
					 or not, i = 0 to 11.
				*/

    /* Internal variables */
I 3
    int mbaRef;			/* reference for mbaIncr */
E 3
    Coor pos;			/* position (of upper left corner) */
    Coor pmv[2][2];		/* indices usage are same as for mv[][] */
    int dc_dct_pred[3];		/* DC predictors */
}
MbInfo;


/* Run-level pair info
*/
typedef struct
{
    int count;			/* number of RL pairs */
    int run[64];		/* runlengths */
    int level[64];		/* levels (with sign) */
}
RlInfo;

void initPicInfo(PicInfo *pic);
void initSeq(PicInfo *pic);  // set up pic info based on sequence level headers
void initGop(PicInfo *pic);
void initPic(PicInfo *pic);
void initSlc(PicInfo *pic, MbInfo *mb);
void initMb(PicInfo *pic, MbInfo *mb);
void initQuantMatrices(PicInfo *pic);

void initBuffers(PicInfo *pic, uchar *frmBuf[]);
void swapBuffers(uchar *curFrm[], uchar *newRefFrm[], uchar *oldRefFrm[]);
void printSeqInfo(PicInfo *pic);


void printMbInfo(PicInfo *pic, MbInfo *mb);
void setMbAttrib(MbInfo *mb, int idx);
I 4
int  isSkippedMb(MbInfo *mb);
int  setMbType(MbInfo *mb, int pic_coding_type);
E 4
void resetMvPred(PicInfo* pic, MbInfo* mb);
void resetDcDctPred(PicInfo* pic, MbInfo* mb);
void setDefaultMotion(PicInfo* pic, MbInfo* mb);
void resetFwdMv(MbInfo* mb);

int addRl(RlInfo *rl, int run, int level);
void printRlInfo(RlInfo *rl);

void readFrm(char *seqName, PicInfo *pic, uchar **frmBuf);
void saveFrm(char *seqName, PicInfo *pic, uchar **curFrm);

#endif
E 1
