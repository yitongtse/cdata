/*
 * Copyright (c) 2008-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __VIDEO_ERRMSG_H__
#define __VIDEO_ERRMSG_H__

/*
 * The following are the functions that will be used by the VIDMAN
 * to generate errmsg for the cbr8 syslog.
 * Any changes to the error msg please update the 
 * the $BINOS_ROOT/cable/video/vdman/src/vidman.em
 */
extern void em_vidman_LOGGER_OPEN_FAILED (void);

extern void em_vidman_LCRED_BAD_MODE_ROLE_TRANSITION (const char *prev_mode,
                                          const char *prev_role,
                                          const char *new_mode,
                                          const char *new_role);

extern void em_vidman_LCRED_UNEXPECTED_GO_HOT (const char *prev_mode,
                                             const char *prev_role);

extern void em_vidman_LCRED_UNEXPECTED_JOINED_GROUP (const char *prev_mode,
                                                   const char *prev_role);

extern void em_vidman_LCRED_UNEXPECTED_LEFT_GROUP (const char *prev_mode,
                                                 const char *prev_role);

extern void em_vidman_VIDEO_CONTEXT_RESET_FAILED (const uint16_t slot,
                                                const char *error);

extern void em_vidman_VIDEO_BAD_OUT_SESSION_ID (const char *oper_label,
                                              const uint32_t id);

extern void em_vidman_VIDEO_BAD_QAM_ID (const char *oper_label,
                                      const uint16_t id);

extern void em_vidman_VIDEO_UNEXPECTED_PROG_NUM (const char *oper_label,
                                               const uint16_t prog_num,
                                               const uint32_t id);

extern void em_vidman_VIDEO_PROG_NUM_IN_USE (const char *oper_label,
                                           const uint16_t prog,
                                           const uint32_t out_id,
                                           const uint16_t qam_id);

extern void em_vidman_VIDEO_MISSING_PROG_NUM (const char *operation,
                                            const uint32_t id);

extern void em_vidman_VIDEO_COMMAND_FAILED (const char *operation,
                                          const uint32_t id, const int32_t rc);

extern void em_vidman_VIDEO_IN_SESSION_INIT_FAILED (const char *operation,
                                                  const uint32_t id,
                                                  const char *error);

extern void em_vidman_VIDEO_OUT_SESSION_INIT_FAILED (const char *operation,
                                                   const uint32_t id,
                                                   const char *error);

extern void em_vidman_VIDEO_SESSION_NOT_USED (const char *operation,
                                            const uint32_t id);

extern void em_vidman_VIDEO_OUT_SESSION_NOT_USED (const char *operation,
                                                const uint32_t id);

extern void em_vidman_VIDEO_INCOMPLETE_MSG (const char *operation);

extern void em_vidman_VIDEO_UNKNOWN_MSG (const char *operation, const uint type);

extern void em_vidman_IPC_SEND_FAILED (const char *dest, const char *err,
                                       const char *rc, const char *ipc_rc);

extern void em_vidman_VIDEO_CAROUSEL_ALLOC_FAILED (const char *operation,
                                                 const uint32_t id);

extern void em_vidman_VIDEO_CAROUSEL_NOT_FOUND (const char *operation,
                                              const uint32_t indx);

extern void em_vidman_VIDEO_FLOW_ALLOC_FAILED (const char *operation,
                                             const uint32_t rid,
                                             const uint16_t qam_id);

extern void em_vidman_VIDEO_BAD_PRIMARY_ID (const char *operation,
                                          const int context_id);

extern void em_vidman_VIDEO_CONTEXT_INIT_FAILED (const char *operation,
                                               const int context_id);

extern void em_vidman_VIDEO_IN_SESSION_ALLOC_FAILED (const char *operation,
                                                   const uint32_t id);

extern void em_vidman_VIDEO_BAD_IN_SESSION_ID (const char *operation,
                                             const uint32_t id);

extern void em_vidman_VIDEO_BAD_INPUT_TYPE (const char *operation,
                                          const int input_type);

extern void em_vidman_VIDEO_BAD_IP_ADDR_LEN (const char *operation,
                                           const int len);

extern void em_vidman_VIDEO_BAD_DST_UDP_PORT (const char *operation,
                                            const uint16_t udp_port);

extern void em_vidman_VIDEO_BAD_JITTER (const char *operation,
                                      const uint jitter);

extern void em_vidman_VIDEO_BAD_DELAY (const char *operation,
                                     const int delay_target);

