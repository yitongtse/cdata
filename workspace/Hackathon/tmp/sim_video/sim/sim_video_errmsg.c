///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#include <stdio.h>
#include <video.h>

void em_vidman_LOGGER_OPEN_FAILED (void)
{
    printf("%s -> Unable to open video log file.\n", __FUNCTION__);
}

void em_vidman_LCRED_BAD_MODE_ROLE_TRANSITION (const char *prev_mode,
                                          const char *prev_role,
                                          const char *new_mode,
                                          const char *new_role)
{
    printf("%s -> LCRED_BAD_MODE_ROLE_TRANSITION [%s, %s] => [%s, %s]\n",
           __FUNCTION__,
           prev_mode, prev_role,
           new_mode, new_role);
}

void em_vidman_LCRED_UNEXPECTED_GO_HOT (const char *prev_mode,
                                        const char *prev_role)
{
    printf("%s -> LCRED_UNEXPECTED_GO_HOT mode = %s, role = %s\n",
           __FUNCTION__, prev_mode, prev_role);
}

void em_vidman_LCRED_UNEXPECTED_JOINED_GROUP (const char *prev_mode,
                                              const char *prev_role)
{
    printf("%s -> LCRED_UNEXPECTED_JOINED_GROUP mode = %s, role = %s\n",
           __FUNCTION__, prev_mode, prev_role);
}

void em_vidman_LCRED_UNEXPECTED_LEFT_GROUP (const char *prev_mode,
                                            const char *prev_role)
{
    printf("%s -> LCRED_UNEXPECTED_LEFT_GROUP mode = %s, role = %s\n",
           __FUNCTION__, prev_mode, prev_role);
}

void em_vidman_VIDEO_CONTEXT_RESET_FAILED (const uint16_t slot,
                                           const char *error)
{
    printf("%s -> VIDEO_CONTEXT_RESET_FAILED slot %d, error = %s\n",
           __FUNCTION__, slot, error);
}

void em_vidman_VIDEO_BAD_OUT_SESSION_ID (const char *oper_label,
                                         const uint32_t id)
{
    printf("%s -> VIDEO_BAD_OUT_SESSION_ID oper = %s out id = %d\n",
           __FUNCTION__,
           oper_label, id);
}

void em_vidman_VIDEO_BAD_QAM_ID (const char *oper_label,
                                 const uint16_t id)
{
    printf("%s -> VIDEO_BAD_QAM_ID oper = %s, QAM id %d\n",
           __FUNCTION__,
           oper_label, id);
}

void em_vidman_VIDEO_UNEXPECTED_PROG_NUM (const char *oper_label,
                                          const uint16_t prog_num,
                                          const uint32_t id)
{
    printf("%s -> VIDEO_UNEXPECTED_PROG_NUM oper = %s, prog_num = %d, out id = %d\n",
           __FUNCTION__,
           oper_label,
           prog_num,
           id);
}

void em_vidman_VIDEO_PROG_NUM_IN_USE (const char *oper_label,
                                      const uint16_t prog,
                                      const uint32_t out_id,
                                      const uint16_t qam_id)
{
    printf("%s -> VIDEO_PROG_NUM_IN_USE oper = %s, prog_num = %d. "
           "out_id = %d, qam_id = %d\n",
           __FUNCTION__,
           oper_label, prog, out_id, qam_id);
}

void em_vidman_VIDEO_MISSING_PROG_NUM (const char *operation,
                                       const uint32_t id)
{
    printf("%s -> VIDEO_MISSING_PROG_NUM. oper = %s, out_id = %d\n",
           __FUNCTION__, operation, id);
}

void em_vidman_VIDEO_COMMAND_FAILED (const char *operation,
                                     const uint32_t id, const int32_t rc)
{
    printf("%s -> VIDEO_COMMAND_FAILED. oper = %s, resource id = %d, rc = %d\n",
           __FUNCTION__,
           operation, id, rc);
}

