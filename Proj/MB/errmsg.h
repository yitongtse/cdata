// QNX APIs
//
#define LC_ERRMSG_THREAD_CREATE                 "%s: Failed to create %s thread: %s"
#define LC_ERRMSG_DISPATCH_CREATE               "%s: Failed to create dispatch: %s"
#define LC_ERRMSG_DISPATCH_CONTEXT_ALLOC        "%s: Failed to allocate dispatch context: %s"
#define LC_ERRMSG_PULSE_ATTACH                  "%s: Failed to attach pulse %s: %s"
#define LC_ERRMSG_MEM_OFFSET                    "%s: Failed to create timer: %s"


// LC-RED
//
#define LC_ERRMSG_LCRED_MODE_ROLE               "%s: Bad mode-role combinate: %s %s"
#define LC_ERRMSG_LCRED_GO_HOT                  "%s: Unexpected go hot message"


// Common utils
//
#define LC_ERRMSG_IPC_INIT                      "%s: Failed to initialize IPC"
#define LC_ERRMSG_LOGGER_OPEN                   "%s: Failed to open logger %s"


// Video specific
//
#define LC_ERRMSG_VID_TLV_INCOMPLETE            "%s: Incomplete TLV"
#define LC_ERRMSG_VID_TLV_UNKNOWN               "%s: Unknown TLV type %d"
#define LC_ERRMSG_VID_TLV_MISSING               "%s: Missing TLV %s"
#define LC_ERRMSG_VID_MSG_TRUNCATED             "%s: Video response truncated"

#define LC_ERRMSG_VID_CONTEXT_INIT		"%s: Failed to set up video context %d"
#define LC_ERRMSG_VID_CONTEXT_CHANGE		"%s: Cannot change video context"

#define LC_ERRMSG_VID_IN_ALLOC                  "Failed to allocate in session"
#define LC_ERRMSG_VID_IN_INIT                   "%s: Failed to set up in session %d"
#define LC_ERRMSG_VID_IN_NOT_USED               "%s: In session %d not used"
#define LC_ERRMSG_VID_IN_BUSY                   "%s: In %d has out sessions configured"

#define LC_ERRMSG_VID_OUT_ALLOC                 "Failed to allocate out session"
#define LC_ERRMSG_VID_OUT_INIT                  "%s: Failed to set up out session %d"
#define LC_ERRMSG_VID_OUT_NOT_USED              "%s: Out session %d not used"

#define LC_ERRMSG_VID_CRSL_ALLOC                "Failed to allocate carousel"
#define LC_ERRMSG_VID_CRSL_NOT_USED             "%s: Carousel %d not used"

#define LC_ERRMSG_VID_BAD_DST_IP                "In %d: Bad dest IP %s"
#define LC_ERRMSG_VID_BAD_MCAST_GRP_IP          "In %d: Bad multicast group address %08x"
#define LC_ERRMSG_VID_BAD_PRIMARY_ID            "%s: Bad primary ID %d"
#define LC_ERRMSG_VID_BAD_IN_PORT               "%s: Bad input port %d"
#define LC_ERRMSG_VID_BAD_IN_TYPE               "%s: Bad input type: %d"
#define LC_ERRMSG_VID_BAD_ENABLE                "%s: Bad ENABLE value %d"
#define LC_ERRMSG_VID_BAD_IN_ID                 "%s: Bad in session ID %d"
#define LC_ERRMSG_VID_BAD_OUT_ID                "%s: Bad out session ID %d"
#define LC_ERRMSG_VID_BAD_QAM_ID                "%s: Bad QAM id %d"
#define LC_ERRMSG_VID_BAD_CRSL_ID               "%s: Bad carousel ID %d"
#define LC_ERRMSG_VID_BAD_CRSL_TP_IDX           "%s: Bad carousel TP index %d"

#define LC_ERRMSG_VID_PRIMARY_ID_CHANGE         "Cannot change primary id"
#define LC_ERRMSG_VID_INPUT_TYPE_CHANGE         "%s: Cannot change input type"
#define LC_ERRMSG_VID_SRC_IP_CHANGE             "%s: Cannot change source IP"
#define LC_ERRMSG_VID_DST_IP_CHANGE             "%s: Cannot change dest IP"
#define LC_ERRMSG_VID_SRC_UDP_CHANGE            "%s: Cannot change source UDP"
#define LC_ERRMSG_VID_DST_UDP_CHANGE            "%s: Cannot change dest UDP"
#define LC_ERRMSG_VID_CRSL_NUM_TP_CHANGE        "%s: Cannot change number of TPs in carousel"

