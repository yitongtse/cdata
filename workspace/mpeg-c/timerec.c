/*
 * Copyright (c) 2006-2014 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *   timerec.c - Utilities used for TP time info
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "common.h"
#include "video.h"
#include "video_util.h"
#include "timerec.h"
#include "endian.h"


// Return 1 if end of file reached.  Otherwise returns 0.
//
int tp_time_info_read (FILE* fp, tp_time_info_t *info)
{
    uint8 hdr;
    uint8 new_arvl_flag;
    uint8 tmp;

    if (fread(&hdr, 1, 1, fp) < 1) {
        return 1;
    }

    new_arvl_flag = hdr >> 7;
    if (new_arvl_flag) {
        fread(&info->arvl_time, 4, 1, fp);
        info->arvl_time = ENDIAN_SWAP32(info->arvl_time);
    }

    info->tp_idx_inc = hdr & 0x3f;
    if (!info->tp_idx_inc) {
        // tp_idx_inc escape
        fread(&info->tp_idx_inc, 2, 1, fp);
        info->tp_idx_inc = ENDIAN_SWAP16(info->tp_idx_inc);
    }
    info->tp_idx += info->tp_idx_inc;

    info->pcr_flag = (hdr >> 6) & 1;
    if (info->pcr_flag) {
        fread(&info->pcr.base, 4, 1, fp);
        info->pcr.base = ENDIAN_SWAP32(info->pcr.base);
        fread(&info->pcr.ext, 2, 1, fp);
        info->pcr.ext = ENDIAN_SWAP16(info->pcr.ext);
        fread(&tmp, 1, 1, fp);

        info->disc_flag = (info->pcr.ext >> 15) & 1;
        info->pid = ((info->pcr.ext & 0x7c00) >> 2) | tmp;
        info->pcr.ext &= 0x3ff;
    }

    return 0;
}


void tp_time_info_print (tp_time_info_t *info)
{
    printf("TP %d (inc %d): AT %08x",
           info->tp_idx, info->tp_idx_inc, info->arvl_time);
    if (info->pcr_flag) {
        printf(", PCR %08x:%d", info->pcr.base, info->pcr.ext);
        if (info->disc_flag) {
            printf(", disc");
        }
        printf(", pid %d", info->pid);
    }
}