void em_vidman_VIDEO_IN_SESSION_INIT_FAILED (const char *operation,
                                             const uint32_t id,
                                             const char *error)
{
    printf("%s -> VIDEO_IN_SESSION_INIT_FAILED. oper = %s, in_id = %d, err = %s\n",
           __FUNCTION__,
           operation, id, error);
}

void em_vidman_VIDEO_OUT_SESSION_INIT_FAILED (const char *operation,
                                              const uint32_t id,
                                              const char *error)
{
    printf("%s -> VIDEO_OUT_SESSION_INIT_FAILED. oper = %s, out_id = %d, err = %s\n",
           __FUNCTION__,
           operation, id, error);
}

void em_vidman_VIDEO_SESSION_NOT_USED (const char *operation,
                                       const uint32_t id)
{
    printf("%s -> VIDEO_SESSION_NOT_USED. oper = %s, session_id = %d\n",
           __FUNCTION__, operation, id);
}

void em_vidman_VIDEO_OUT_SESSION_NOT_USED (const char *operation,
                                           const uint32_t id)
{
    printf("%s -> VIDEO_OUT_SESSION_NOT_USED. oper = %s, out_id = %d\n",
           __FUNCTION__,
           operation, id);
}

void em_vidman_VIDEO_INCOMPLETE_MSG (const char *operation)
{
    printf("%s -> VIDEO_INCOMPLETE_MSG. oper = %s\n", 
           __FUNCTION__, operation);
}

void em_vidman_VIDEO_UNKNOWN_MSG (const char *operation, const uint type)
{
    printf("%s -> VIDEO_UNKNOWN_MSG. oper = %s, msg_type = %d\n",
           __FUNCTION__, operation, type);
}

void em_vidman_IPC_SEND_FAILED (const char *dest, const char *err_msg,
				     const char *rc, const char *ipc_rc)
{
    printf("%s -> IPC_SEND_FAILED. peer proc = %s, err = %s, rc = %s, "
           "ipc status = %s\n",
           __FUNCTION__, dest, err, rc, ipc_rc);
}

void em_vidman_VIDEO_CAROUSEL_ALLOC_FAILED (const char *operation,
                                            const uint32_t id)
{
    printf("%s -> VIDEO_CAROUSEL_ALLOC_FAILED. oper = %s, id = %d\n",
           __FUNCTION__,
           operation, id);
}

void em_vidman_VIDEO_CAROUSEL_NOT_FOUND (const char *operation,
                                         const uint32_t indx)
{
    printf("%s -> VIDEO_CAROUSEL_NOT_FOUND. oper = %s, index = %d\n",
           __FUNCTION__, operation, indx);
}

void em_vidman_VIDEO_FLOW_ALLOC_FAILED (const char *operation,
                                        const uint32_t rid,
                                        const uint16_t qam_id)
{
    printf("%s -> VIDEO_FLOW_ALLOC_FAILED. oper = %s, resource_id = %d, qam_id = %d\n",
           __FUNCTION__,
           operation, rid, qam_id);
}

void em_vidman_VIDEO_BAD_PRIMARY_ID (const char *operation,
                                     const int context_id)
{
    printf("%s -> VIDEO_BAD_PRIMARY_ID. oper = %s, context_id = %d\n",
           __FUNCTION__, operation, context_id);
}

void em_vidman_VIDEO_CONTEXT_INIT_FAILED (const char *operation,
                                          const int context_id)
{
    printf("%s -> VIDEO_CONTEXT_INIT_FAILED. oper = %s, context_id = %d\n",
           __FUNCTION__, operation, context_id);
}

void em_vidman_VIDEO_IN_SESSION_ALLOC_FAILED (const char *operation,
                                              const uint32_t id)
{
    printf("%s -> VIDEO_IN_SESSION_ALLOC_FAILED. oper = %s, in_id = %d\n",
           __FUNCTION__, operation, id);
}

