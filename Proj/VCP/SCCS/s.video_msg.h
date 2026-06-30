h48217
s 00108/00000/00000
d D 1.1 06/11/17 13:50:01 ytse 1 0
c date and time created 06/11/17 13:50:01 by ytse
e
u
U
f e 0
t
T
I 1
enum {
}


// Video session allocation
//
typedef struct {
    uint32 session_type;        // IN_SESSION, OUT_SESSION
    uint32 num_session;
} video_session_alloc_cmd;


typedef struct {
    uint32 status;
    uint32 session_type;
    uint32 num_session;
    uint32 session_id[0];
} video_session_alloc_resp;


// Video input session configuration
//
typedef struct {
    uint32 session_id;
    uint8 enable;               // whether to turn on (1) the port or not (0)
    uint8 input_type;           // input type: UNICAST or MCAST
    ipaddrtype dst_ip;
    ipaddrtype src_ip;
    uint16 dst_udp;
    uint16 src_udp;
    uint8 proc_type;            // process type: SPTS_REMUX, DATA_INJECT,
                                //               MPTS_PASSTHRU, MPTS_REMUX
    uint32 bitrate;             // allocated bitrate in bps
    uint32 idle_thres;          // idle threshold in millisecond
} video_in_cfg_cmd;


typedef struct {
    uint32 status;
    uint32 session_id;
} video_in_cfg_resp;


// Video input session program selection
//
typedef struct {
    uint32 session_id;
    uint8 enable;
    uint32 prog_cnt;
    uint16 prog_num[0];
} video_in_prog_select_cmd;


typedef struct {
    uint32 status;
} video_in_prog_select_resp;


// Video input session PID filter
//
typedef struct {
    uint32 session_id;
    uint8 enable;
    uint32 pid_cnt;
    uint16 pid[0];
} video_in_pid_filter_cmd;


typedef struct {
    uint32 status;
} video_in_pid_filter_resp;


// Video input session delete
//
typedef struct {
    uint32 session_id;
} video_in_session_delete_cmd;


typedef struct {
    uint32 status;
} video_in_session_delete_resp;


// Video output session configuration
//
typedef struct {
    uint32 in_session_id;       // corresponding input session ID
    uint32 qam_id;
    uint16 prog_num;            // output program number
} video_out_cfg_msg;


typedef struct {
    uint32 status;
    uint32 session_id;
} video_out_cfg_resp;


// Video input session status update
//   LC -> SUP
//
typedef struct {
    uint32 session_id;
    uint32 session_status;    
} video_in_status_msg;

E 1
