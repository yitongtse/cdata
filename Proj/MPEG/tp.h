#ifndef TP_H
#define TP_H


#define	TP_SIZE		188
#define	TP_SIZE2	204
#define	TP_SYNC		0x47

#define	TP_ERROR_LOST_SYNC	1


/* MPEG-2 transport stream packet info
*/
typedef struct
{
    /* Packet header syntax */
    int transport_error_indicator;
    int payload_unit_start_indicator;
    int transport_priority;
    int PID;
    int transport_scrambling_control;
    int adaptation_field_control;
    int continuity_counter;

    /* Adaptation field syntax */
    int adaptation_field_length;
    int discontinuity_indicator;
    int random_access_indicator;
    int elementary_stream_priority_indicator;
    int PCR_flag;
    int OPCR_flag;
    int splicing_point_flag;
    int transport_private_data_flag;
    int adaptation_field_extension_flag;

	/* Note: According to MPEG-2, PCR and OPCR consist of:
		 - a base part of 33 bits in 90 kHz, and
                 - an extension part of 9 bits in 27 MHz
                 However, due to 32-bit word consideration, they are stored
                 internally in the following format:
	         [0]: bits 32-1 of base part, in 45 kHz
 		 [1]: (bits 0 of base part x 300) + extension part, in 27 MHz,
                      right aligned
	*/
    uint PCR[2];
    uint OPCR[2];

    int splice_countdown;
    int transport_private_data_length;

    int nbytes;			/* number of bytes already read or written */
    int stuffSz;		/* number of stuff bytes */
}
TpInfo;


void initTp(TpInfo *tp);

int findTp(FILE *fp, int tpSize, int syncCount);
int seekTp(TpInfo *tp, Reader *in, int tpSize, int syncCount);
int skipTp(TpInfo *tp, Reader *in, int tpSize);
int getTpHdr(TpInfo *tp, Reader *in);
int getTpAf(TpInfo *tp, Reader *in);
int skipTpAf(TpInfo *tp, Reader *in);

void putTpHdr(TpInfo *tp, Writer *out);
void putTpAf(TpInfo *tp, Writer *out);

#endif

