// PAT build info
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
} out_session_t;


// Carousel
//
typedef crsl_ {
    int        num_tp;          // number of TPs in carousel
    que_elem_t tp_list;         // TP list
    uint32     send_period;     // send period
    uint32     next_send_time;  // refers to 45 kHz system clock
    uint8      next_cc;         // next continuity counter to use
} crsl_t;


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
    crsl_t     pmt_crsl;
} out_prog_t;


/****************************************************************************/

// Global variables
//
pool_t out_prog_pool;

/****************************************************************************/

// Utilities
//

// Return the first PID assigned to a program number
//   We use a fixed relationship between PIDs and program number.
//   PID 0 to 47 are reserved.
//   Each program has 32 PIDs reserved.
//
static inline
uint16 get_pid_by_prog_num (int prog_num)
{
    return 0x30 + prog_num * 32;
}


// Allocate an output program info with specified program number
//
out_prog_t* out_prog_alloc (int prog_num)
{
    out_prog_t* prog = (out_prog_t*)pool_alloc(&out_prog_pool);
    if (prog) {
        memset(prog, 0, sizeof(out_prot_t));
        prog->prog_num = prog_num;
        prog->pmt_pid = get_pid_by_prog_num(prog_num);
        prog->pmt_ver = 0;
    }
    return prog;
}


// Free an output program info
//
void out_prog_free (out_prog_t *prog)
{
    if (prog->pmt.sect) {
        pool_free(&psi_section_pool, &prog->pmt.sect->link);
    }
    pool_free(&out_prog_pool, &prog->link);
}


// Find out_prog_t in a list with specified PMT PID
// Returns NULL if not found
//
out_prog_t* out_prog_lookup_by_pmt_pid (que_elem_t *prog_list, uint16 pmt_pid)
{
    out_prog_t *prog;

    FOR_ALL_ELEMENTS_IN_QUE(prog_list, (que_elem_t*)prog) {
        if (prog->pmt_pid == pmt_pid) {
            return prog;
        }
    }
    return NULL;
}


// Remap PIDs of a program
//   Output PIDs are selected based on the given out program number.
//   QAM's output PID table should be checked to avoid PID collision (TBD).
//
void pmt_modify (pmt_info_t *pmt, int prog_num, int ver, pid_table_t* pid_tab)
{
    int i, idx;
    uint16 pid = get_pid_by_prog_num(prog_num) + 1;
    pmt_header_t* pmt_hdr = (pmt_header_t*)(pmt->sect + 1);
    uint16 orig_pcr_pid = pmt_hdr->pcr_pid;

    pmt_hdr->prog_num = prog_num;
    pmt_hdr->ver = ver;

    for (i=0; i<out_prog->es_cnt; i++) {
        // Find a PID that is not in the PID table
        do {
            idx = pid_table_lookup(pid_tab, pid);
        } while (idx == PID_NOT_FOUND);

        orig_pid = es_info_get_pid(pmt->es_info[i]);
        if (orig_pid == orig_pcr_pid) {
            pmt_hdr->pcr_pid = pid;
        }
        es_info_set_pid(pmt->es_info[i], pid);
    }
}


// Copy PSI section
//
static inline
void psi_section_copy (psi_section_t *to_sect, psi_section_t *from_sect)
{
    int copy_size = from_sect->sect_hdr.sect_len + 3;
    memcpy(to_sect->sect_hdr, from_sect->sect_hdr, copy_size);
}


// Set CRC32 field of PSI section
//
static inline
void psi_section_set_crc (psi_section_t *sect, uint32 crc)
{
    uint8* ptr = (uint8*)sect->sect_hdr;
    memcpy(ptr + sect->sect_hdr.sect_len + 3 - 4, &crc, 4);
}


