#define	NUM_CHAN		30
				// total number of channels

#define	DATA_SIZE		1024000
				// data size in bytes

#define NUM_SCHWIN		1000
				// number of schedule windows to benchmark

#define START_TIME		0
				// local clock start time


int numQam = 2;			// actual number of QAMs used
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
    }
};

