/*
 * Copyright (c) 2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "video.h"


void psi_record_init (psi_record_t *rec, const char *filename, int max_tp)
{
    assert(strlen(filename) < 128);
    strcpy(rec->filename, filename);

    rec->fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    if (rec->fd == -1) {
        return;
    }

    rec->in_id = INVALID_SES_ID;
    rec->max_tp = max_tp;
    rec->tp_cnt = 0;
    rec->rec_flag = FALSE;
}


void psi_record_start (psi_record_t *rec, uint16 in_id)
{
    rec->in_id = in_id;
    rec->rec_flag = TRUE;
}


void psi_record_status (psi_record_t *rec, FILE *fp)
{
    if (rec->fd == -1) {
        fprintf(fp, "Recording not set up\n");

    } else {
        fprintf(fp, "Recording PSI to %s for up to %d TPs\n",
                rec->filename, rec->max_tp);

        if (rec->in_id != INVALID_SES_ID) {
            fprintf(fp, "  Status %s: %d TPs recorded from IN %d\n",
                (rec->rec_flag ? "ON" : "OFF"), rec->tp_cnt, rec->in_id);
        }
    }
}


void psi_record_tp (psi_record_t *rec, uint16 in_id, tp_header_t *tp_hdr)
{
    if (rec->rec_flag && rec->in_id == in_id && rec->tp_cnt < rec->max_tp) {
        int n = write(rec->fd, tp_hdr, TP_SIZE);
        if (n != TP_SIZE) {
            VIDEO_DEBUG("Failed saving PSI TP: %d bytes written (errno %d)\n",
                        n, errno);
            rec->rec_flag = FALSE;
        }
        if (++rec->tp_cnt == rec->max_tp)  rec->rec_flag = FALSE;
    }
}


