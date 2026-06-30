#define	NUM_CHAN		30
				// total number of channels

#define	DATA_SIZE		384000
				// data size in bytes

#define NUM_SCHWIN		1000
				// number of schedule windows to benchmark

#define START_TIME		0
				// local clock start time


#if DATA_FROM_FILE
#define DATA_FILE_NAME		"data/ip_1x24_z.dat"
Uint32 rawData[DATA_SIZE >> 2];
#endif


#if DATA_INCLUDED
#include ip_1x24z.h
extern Uint32 rawData[];
#endif



int numQam = 1;			// actual number of QAMs used
int numProg = 24;		// actual number of programs per QAM used

//int qamRate = 26970350;		// QAM bitrate (assume 6 MHz 64 QAM)
//int qamRate = 38810700;		// QAM bitrate (assume 6 MHz 256 QAM)
//int qamRate = 38500000;		// QAM bitrate (assume 8 MHz 64 QAM)
//int qamRate = 51300000;		// QAM bitrate (assume 8 MHz 256 QAM)
int qamRate = 75000000;		// for the 24 channel simulation only


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
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc004, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc006, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc008, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc00a, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc00c, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc00e, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc010, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc012, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc014, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc016, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc018, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc01a, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc01c, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc01e, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc020, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc022, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc024, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc026, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc028, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc02a, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc02c, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc02e, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    }
};

