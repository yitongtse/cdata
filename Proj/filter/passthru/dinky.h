#include <devctl.h>

typedef struct {
	unsigned total;
	unsigned drop;
	unsigned pkt_all;
	unsigned pkt_dropped;
	pthread_mutex_t mutex;
	unsigned dcount;
	unsigned pcount;
	unsigned *dropid;
} droprate_t;

struct dinky_control {
	int reg_hdl;
	io_net_self_t *ion;
	dispatch_t *dpp;
	uint16_t endpoint;
	uint16_t zero;
  
	/* dropping rate */
	droprate_t irate;
	droprate_t orate;
	unsigned   dropflag;
};

#define DROP_FLAG_ARP    0x00000001
#define DROP_FLAG_IP     0x00000002
#define DROP_FLAG_UDP    0x00000004
#define DROP_FLAG_TCP    0x00000008
#define DROP_FLAG_QNET   0x00000010
#define DROP_FLAG_ONWIRE 0x00010000

#define IPPROTO_QNET     106

/* DCMDs to control dinky */
#define _DCMD_DINKY      0x80

struct dinky_stat {
	droprate_t irate;
	droprate_t orate;
	unsigned dropflag;
};

struct dinky_rate {
	unsigned total;
	unsigned drop;
};

#define DCMD_DINKY_GETSTAT  __DIOF(_DCMD_DINKY,  1, struct dinky_stat)
#define DCMD_DINKY_SETFLAG  __DIOT(_DCMD_DINKY,  2, unsigned)
#define DCMD_DINKY_SETIRATE __DIOT(_DCMD_DINKY,  3, struct dinky_rate)
#define DCMD_DINKY_SETORATE __DIOT(_DCMD_DINKY,  4, struct dinky_rate)
#define DCMD_DINKY_SETRATE  __DIOT(_DCMD_DINKY,  5, struct dinky_rate)