void em_vidman_VIDEO_BAD_IN_SESSION_ID (const char *operation,
                                        const uint32_t id)
{
    printf("%s -> VIDEO_BAD_IN_SESSION_ID. oper = %s, id %d\n",
           __FUNCTION__, operation, id);
}

void em_vidman_VIDEO_BAD_INPUT_TYPE (const char *operation,
                                     const int input_type)
{
    printf("%s -> VIDEO_BAD_INPUT_TYPE. oper = %s, input type = %d\n",
           __FUNCTION__, operation, input_type);
}

void em_vidman_VIDEO_BAD_IP_ADDR_LEN (const char *operation,
                                      const int len)
{
    printf("%s -> VIDEO_BAD_IP_ADDR_LEN. oper = %s, length = %d\n",
           __FUNCTION__, operation, len);
}

void em_vidman_VIDEO_BAD_DST_UDP_PORT (const char *operation,
                                       const uint16_t udp_port) 
{
    printf("%s -> VIDEO_BAD_DST_UDP_PORT. oper = %s, udp_port = %d\n",
           __FUNCTION__,
           operation, udp_port);
}

void em_vidman_VIDEO_BAD_JITTER (const char *operation,
                                 const uint jitter)
{
    printf("%s -> VIDEO_BAD_JITTER. oper = %s, jitter = %d\n",
           __FUNCTION__, operation, jitter);
}

void em_vidman_VIDEO_BAD_DELAY (const char *operation,
                                const int delay_target)
{
    printf("%s -> VIDEO_BAD_DELAY. oper = %s, delay_target = %d\n",
           __FUNCTION__, operation, delay_target);
}

void em_vidman_VIDEO_BAD_CR_MODE (const char *operation, const int mode)
{
    printf("%s -> VIDEO_BAD_CR_MODE. oper = %s, mode = %d\n",
           __FUNCTION__, operation, mode);
}

void em_vidman_VIDEO_BAD_BITRATE_ALLOC (const char *operation,
                                        const uint32_t bitrate)
{
    printf("%s -> VIDEO_BAD_BITRATE_ALLOC. oper = %s, bitrate = %d\n",
           __FUNCTION__, operation, bitrate);
}

void em_vidman_VIDEO_BAD_PROG_NUM (const char *operation,
                                   const uint16_t prog_num)
{
    printf("%s -> VIDEO_BAD_PROG_NUM. oper = %s, prog_num = %d\n",
           __FUNCTION__, operation, prog_num);
}

void em_vidman_VIDEO_BAD_PID_RANGE (const char *operation,
                                    const uint16_t first_pid,
                                    const uint16_t last_pid)
{
    printf("%s -> VIDEO_BAD_PID_RANGE. oper = %s, pid range = %d-%d\n",
           __FUNCTION__, operation, first_pid, last_pid);
}

void em_vidman_VIDEO_BAD_INPUT_PORT (const char *operation,
                                     const uint16_t input_port)
{
    printf("%s -> em_vidman_VIDEO_BAD_INPUT_PORT. oper = %s, input_port = %d\n",
           __FUNCTION__, operation, input_port);
}

void em_vidman_VIDEO_CAROUSEL_IN_USE (const char *operation,
                                      const uint16_t crsl_id)
{
    printf("%s -> VIDEO_CAROUSEL_IN_USE. oper = %s, carousel id = %d\n",
           __FUNCTION__, operation, crsl_id);
}

void em_vidman_VIDEO_PACKET_INSERT_ALLOC_FAILED (const char *operation,
                                                 const uint16_t crsl_id)
{
    printf("%s -> VIDEO_PACKET_INSERT_ALLOC_FAILED. oper = %s, crsl_id = %d\n",
           __FUNCTION__, operation, crsl_id);
}

