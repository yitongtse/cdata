// Copyright (c) 2012-2015 by cisco Systems, Inc.
// All rights reserved
//

#ifndef __VIDEO_CONFIG_DB_H__
#define __VIDEO_CONFIG_DB_H__



int video_db_init(void);

void video_db_state_recovery(void);

int video_db_lcred_checkin(uint32_t mode, uint32_t role, int32_t primary_id);

err_t video_db_in_cnfg_checkin(
          uint32_t slot_id, uint32_t in_id, in_config_t *in_cnfg_p);
err_t video_db_in_cnfg_delete(uint32_t slot_id, uint32_t in_id);
void video_db_in_cnfg_diag(FILE *fp);

err_t video_db_out_cnfg_checkin(
          uint32_t slot_id, uint32_t out_id, out_config_t *out_cnfg_p);
err_t video_db_out_cnfg_delete(uint32_t slot_id, uint32_t out_id);
void video_db_out_cnfg_diag(FILE *fp);

err_t video_db_qam_cnfg_checkin(
          uint32_t slot_id, uint32_t qam_id, qam_config_t *qam_cnfg_p);
err_t video_db_qam_cnfg_delete(uint32_t slot_id, uint32_t qam_id);
void video_db_qam_cnfg_diag(FILE *fp);

err_t video_db_crsl_cnfg_checkin(
          video_context_t *ctx, uint32_t crsl_id, crsl_t *crsl_p);
err_t video_db_crsl_cnfg_delete(video_context_t *ctx, uint32_t crsl_id);
void video_db_crsl_cnfg_diag(FILE *fp);

err_t video_db_crsl_tp_checkin(video_context_t *ctx, crsl_t *crsl_p);
err_t video_db_crsl_tp_delete(video_context_t *ctx, crsl_t *crsl_p);
void video_db_crsl_tp_diag(FILE *fp);


#endif /* __VIDEO_CONFIG_DB_H__ */
