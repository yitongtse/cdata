#define	NUM_CHAN		150
				// total number of channels

#define	DATA_SIZE		3840000
				// data size in bytes

#define	DATA_SIZE_WORD		960000

#define NUM_SCHWIN		1000
				// number of schedule windows to benchmark

#define START_TIME		0
				// local clock start time


#if DATA_FROM_FILE
#define DATA_FILE_NAME          "data/ip_10x15_z.dat"
Uint32 rawData[DATA_SIZE >> 2];
#endif


#if DATA_INCLUDED
#include "data/ip_10x15z.h"
extern Uint32 rawData[];
#endif



int numQam = 10;		// actual number of QAMs used
int numProg = 15;		// actual number of programs per QAM used

//int qamRate = 26970350;		// QAM bitrate (assume 6 MHz 64 QAM)
//int qamRate = 38810700;		// QAM bitrate (assume 6 MHz 256 QAM)
//int qamRate = 38500000;		// QAM bitrate (assume 8 MHz 64 QAM)
int qamRate = 51300000;		// QAM bitrate (assume 8 MHz 256 QAM)


// This structure contains all information of a channel for simulation
typedef struct {
    Uint16  udpDestPort;
    Uint16  numPid;		// max 4
    PidInfo pidMap[4];
} ChanInfo;


ChanInfo chanInfo[NUM_CHAN] = {

    // QAM 1
    //
    { 0xc000, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc002, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc004, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc006, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc008, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc00a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc00c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc00e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc010, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc012, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc014, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc016, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc018, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc01a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc01c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 2
    //
    { 0xc040, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc042, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc044, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc046, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc048, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc04a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc04c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc04e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc050, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc052, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc054, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc056, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc058, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc05a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc05c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 3
    //
    { 0xc080, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc082, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc084, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc086, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc088, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc08a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc08c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc08e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc090, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc092, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc094, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc096, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc098, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc09a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc09c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 4
    //
    { 0xc0c0, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc0c2, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc0c4, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc0c6, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc0c8, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc0ca, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc0cc, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc0ce, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc0d0, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc0d2, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc0d4, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc0d6, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc0d8, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc0da, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc0dc, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 5
    //
    { 0xc100, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc102, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc104, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc106, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc108, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc10a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc10c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc10e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc110, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc112, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc114, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc116, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc118, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc11a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc11c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 6
    //
    { 0xc140, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc142, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc144, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc146, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc148, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc14a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc14c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc14e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc150, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc152, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc154, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc156, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc158, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc15a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc15c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 7
    //
    { 0xc180, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc182, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc184, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc186, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc188, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc18a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc18c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc18e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc190, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc192, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc194, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc196, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc198, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc19a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc19c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 8
    //
    { 0xc1c0, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc1c2, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc1c4, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc1c6, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc1c8, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc1ca, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc1cc, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc1ce, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc1d0, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc1d2, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc1d4, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc1d6, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc1d8, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc1da, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc1dc, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 9
    //
    { 0xc200, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc202, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc204, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc206, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc208, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc20a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc20c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc20e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc210, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc212, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc214, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc216, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc218, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc21a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc21c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    // QAM 10
    //
    { 0xc240, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc242, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc244, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc246, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc248, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc24a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc24c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc24e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc250, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc252, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc254, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc256, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc258, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc25a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc25c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    }
};