void em_vidman_VIDEO_FEATURE_NOT_AVAIL (const char *operation,
                                        const uint32_t out_id)
{
    printf("%s -> VIDEO_FEATURE_NOT_AVAIL. oper = %s, out_id = %d\n",
           __FUNCTION__, operation, out_id);
}

void em_vidman_VIDEO_BAD_FILTERED_PID (const char *operation,
                                       const uint16_t pid,
                                       const uint32_t out_id) 
{
    printf("%s -> VIDEO_BAD_FILTERED_PID. oper = %s, pid = %d, out_id = %d\n",
           __FUNCTION__, operation, pid, out_id);
}

void em_vidman_VIDEO_BAD_FILTERED_PROG (const char *operation,
                                        const uint16_t prog,
                                        const uint32_t out_id)
{
    printf("%s -> VIDEO_BAD_FILTERED_PROG. oper = %s, prog = %d, out_id = %d\n",
           __FUNCTION__, operation, prog, out_id);
}

void em_vidman_VIDEO_BAD_REMAPPED_PID (const char *operation,
                                       const uint16_t pid,
                                       const uint32_t out_id)
{
    printf("%s -> VIDEO_BAD_REMAPPED_PID. oper = %s, pid = %d, out_id = %d\n",
           __FUNCTION__, operation, pid, out_id);
}

void em_vidman_IPC_MALLOC_BUFFER_FAILED (const char *operation,
                                         const char *msg_type)
{
    printf("%s -> IPC_MALLOC_BUFFER_FAILED. oper = %s, msg_type = %s\n",
           __FUNCTION__, operation, msg_type);
}

void em_vidman_VIDEO_OUT_PROG_ALLOC_FAILED (const char *operation,
                                            const uint16_t prog)
{
    printf("%s -> VIDEO_OUT_PROG_ALLOC_FAILED. oper = %s, prog = %d\n",
          __FUNCTION__, operation, prog);
}

void em_vidman_VIDEO_PSI_SECTION_ALLOC_FAILED (const char *operation,
                                               const uint16_t psi_id)
{
    printf("%s -> VIDEO_PSI_SECTION_ALLOC_FAILED. oper = %s, psi_id = %d\n",
           __FUNCTION__, operation, psi_id);
}

void em_vidman_VIDEO_BAD_TABLE_IN_PAT (const uint16_t table_id,
                                       const uint32_t in_id)
{
    printf("%s -> VIDEO_BAD_TABLE_IN_PAT. table_id = %d, in_id = %d\n",
           __FUNCTION__, table_id, in_id);
}

void em_vidman_VIDEO_TOO_MANY_SECTION_IN_PAT (const uint32_t in_id,
                                              const uint16_t sect_num)
{
    printf("%s -> VIDEO_TOO_MANY_SECTION_IN_PAT. in_id = %d, sect_num = %d\n",
           __FUNCTION__, in_id, sect_num);
}

void em_vidman_VIDEO_BAD_PSI_SECTION_NUM (const uint16_t sect_num,
                                          const uint32_t in_id,
                                          const uint16_t last_sect_num)
{
    printf("%s -> VIDEO_BAD_PSI_SECTION_NUM. sect_num = %d, in_id = %d, last_sect_num = %d\n",
           __FUNCTION__, sect_num, in_id, last_sect_num);
}

void em_vidman_VIDEO_PSI_CRC_ERROR (const char *operation,
                                    const uint32_t in_id)
{
    printf("%s -> em_vidman_VIDEO_PSI_CRC_ERROR. oper = %s, in_id = %d\n",
           __FUNCTION__, operation, in_id);
}

void em_vidman_VIDEO_TOO_MANY_PROG_IN_PAT (const char *operation,
                                           const uint16_t prog_cnt)
{
    printf("%s -> em_vidman_VIDEO_TOO_MANY_PROG_IN_PAT. oper = %s prog count = %d\n",
           __FUNCTION__, operation, prog_cnt);
}