extern void em_vidman_VIDEO_BAD_CR_MODE (const char *operation, const int mode);

extern void em_vidman_VIDEO_BAD_BITRATE_ALLOC (const char *operation,
                                             const uint32_t bitrate);

extern void em_vidman_VIDEO_BAD_PROG_NUM (const char *operation,
                                        const uint16_t prog_num);

extern void em_vidman_VIDEO_BAD_PID_RANGE (const char *operation,
                                         const uint16_t first_pid,
                                         const uint16_t last_pid);

extern void em_vidman_VIDEO_BAD_INPUT_PORT (const char *operation,
                                            const uint16_t input_port);

extern void em_vidman_VIDEO_CAROUSEL_IN_USE (const char *operation,
                                           const uint16_t crsl_id);

extern void em_vidman_VIDEO_PACKET_INSERT_ALLOC_FAILED (const char *operation,
                                                      const uint16_t crsl_id);

extern void em_vidman_VIDEO_FEATURE_NOT_AVAIL (const char *operation,
                                             const uint32_t out_id);

extern void em_vidman_VIDEO_BAD_FILTERED_PID (const char *operation,
                                            const uint16_t pid,
                                            const uint32_t out_id);

extern void em_vidman_VIDEO_BAD_FILTERED_PROG (const char *operation,
                                             const uint16_t prog,
                                             const uint32_t out_id);

extern void em_vidman_VIDEO_BAD_REMAPPED_PID (const char *operation,
                                            const uint16_t pid,
                                            const uint32_t out_id);

extern void em_vidman_IPC_MALLOC_BUFFER_FAILED (const char *operation,
                                              const char *msg_type);

extern void em_vidman_VIDEO_OUT_PROG_ALLOC_FAILED (const char *operation,
                                                 const uint16_t prog);

extern void em_vidman_VIDEO_PSI_SECTION_ALLOC_FAILED (const char *operation,
                                                    const uint16_t psi_id);

extern void em_vidman_VIDEO_BAD_TABLE_IN_PAT (const uint16_t table_id,
                                            const uint32_t in_id);

extern void em_vidman_VIDEO_TOO_MANY_SECTION_IN_PAT (const uint32_t in_id,
                                                   const uint16_t sect_num);

extern void em_vidman_VIDEO_BAD_PSI_SECTION_NUM (const uint16_t sect_num,
                                               const uint32_t in_id,
                                               const uint16_t last_sect_num);

extern void em_vidman_VIDEO_PSI_CRC_ERROR (const char *operation,
                                           const uint32_t in_id);

extern void em_vidman_VIDEO_TOO_MANY_PROG_IN_PAT (const char *operation,
                                                const uint16_t prog_cnt);

extern void em_vidman_VIDEO_TOO_MANY_NIT_IN_PAT (void);

extern void em_vidman_VIDEO_UNKNOWN_PROG_NUM (const char *operation,
                                            const uint16_t prog_num,
                                            const uint32_t in_id);

extern void em_vidman_VIDEO_TOO_MANY_PMT_SECTIONS (const char *operation,
                                                 const uint32_t in_id,
                                                 const uint16_t sect_num);

extern void em_vidman_VIDEO_TOO_MANY_CA_DESC (const char *operation,
                                            const uint16_t ca_desc_cnt,
                                            const uint16_t max_ca_desc_cnt);

extern void em_vidman_VIDEO_BAD_PROG_DESC (const char *operation);

extern void em_vidman_VIDEO_TOO_MANY_ES (const char *operation,
                                       const uint16_t es_cnt,
                                       const uint16_t max_es_cnt);

extern void em_vidman_VIDEO_BAD_ES_DESC (const char *operation);

extern void em_vidman_VIDEO_PSI_BLOCKED (const char *operation,
                                       const uint32_t in_id,
                                       const char *err_string);

extern void em_vidman_VIDEO_PMT_NOT_FOUND (const char *operation,
                                         const uint16_t pid,
                                         const uint32_t in_id);

extern void em_vidman_VIDEO_TOO_MANY_PROG_IN_QAM (const uint16_t qam_id);

extern void em_vidman_VIDEO_PAT_BUILD_FAILED (const uint16_t qam_id);

extern void em_vidman_VIDEO_PACKET_ALLOC_FAILED (const char *operation);

extern void em_vidman_VIDEO_PROG_CA_BLOCKED (const char *operation,
                                            const uint16_t prog_num, 
                                            const uint16_t qam_id);

#endif /* __VIDEO_ERRMSG_H__ */
