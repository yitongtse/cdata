/* MPEG-2 definitions common to encoder and decoder */

#include <stdio.h>
#include "util.h"
//#include "mpeg2.h"


/* Informative labels */
char* PROFILE[] =
    {"reserved", "HIGH", "SPATIAL", "SNR",
     "MAIN", "SIMPLE", "reserved", "reserved"};

char* LEVEL[] =
    {"reserved", "reserved", "reserved", "reserved",
     "HIGH", "reserved", "HIGH-1440", "reserved",
     "MAIN", "reserved", "LOW", "reserved",
     "reserved", "reserved", "reserved", "reserved"};
 
char* CHROMA_FORMAT[] =
    {"reserved", "4:2:0", "4:2:2", "4:4:4"};

char* PIC_TYPE[] =
    {"forbidden", "I", "P", "B", "D"};

char* PIC_STRUCT[] =
    {"reserved", "TOP_FIELD", "BOTTOM_FIELD", "FRAME"};

char* MC_TYPE[] =
    {"NONE", "FORWARD", "BACKWARD", "BIDIRECTIONAL"};

char* PRED_TYPE[] =
    {"FRM_RESERVED", "FRM_FIELD_PRED",
     "FRM_FRAME_PRED", "FRM_DUAL_PRIME",
     "FLD_RESERVED", "FLD_FIELD_PRED",
     "FLD_16X8", "FLD_DUAL_PRIME"};

char* MB_TYPE[] =
    {"REP",
     "I_INTRA", "I_INTRA_Q",
     "P_MC_DPCM", "P_DPCM", "P_MC_REP", "P_INTRA", "P_MC_DPCM_Q", "P_DPCM_Q",
     "P_INTRA_Q",
     "B_MC_REP_FB", "B_MC_DPCM_FB", "B_MC_REP_B", "B_MC_DPCM_B", "B_MC_REP_F",
     "B_MC_DPCM_F", "B_INTRA", "B_MC_DPCM_FB_Q", "B_MC_DPCM_F_Q",
     "B_MC_DPCM_B_Q", "B_INTRA_Q"};
/*
char *MB_TYPE_I_PIC[] = {
     "I_INTRA", "I_INTRA_Q"
    };

char *MB_TYPE_P_PIC[] = {
     "P_MC_DPCM", "P_DPCM", "P_MC_REP", "P_INTRA", "P_MC_DPCM_Q", "P_DPCM_Q",
     "P_INTRA_Q"
    };

char *MB_TYPE_B_PIC[] = {
     "B_MC_REP_FB", "B_MC_DPCM_FB", "B_MC_REP_B", "B_MC_DPCM_B", "B_MC_REP_F",
     "B_MC_DPCM_F", "B_INTRA", "B_MC_DPCM_FB_Q", "B_MC_DPCM_F_Q",
     "B_MC_DPCM_B_Q", "B_INTRA_Q"
    };

char **MB_TYPE[] = {0, MB_TYPE_I_PIC, MB_TYPE_P_PIC, MB_TYPE_B_PIC };
*/
 
char* DCT_TYPE[] =
    {"FRAME_DCT", "FIELD_DCT"};


/* Tables
*/ 

/* Frame rate table */
float frameRateTab[8] =  {
    23.976, 24, 25, 29.97, 30, 50, 59.94, 60
};

/* Default intra quantizer matrix */
short intraQmatDefault[64] =  {
     8, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};

/* Default inter quantizer matrix */
short interQmatDefault[64] =  {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16
};

/* Custom quantizer matrices */
short intraQmatCustom[64];
short interQmatCustom[64];
short chromaIntraQmatCustom[64];
short chromaInterQmatCustom[64];

/* Macroblock attributes
       {quant, motion_forward, motion_backward, pattern, intra}
*/
int mbAttrib[21][5] = {
    { 0, 0, 0, 0, 0 },          // Skipped macroblock

    { 0, 0, 0, 0, 1 },          // I_INTRA
    { 1, 0, 0, 0, 1 },          // I_INTRA_Q

    { 0, 1, 0, 1, 0 },          // P_MC_DPCM
    { 0, 0, 0, 1, 0 },          // P_DPCM
    { 0, 1, 0, 0, 0 },          // P_MC_REP
    { 0, 0, 0, 0, 1 },          // P_INTRA
    { 1, 1, 0, 1, 0 },          // P_MC_DPCM_Q
    { 1, 0, 0, 1, 0 },          // P_DPCM_Q
    { 1, 0, 0, 0, 1 },          // P_INTRA_Q
 
    { 0, 1, 1, 0, 0 },          // B_MC_REP_FB
    { 0, 1, 1, 1, 0 },          // B_MC_DPCM_FB
    { 0, 0, 1, 0, 0 },          // B_MC_REP_B
    { 0, 0, 1, 1, 0 },          // B_MC_DPCM_B
    { 0, 1, 0, 0, 0 },          // B_MC_REP_F
    { 0, 1, 0, 1, 0 },          // B_MC_DPCM_F
    { 0, 0, 0, 0, 1 },          // B_INTRA
    { 1, 1, 1, 1, 0 },          // B_MC_DPCM_FB_Q
    { 1, 1, 0, 1, 0 },          // B_MC_DPCM_F_Q
    { 1, 0, 1, 1, 0 },          // B_MC_DPCM_B_Q
    { 1, 0, 0, 0, 1 },          // B_INTRA_Q
};


/* Zig-zag scan */
short zigzagScanTab[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* Alternate scan */
short altScanTab[64] = {
     0,  8, 16, 24,  1,  9,  2, 10,
    17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12,
    19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14,
    21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31,
    38, 46, 54, 62, 39, 47, 55, 63
};


/* Quantizer scale tables */
short quantScaleTab[2][31] = {
    {   2,   4,   6,   8,  10,  12,  14,  16,
       18,  20,  22,  24,  26,  28,  30,  32,
       34,  36,  38,  40,  42,  44,  46,  48,
       50,  52,  54,  56,  58,  60,  62       },

    {   1,   2,   3,   4,   5,   6,   7,   8,
       10,  12,  14,  16,  18,  20,  22,  24,
       28,  32,  36,  40,  44,  48,  52,  56,
       64,  72,  80,  88,  96, 104, 112       }
};


/* Storage space for 8x8 blocks (enought for even 4:4:4) */
short curBlock[12*64];
short *curBlk[12] = {
    curBlock+64*0, curBlock+64*1, curBlock+64*2, curBlock+64*3,
    curBlock+64*4, curBlock+64*5, curBlock+64*6, curBlock+64*7,
    curBlock+64*8, curBlock+64*9, curBlock+64*10, curBlock+64*11
};



static uchar CLIP[1024];
uchar *clipTab;


void initClipTab()
{
    int i;
    clipTab = CLIP + 384;
    for (i=-384; i<0; i++)  clipTab[i] = 0;
    for ( ; i<256; i++)  clipTab[i] = i;
    for ( ; i<640; i++)  clipTab[i] = 255;
}