#define LC_ERRMSG_VID_DST_UDP_USED              "In %d: Dest UDP %d already used"
#define LC_ERRMSG_VID_ASM_SRC_IP                "In %d: Unexpected source IP %s for ASM"
#define LC_ERRMSG_VID_MCAST_PORT                "Mcast port (%08x, %08x) not provisioned"

#define LC_ERRMSG_VID_PORT_BUSY                 "%s: Cannot reprovision port with existing sessions"
#define LC_ERRMSG_VID_DST_IP_NOT_PROV           "In %d: Dest IP %s not provisioned for input port %d"

#define LC_ERRMSG_VID_PROG_NUM_MISS             "%s: Missing program number for PID remap session"
#define LC_ERRMSG_VID_PROG_NUM_PASSTHRU         "%s: Unexpected program number for passthru session"

#define LC_ERRMSG_VID_FLOW_TAB_CORRUPT          "Out %d: Corrupted flow table"
#define LC_ERRMSG_VID_PROG_NUM_RESERVED         "%s: Cannot use reserved program number 0"

#define LC_ERRMSG_VID_CRSL_QAM                  "%s: Carousel %d not inserted in QAM %d"

#define LC_ERRMSG_VID_PSI_SECT_ALLOC            "Failed to allocate PSI section"
#define LC_ERRMSG_VID_PSI_TP_ALOC               "Failed to allocate PSI TP buffer"
#define LC_ERRMSG_VID_PMT_SECT_ALOC             "Out %d: Failed to allocate PMT section"
#define LC_ERRMSG_VID_FLOW_ALLOC                "Out %d: No flow available in qam %d"
#define LC_ERRMSG_VID_DMA_DESC_FULL             "DMA desc buffer full"

#define LC_ERRMSG_VID_PSI_SECT_TOO_BIG          "Oversized PSI section (%d bytes)"
#define LC_ERRMSG_VID_PSI_SECT_NUM              "In %d: Section number (%d) > last section number (%d)"

#define LC_ERRMSG_VID_PAT_SECT_EXCEEEDED        "In %d: PAT has too many sections (%d)"
#define LC_ERRMSG_VID_PAT_BAD_TABLE             "In %d: Illegal table id %d found in PAT"
#define LC_ERRMSG_VID_PAT_CRC                   "In %d: CRC error in PAT"
#define LC_ERRMSG_VID_PAT_NIT_EXCEEDED          "Input PAT has more than one NIT PID"
#define LC_ERRMSG_VID_PAT_PROG_EXCEEDED         "In %d: Too many programs (%d) in PAT"

#define LC_ERRMSG_VID_PROG_UNKNOWN              "In %d: PMT with unknown program number %d"
#define LC_ERRMSG_VID_PMT_SECT_EXCEEDED         "In %d: PMT has Too many sections (%d)"
#define LC_ERRMSG_VID_PMT_CRC                   "In %d: CRC error in PMT"
#define LC_ERRMSG_VID_BAD_PROG_DESC             "Bad descriptor in program info loop"
#define LC_ERRMSG_VID_BAD_ES_DESC               "Bad descriptor in ES loop"
#define LC_ERRMSG_VID_ES_EXCEEDED               "Too many ES (%d) in program"
#define LC_ERRMSG_VID_CA_DESC_EXCEEDED          "Too many CA descriptors in program"
#define LC_ERRMSG_VID_IN_PMT_BLOCK              "In %d: blocked due to PMT problem"
#define LC_ERRMSG_VID_PMT_NOT_FOUND             "In %s: PMT PID %08x not found"

#define LC_ERRMSG_VID_PROG_EXCEEDED             "Qam %d: Too many output programs (%d)"
#define LC_ERRMSG_VID_PID_EXCEEDED              "Total PID count exceeded: ES %d, CAS %d"
#define LC_ERRMSG_VID_PAT_BUILD                 "Qam %d: Failed to build PAT"
#define LC_ERRMSG_VID_PMT_BUILD                 "Out %d: Failed to build PMT for program %d"

#define LC_ERRMSG_VID_PROG_NUM_EXIST            "%s: Out program number %d already used in qam %d"
#define LC_ERRMSG_VID_PROG_EXIST                "Out %d: Program %d already exists in QAM %d"
#define LC_ERRMSG_VID_PMT_PID_EXIST             "Out %d: PMT pid %d already exists in QAM %d"
#define LC_ERRMSG_VID_PID_EXIST                 "Out %d: Pid %d already exists in QAM %d"

