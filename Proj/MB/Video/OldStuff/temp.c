// Code to add a in session to a QAM (Configuration phase)
    out_session_t *ses1 = NULL;
    out_session_t *ses2 = NULL;

    out_ses = out_session_alloc();
    out_session_init(out_ses, in_ses);
    qam_add_out_session(qam, out_ses);

    // Check for output program number collision
    ses1 = qam_lookup_alloc_prog_num(qam, prog_num_start);
    if (max_prog_cnt > 1) {
        ses2 = qam_lookup_alloc_prog_num(qam, prog_num_start+max_prog_cnt-1);
    }
    if ((ses1) || (ses2)) {
        printf("Error: output program number collision\n");
        return -1;
    }

    // Output program number allocation
    out_ses->prog_num = prog_num_start;
    out_ses->max_prog_cnt = max_prog_cnt;

// PSI regeneration




// Return the first PID assigned to a program number
//   We use a fixed relationship between PIDs and program number.
//   PID 0 to 47 are reserved.
//   Each program has 32 PIDs reserved.
//
uint16 get_pid_by_prog_num (int prog_num)
{
    return 0x30 + prog_num * 32;
}


// Find allocated output program number in a list of out session
//
out_session* qam_lookup_by_alloc_prog_num (que_elem_t *ses_list, int prog_num)
{
    out_session_t *ses;

    FOR_ALL_ELEMENTS_IN_QUE(ses_list, (que_elem_t*)ses) {
        if (prog_num >= ses->prog_num &&
            prog_num < ses->prog_num + ses->max_prog_cnt) {
            return ses;
        }
    }
    return NULL;
}


// Add out session to QAM
//
int qam_add_out_session (qam_info_t *qam, out_session_t *out_ses)
{
    int i;

    // Search for unused flow in the QAM
    for (i=0; i<MAX_OUT_SESSIONS_PER_QAM; i++) {
        if (!flow_tab[i]) {
            break;
        }
    }
    if (i >= MAX_OUT_SESSIONS_PER_QAM) {
        printf("Error: No available flows in the QAM\n");
        return ENOMEM;
    }

    // Set up cross references
    out_ses->flow_id = i;
    qam->flow_tab[i] = out_ses;

    // Add out session to the QAM's session list
    que_put(NULL, qam->ses_list, &out_ses->link);

    out_ses->qam_id = qam_id;
}


// Process output PAT
//   When an new input PAT session is snooped, the corresponding TP info
//   will be flagged as new PAT.
//
int process_out_pat (out_session_t *out_ses)
{
    int i;

    // Generate out program for each in prog
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


// Regenerate output PMT
//
int regenerate_pmt (tp_info_t *tp_info, out_session_t *out_ses)
{
    pmt_pid = out_ses->pid_tab->pid_info[tp_info->pid_idx];
    out_prog = out_prog_lookup_by_pmt_pid(out_ses->prog_list, pmt_pid);
    assert(out_prog);

    // Allocate PSI section for output PMT (PMT must have only 1 section)
    out_prog->pmt.sect = (psi_section_t*)pool_alloc(&psi_section_pool);
    if (!out_prog->pmt.sect) {
        printf("Error: Failed to allocate memory for PSI section\n");
        exit (EXIT_FAILURE);
    }

    // Copy input PMT section to output
    psi_section_copy(out_prog->pmt.sect, out_prog->in_prog->pmt.sect);

    // Remap output PID values in output PMT
    out_prog_pid_remap(out_prog, out_ses->pid_tab);

    // Calculate CRC32 for output PMT section
    crc = crc32(out_prog->pmt.sect->sect_hdr,
                out_prog->pmt.sect->sect_hdr.sect_len - 4);
    psi_section_set_crc(out_prog->pmt.sect, crc);
}


// Remap PIDs of a program
//   Output PIDs are selected based on the given out program number.
//   QAM's output PID table should be checked to avoid PID collision (TBD).
//
void out_prog_pid_remap (out_prog_t* out_prog, pid_table_t* pid_tab)
{
    int i;
    pmt_info_t* pmt = &out_prog->pmt;
    uint16 base_pid = get_pid_by_prog_num(out_prog->prog_num);

    uint16 pcr_pid = 

    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt->es_info[i];
        
    }
}


process_out_psi_tp()
{
}