// Schedule to send a carousel
//
crsl_schedule (crsl_t *crsl, out_session_t* ses)
{
    que_elem_t *elem;

    FOR_ALL_ELEMENTS_IN_QUE(&crsl->tp_list, elem) {
        // Set up output command
        out_cmd = out_command_setup(ses->qam_id, ses->flow_id, out_time,
                                    (uint32)(elem + 1));

        out_cmd->cc_restamp_flag = 1;
        out_cmd->cc = crsl->next_cc++;
        crsl->next_cc &= 0xF;
    }

    crsl->next_send_time += send_period;
}


/****************************************************************************/

// Regenerate output PAT
//
int regenerate_pat (out_session_t *out_ses)
{
    int i;
    pat_info_t* in_pat = &out_ses->in_ses->pat;
    pid_table_t* pid_tab = out_ses->pid_tab;
    pid_info_t* pid_info = &pid_tab->pid_info[pid_tab->pri_size];

    switch (out_ses->in_ses->mode) {

    case SESSION_MODE_SPTS_MUX:
        assert(in_pat->prog_cnt == 1);
        // Deliberately falls through here!

    case SESSION_MODE_MPTS_REMUX:
        pid_info_set(pid_info++, PAT_PID, PID_FLAG_USED | PID_FLAG_PAT);

        for (i=0; i<in_pat->prog_cnt; i++) {
            // Allocate output program for each program in MPTS
            out_prog = out_prog_alloc(out_ses->prog_num + i);
            assert(out_prog);
            que_put(NULL, &out_ses->prog_list, &out_prog->link);

            // Add PMT PID in secondary PID table
            pid_info_set(pid_info++, out_prog->pmt_pid,
                         PID_FLAG_USED | PID_FLAG_PMT);

            // Add pointer to the corresponding in program
            in_pmt_pid = in_pat->prog_info[i]->pmt_pid;
            out_prog->in_prog = in_prog_lookup_by_pmt_pid(
                                    &out_ses->in_ses->prog_list, in_pmt_pid);
            assert(out_prog->in_prog);
        }

        pid_tab->sec_pid_cnt = in_pat->prog_cnt + 1;

        break;

    case SESSION_MODE_MPTS_PASSTHRU:
    case SESSION_MODE_DATA_INJECT:
        // TBD
        printf("Error: Session mode %d not supported yet\n", mode);
        return ENOTSUP;

    default:
        printf("Error: Unknown session mode %d\n", mode);
        return EINVAL;
    }

    return EOK;
}


// Regenerate output PMT
//
int regenerate_pmt (out_session_t *out_ses, uint16 pmt_pid)
{
    out_prog_t* out_prog = out_prog_lookup_by_pmt_pid(out_ses->prog_list,
                                                      pmt_pid);
    assert(out_prog);

    // Allocate PSI section for output PMT (PMT must have only 1 section)
    out_prog->pmt.sect = (psi_section_t*)pool_alloc(&psi_section_pool);
    if (!out_prog->pmt.sect) {
        printf("Error: Failed to allocate memory for PSI section\n");
        exit (EXIT_FAILURE);
    }

    // Copy input PMT section to output
    psi_section_copy(out_prog->pmt.sect, out_prog->in_prog->pmt.sect);

    // Parse output PMT
    parse_pmt(&out_prog->pmt, out_prog->pmt.sect);

    // Modify output PMT
    pmt_modify(out_prog->pmt, out_prog->prog_num, out_prog->pmt_ver,
               out_ses->pid_tab);

    // Calculate CRC32 for output PMT section
    crc = crc32(out_prog->pmt.sect->sect_hdr,
                out_prog->pmt.sect->sect_hdr.sect_len - 4);
    psi_section_set_crc(out_prog->pmt.sect, crc);

    // Generate PMT carousel
    que_init(NULL, &out_prog->pmt_crsl.tp_list);
    out_prog->pmt_crsl.num_tp = psi_section_packetize(out_prog->pmt.sect,
                                        pmt_pid, &out_prog->pmt_crsl.tp_list);

    // Add PIDs from output PMT to PID table
    pid_table_add_pids_from_pmt(out_ses->pid_tab, out_ses->pmt);

    // Schedule PMT carousel right away
    crsl_schedule(out_ses->pmt_crsl, out_ses);
}

