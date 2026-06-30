h03495
s 00046/00002/00064
d D 1.4 08/09/18 14:07:09 ytse 4 3
c Update
e
s 00000/00001/00066
d D 1.3 08/09/08 23:20:24 ytse 3 2
c update
e
s 00001/00000/00066
d D 1.2 08/09/08 22:49:08 ytse 2 1
c update
e
s 00066/00000/00000
d D 1.1 08/09/06 00:25:44 ytse 1 0
c date and time created 08/09/06 00:25:44 by ytse
e
u
U
f e 0
t
T
I 4
#define MAX_PID                 0x1FFF
#define UNKNOWN_IDX             -1

E 4
I 1
#define SCALE_BASE_EXT          600


typedef int64 mpeg_time_t;


// PID info
//
typedef struct {
    int32 first_idx;
    int32 prev_idx;
    uint32 tp_cnt;
    int32 prog_idx;
} pid_info_t;


// MPEG-2 TP info
//
typedef struct {
D 4
    uint8 pcr_flag : 1;         // whether the TP has PCR
E 4
I 4
    uint8 pusi      : 1;        // payload_unit_start_indicator
    uint8 pcr_flag  : 1;        // whether the TP has PCR
    uint8 ra_flag   : 1;        // whether the TP is an random access point
E 4
    uint8 disc_flag : 1;        // whether TP is a discontinue point
D 4
    uint8 cc : 4;               // countinuity counter
E 4
I 4
    uint8 cc        : 4;        // countinuity counter
E 4

    uint16 pid;
    int32 pid_next_idx;         // TP index for the next TP in the same PID
    int32 prog_next_idx;        // TP index of the next TP in the same program
    mpeg_time_t pcr;            // input or interpolated PCR
                                // (in F.16 of 27 MHz ticks)
} tp_info_t;


// Program info
//
typedef struct {
    int32 prog_num;
    int32 first_idx;
    int32 prev_idx;
I 2
    int32 first_pcr_idx;
E 2
D 3
    int32 prev_pcr_idx;
E 3
    uint32 tp_cnt;
} prog_info_t;


// PAT info
#define MAX_PROGS               30

typedef struct {
    uint8 prog_cnt;             // number of programs
    uint16 nit_pid;             // NIT PID
    uint8* buf;                 // PAT buffer
    pat_header_t *pat_hdr;      // points to the PAT header
    pat_prog_info_t *prog_info[MAX_PROGS];
} pat_info_t;


// PMT info
//
#define MAX_ES_PER_PROG         15

typedef struct {
    uint8 es_cnt;               // number of elementary streams in PMT
    uint16 prog_num;            // program number
    uint8* buf;                 // PMT buffer
    pmt_header_t* pmt_hdr;      // this actually holds the PMT buffer!
    pmt_es_info_t *es_info[MAX_ES_PER_PROG];
} pmt_info_t;
I 4


typedef struct {
    uint8* buf;                 // PES header buffer
    pes_header_t* pes_hdr;      // this actually holds the PES header buffer!
    mpeg_time_t pts;            // PTS (-1 if not present)
    mpeg_time_t dts;            // DTS (-1 if not present)
} pes_info_t;


// Global variables
//
extern tp_info_t *tp_info_array;
extern pid_info_t pid_info_array[MAX_PID + 1];
extern prog_info_t prog_info_array[MAX_PROGS];

extern uint8 tp_buf[TP_SIZE];
extern pat_info_t pat_info;
extern pmt_info_t pmt_info[MAX_PROGS];
extern pes_info_t pes_info;


// Utilities
//
void print_hex(void* ptr, int nbytes);
void dump_pid_info(void);
void dump_prog_info(void);
void dump_tp_header(tp_header_t *tp_hdr);
void trace_pid(int16 pid);
void trace_prog(int prog_idx);
mpeg_time_t parse_pcr(af_header_t *af_hdr);
void snoop_pat(pat_info_t *pat, uint8 *tp_buf);
void dump_pat(pat_info_t *pat);
void snoop_pmt(pmt_info_t *pmt, uint8 *tp_buf, int32 prog_idx);
void dump_pmt(pmt_info_t *pmt);
mpeg_time_t get_time_stamp(time_stamp_t *ts);
void snoop_pes(pes_info_t *pes_info, uint8 *tp_buf);
void dump_pes(pes_info_t *pes_info);
uint32 tp_itvl_to_bitrate(mpeg_time_t tp_itvl);
E 4

E 1
