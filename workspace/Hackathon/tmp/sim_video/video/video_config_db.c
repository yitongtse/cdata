///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "bench.h"
#include "video.h"
#include "video_messages.h"
#include "video_util.h"
#include "video_cli.h"



int video_db_init (void)
{
    return EOK;
}

void video_db_state_recovery (void)
{
}

int video_db_lcred_checkin (uint32_t mode, uint32_t role, int32_t primary_id)
{
    return EOK;
}

err_t video_db_in_cnfg_checkin (uint32_t slot_id, uint32_t in_sess_id,
                                 in_config_t *in_cnfg_p)
{
    return EOK;
}

err_t video_db_in_cnfg_delete (uint32_t slot_id, uint32_t in_sess_id)
{
    return EOK;
}

err_t video_db_out_cnfg_checkin (uint32_t slot_id, uint32_t out_sess_id,
                                  out_config_t *out_cnfg_p)
{
    return EOK;
}

err_t video_db_out_cnfg_delete (uint32_t slot_id, uint32_t out_sess_id)
{
    return EOK;
}

err_t video_db_qam_cnfg_checkin (uint32_t slot_id, uint32_t qam_id,
                                  qam_config_t *qam_cnfg_p)
{
    return EOK;
}

err_t video_db_qam_cnfg_delete (uint32_t slot_id, uint32_t qam_id)
{
    return EOK;
}

err_t video_db_crsl_cnfg_checkin(video_context_t *ctx, uint32_t crsl_id,
                                 crsl_t *crsl_p)
{
    return EOK;
}

err_t video_db_crsl_cnfg_delete(video_context_t *ctx, uint32_t crsl_id)
{
    return EOK;
}

err_t video_db_crsl_tp_checkin(video_context_t *ctx, crsl_t *crsl_p)
{
    return EOK;
}

err_t video_db_crsl_tp_delete(video_context_t *ctx, crsl_t *crsl_p)
{
    return EOK;
}


// ############################################################################
// ## Debug Utility
// ############################################################################

void video_db_in_cnfg_diag(FILE *fp)
{
}

void video_db_qam_cnfg_diag(FILE *fp)
{
}

void video_db_out_cnfg_diag(FILE *fp)
{
}

void video_db_crsl_cnfg_diag(FILE *fp)
{
}
