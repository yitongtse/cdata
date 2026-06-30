h54831
s 00000/00000/00142
d D 2.1 00/08/21 11:04:23 ytse 2 1
c Before supporting Windows
e
s 00142/00000/00000
d D 1.1 99/10/29 15:58:14 yitong 1 0
c date and time created 99/10/29 15:58:14 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef MPEG2_H
#define MPEG2_H

enum SyntaxLevel { SYNC=0, SEQ=1, GOP=2, PIC=3, SLC=4, MB=5 };

/* Start codes */
#define	SEQ_START_CODE		0xB3
#define	SEQ_END_CODE		0xB7
#define	GOP_START_CODE		0xB8
#define	PIC_START_CODE		0x00
#define	MIN_SLICE_START_CODE	0x01
#define	MAX_SLICE_START_CODE	0xAF
#define	EXT_START_CODE		0xB5
#define	USER_DATA_START_CODE	0xB2

/* Extension start code identifiers */
#define	SEQ_EXT			0x1
#define	SEQ_DISP_EXT		0x2
#define	QUANT_MATRIX_EXT	0x3
#define	COPYRIGHT_EXT		0x4
#define	SEQ_SCALABLE_EXT	0x5
#define	PIC_DISP_EXT		0x7
#define	PIC_CODE_EXT		0x8
#define	PIC_SPAT_SCALABLE_EXT	0x9
#define	PIC_TEMP_SCALABLE_EXT	0xa

/* Chroma formats */
#define	CHROMA_FORMAT_420	1
#define	CHROMA_FORMAT_422	2
#define	CHROMA_FORMAT_444	3

/* Luma, chroma components */
#define	Y			0
#define	Cb			1
#define	Cr			2

/* Picture coding types */
#define	I_PIC			1
#define	P_PIC			2
#define	B_PIC			3

/* Picture structures */
#define	TOP_FIELD		1
#define	BOTTOM_FIELD		2
#define	FRAME			3

/* Constants used in macroblock_address_increment VLC */
#define	MBA_STUFF		33
#define	MBA_ESC			34
#define	MBA_SC			35

/* Macroblock types */
#define SKIPPED			-1
#define	I_INTRA			0
#define	I_INTRA_Q		1

#define	P_MC_DPCM		0
#define	P_DPCM			1
#define	P_MC_REP		2
#define	P_INTRA			3
#define	P_MC_DPCM_Q		4
#define	P_DPCM_Q		5
#define	P_INTRA_Q		6

#define	B_MC_REP_FB		0
#define	B_MC_DPCM_FB		1
#define	B_MC_REP_B		2
#define	B_MC_DPCM_B		3
#define	B_MC_REP_F		4
#define	B_MC_DPCM_F		5
#define	B_INTRA			6
#define	B_MC_DPCM_FB_Q		7
#define	B_MC_DPCM_F_Q		8
#define	B_MC_DPCM_B_Q		9
#define	B_INTRA_Q		10

/* Prediction types */
#define	FRAME_FIELD_BASED	1
#define	FRAME_FRAME_BASED	2
#define	FRAME_DUAL_PRIME	3
#define	FIELD_FIELD_BASED	1
#define	FIELD_16X8_MC		2
#define	FIELD_DUAL_PRIME	3

/* Constants used in DCT coefficient VLC */
#define COEF_ESC		111
#define COEF_EOB		112
#define COEF_ERR		113


/* VLC tables */
extern char mba_VLC[];
extern char mbType_I_VLC[];
extern char mbType_P_VLC[];
extern char mbType_B_VLC[];
extern char cbp_VLC[];
extern char mv_VLC[];
extern char dmv_VLC[];
extern char lumaSize_VLC[];
extern char chromaSize_VLC[];
extern char coef0_VLC[];
extern char coef1_VLC[];

/* Tables */
extern float frameRateTab[8];
extern short intraQmatDefault[64];
extern short interQmatDefault[64];
extern short intraQmatCustom[64];
extern short interQmatCustom[64];
extern short chromaIntraQmatCustom[64];
extern short chromaInterQmatCustom[64];
extern int mbAttrib[21][5];
extern short zigzagScanTab[64];
extern short altScanTab[64];
extern short quantScaleTab[2][31];
extern uchar *clipTab;

/* Informative labels */
extern char *PROFILE[];
extern char *LEVEL[];
extern char *CHROMA_FORMAT[];
extern char *PIC_TYPE[];
extern char *PIC_STRUCT[];
extern char *MC_TYPE[];
extern char *PRED_TYPE[];
extern char *MB_TYPE[];
/* extern char **MB_TYPE[]; */
extern char *DCT_TYPE[];

extern short *curBlk[];


/* Debugging parameters */
extern int debug_flag;
extern int debug_frmNo;
extern int debug_mba;


void initClipTab();


#endif
E 1
