// QAM info
//
#define MAX_OUT_SESSIONS_PER_QAM        32
#define MAX_NUM_PAT_TPS                 4  /* ?? */

typedef struct qam_info_ {
    uint8 state;                // QAM state
    uint8 type;                 // QAM type
    que_elem_t ses_list;        // list of out_session_t

    // PAT generation
    pat_build_info_t pat_info;  // output PAT info
    que_elem_t pat_tp_list;

    // Flow table (maps to out session info)
    out_session_t* flow_tab[MAX_OUT_SESSIONS_PER_QAM];

} qam_info_t;


// Output session info
//
#define OUT_SESSION_ID_MASK     0x80000000

typedef struct out_session_ {
    que_elem_t link;
    uint8 status;               // session status
    uint8 mode;                 // session mode
    uint16 flags;

    uint32 id;                  // session ID (msb must be 1 for out session)
    uint32 qam_id;              // QAM ID
    uint32 flow_id;             // flow ID
    uint32 prog_num;            // program number for SPTS
                                // or starting program number of MPTS
    uint32 max_prog_cnt;        // max number of programs in this session
                                // (1 for SPTS)
                                // Note: we assume output program number for
                                //       MPTS are in consecutive range
                                // Note: For MPTS passthru, prog_num and
                                //       max_prog_cnt are not used.

    in_session_t *in_ses;       // points to input session

    uint32 prog_cnt;            // number of programs in the session
    que_elem_t prog_list;       // list of out_prog_info_t

    // Time processing
    uint32 tp_idx;              // TP index
    uint32 tp_interval;         // output TP interval in F(20.12) 45 kHz ticks
    uint32 out_time;            // delivery time if F(20.12) of 45 kHz ticks

    // Ouptut PID table
    pid_table_t *pid_tab;       // PID lookup table

    // TP info buffer
    tp_info_buf_t *tp_info_buf; // TP info buffer
    uint32 tp_info_idx;         // index of next TP info to process

    // PMT generation
    pmt_build_info_t *pmt_info;
    que_elem_t *pmt_tp_list;

} out_session_t;


// Output program info
//
typedef struct out_prog_ {
    que_elem_t link;
    uint16     prog_num;
    uint16     pmt_pid;
    int        pmt_ver;
    int        es_cnt;

    // PMT regeneration
    booleam    pmt_built;
    pmt_info_t pmt;
} out_prog_t;



 PAT build info
//   For output PAT regeneration
//
#define MAX_PROG_PER_QAM        32

typedef struct pat_build_info_ {
    int tsid;
    int ver;
    int cur_next;
    int sect_cnt;
    int prog_cnt;
    pat_prog_info_t prog_info[MAX_PROG_PER_QAM];
} pat_build_info_t;


// PMT build info
//   For output PMT regeneration
//
typedef struct pmt_build_info_ {
    pmt_info_t *orig_pmt;
    int prog_num;
    uint16 out_pid[MAX_ES_PER_PROG];
} pmt_build_info_t;