void em_vidman_VIDEO_TOO_MANY_NIT_IN_PAT (void)
{
    printf("%s -> VIDEO_TOO_MANY_NIT_IN_PAT\n", __FUNCTION__);
}

void em_vidman_VIDEO_UNKNOWN_PROG_NUM (const char *operation,
                                       const uint16_t prog_num,
                                       const uint32_t in_id)
{
    printf("%s -> VIDEO_UNKNOWN_PROG_NUM. oper = %s, prog_num = %d, in_id = %d\n",
           __FUNCTION__, operation, prog_num, in_id);
}

void em_vidman_VIDEO_TOO_MANY_PMT_SECTIONS (const char *operation,
                                            const uint32_t in_id,
                                            const uint16_t sect_num)
{
    printf("%s -> VIDEO_TOO_MANY_PMT_SECTIONS. oper = %s, in_id = %d, sect_num = %d\n",
           __FUNCTION__, operation, in_id, sect_num);
}

void em_vidman_VIDEO_TOO_MANY_CA_DESC (const char *operation,
                                       const uint16_t ca_desc_cnt,
                                       const uint16_t max_ca_desc_cnt)
{
    printf("%s -> VIDEO_TOO_MANY_CA_DESC. oper = %s, ca_desc_cnt = %d, max_ca_desc_cnt = %d\n",
           __FUNCTION__, 
           operation, ca_desc_cnt, max_ca_desc_cnt);
}

void em_vidman_VIDEO_BAD_PROG_DESC (const char *operation)
{
    printf("%s -> VIDEO_BAD_PROG_DESC. oper = %s\n", 
           __FUNCTION__, operation);
}

void em_vidman_VIDEO_TOO_MANY_ES (const char *operation,
                                  const uint16_t es_cnt,
                                  const uint16_t max_es_cnt)
{
    printf("%s -> VIDEO_TOO_MANY_ES. oper = %s, es_cnt = %d, max_es_cnt = %d\n",
           __FUNCTION__, operation, es_cnt, max_es_cnt);
}

void em_vidman_VIDEO_BAD_ES_DESC (const char *operation)
{
    printf("%s -> VIDEO_BAD_ES_DESC. oper = %s\n", 
           __FUNCTION__, operation);
}

void em_vidman_VIDEO_PSI_BLOCKED (const char *operation,
                                  const uint32_t in_id,
                                  const char *err_string)
{
    printf("%s -> VIDEO_PSI_BLOCKED. oper = %s, in_id = %d, err = %s\n",
           __FUNCTION__, operation, in_id, err_string);
}

void em_vidman_VIDEO_PMT_NOT_FOUND (const char *operation,
                                    const uint16_t pid,
                                    const uint32_t in_id)
{
    printf("%s -> VIDEO_PMT_NOT_FOUND. oper = %s, pid = %d, in_id = %d\n",
           __FUNCTION__, operation, pid, in_id);
}

void em_vidman_VIDEO_TOO_MANY_PROG_IN_QAM (const uint16_t qam_id)
{
    printf("%s -> VIDEO_TOO_MANY_PROG_IN_QAM. qam_id = %d\n",
           __FUNCTION__, qam_id);
}

void em_vidman_VIDEO_PAT_BUILD_FAILED (const uint16_t qam_id)
{
    printf("%s -> VIDEO_PAT_BUILD_FAILED. qam_id = %d\n",
           __FUNCTION__, qam_id);
}

void em_vidman_VIDEO_PACKET_ALLOC_FAILED (const char *operation)
{
   printf("%s -> VIDEO_PACKET_ALLOC_FAILED. oper = %s\n",
          __FUNCTION__, operation);
}

void em_vidman_VIDEO_PROG_CA_BLOCKED (const char *operation,
                                      const uint16_t prog_num,
                                      const uint16_t qam_id)
{
   printf("VIDEO_PROC_CA_BLOCKED: oper %s, prog_num %d, qam %d\n",
          operation, prog_num, qam_id);
}


