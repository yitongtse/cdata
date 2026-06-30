// Values always take up multiple of words for alignment.
// For values less than a word, stuffing bytes will be added to make up
// to word boundary

TLVs for session config:
   TYPE_IN_SESSION_ID, 4, in_session_id
   TYPE_SESSION_ENABLE, 4, (0 or 1)
   TYPE_INPUT_TYPE, 4, (UNICAST or MULTICAST)
   TYPE_SRC_IPV4_ADDR, 4, source IP address
   TYPE_DST_IPV4_ADDR, 4, destination IP address
   TYPE_SRC_UDP_PORT, 4, source UDP port (16-bit)
   TYPE_DST_UDP_PORT, 4, destination UDP port (16-bit)
   TYPE_STREAM_TYPE, 4, (SPTS or MPTS)
   TYPE_PID_REMAP, 4, (0 or 1)
   TYPE_PARSE_PSI, 4, (0 or 1)
   TYPE_DEJITTER, 4, (0 or 1)
   TYPE_BITRATE_ALLOC, 4, allocated bitrate in bps
   TYPE_IDLE_THRESHOLD, 4, idle threshold in millisecond
   TYPE_PROGRAM_SELECT, 4n+4, { uint32 prog_cnt, uint32 program_num[n] }
   TYPE_PID_FILTER, 4n+4, { uint32 pid_cnt, uint32 pid[n] }

   TYPE_OUT_SESSION_ID, 4, out_session_id
   TYPE_QAM_ID, 4, qam_id
   TYPE_PROGRAM_NUMBER, 4, MPEG program number (16-bit)

TVLs specific for query:
   TYPE_BITRATE_ACTUAL, 4, actual bitrate in bps
   TYPE_PMT_SECTION, n, pmt_section in raw format
   TYPE_PSI_SNOOP_STATUS, 4, 0 or 1
   TYPE_SESSION_STATUS, 4, 0 or 1



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


// Video session configuration
//   Used to set up a new session, or modify an existing session
//   The first TLV must be either TYPE_IN_SESSION_ID or TYPE_OUT_SESSION_ID
//   Input and output sessions TLVs must not be mixed in the same message
//
typedef struct {
    TLVs
} video_session_config;

typedef struct {
    uint32 status;
} video_cfg_resp;



// Video input session delete
//
typedef struct {
    uint32 session_id;
} video_in_session_delete_cmd;

typedef struct {
    uint32 status;
} video_in_session_delete_resp;



// Video input session status update
//   LC -> SUP
//
typedef struct {
    uint32 session_id;
    uint32 session_status;    // SESSION_IDLE, SESSION_ACTIVE
} video_in_status_msg;


// Video session query
//
typedef struct {
    contains a set of types for query
} video_session_query_cmd;

typedef struct {
    uint32 status;            // OK, NOT_UNDERSTOOD, SESSION_NOT_FOUND
    TLVs of queried attributes
} video_session_query_resp;



