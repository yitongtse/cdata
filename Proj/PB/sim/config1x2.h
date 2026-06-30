#define	NUM_CHAN		2
				// total number of channels

#define	DATA_SIZE		122880
				// data size in bytes

#define NUM_SCHWIN		1000
				// number of schedule windows to benchmark

#define START_TIME		0
				// local clock start time


#if DATA_FROM_FILE
#define DATA_FILE_NAME		"data/ip_1x2z.dat"
Uint32 rawData[DATA_SIZE >> 2];
#endif


#if DATA_INCLUDED
#include "data/ip_1x2z.h"
extern Uint32 rawData[];
#endif



int numQam = 1;			// actual number of QAMs used
int numProg = 2;		// actual number of programs per QAM used

int qamRate = 26970350;		// QAM bitrate (assume 6 MHz 64 QAM)
//int qamRate = 38810700;		// QAM bitrate (assume 6 MHz 256 QAM)
//int qamRate = 38500000;		// QAM bitrate (assume 8 MHz 64 QAM)
//int qamRate = 51300000;		// QAM bitrate (assume 8 MHz 256 QAM)


// This structure contains all information of a channel for simulation
typedef struct {
    Uint16  udpDestPort;
    Uint16  numPid;		// max 4
    PidInfo pidMap[4];
} ChanInfo;


ChanInfo chanInfo[NUM_CHAN] = {
    { 0xc000, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc002, 4,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    }
};

