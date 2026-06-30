/*
 * Copyright (c) 2007-2008, 2010-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

/*****************************************************************************
TP timing info are recorded in the following format:
    uint8 tp_idx_inc                    // bit 7: new_arvl_flag
                                        // bit 6: pcr_flag
                                        // bit 0-5: TP index increment
    if (new_arvl_flag) {
        uint32 arvl_time
    }
    if (TP index increment == 0) {
        uint16 TP index increment       // escape sequence for large value
    }
    if (pcr_flag) {
        uint32 pcr_base
        uint16 pcr_ext                  // bit 0-9: PCR extension (0 to 599)
                                        // bit 10-14: upper 5 bits of pcr pid
                                        // bit 15: disc_flag
        uint8 pid                       // lower 8 bits of pcr pid
    }
*****************************************************************************/

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "video.h"


#define RECORD_PCR_ONLY                 1


// Global variables
char rec_filename[128];
uint32 rec_in_sid;
int rec_fd = -1;
uint32 rec_max_size;
uint32 rec_size;
boolean rec_on = FALSE;
boolean rec_first_tp;
uint32 rec_prev_arvl_time;
uint32 rec_prev_tp_idx;
time_t rec_start_time;
time_t rec_stop_time;
uint32 rec_start_tp_idx;


void clkrec_record_init (FILE *fp, char *filename, int max_size, int in_sid)
{
    rec_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    if (rec_fd == -1) {
        fprintf(fp, "Error: Failed to open file %s for recording\n", filename);
        return;
    }
    assert(strlen(filename) < 128);
    strcpy(rec_filename, filename);
    rec_in_sid = in_sid;
    rec_max_size = max_size;
    rec_on = FALSE;
}


void clkrec_record_start (void)
{
    rec_on = TRUE;
    rec_first_tp = TRUE;
    rec_start_time = time(NULL);
    rec_start_tp_idx = rec_prev_tp_idx = 0;
    rec_size = 0;
}


void clkrec_record_stop (void)
{
    rec_on = FALSE;
    rec_stop_time = time(NULL);
}


void clkrec_record_status (FILE *fp)
{
    if (rec_fd == -1) {
        fprintf(fp, "Recording not set up\n");
    } else {
        time_t end_time = rec_on? time(NULL) : rec_stop_time;
        uint32 sec = difftime(end_time, rec_start_time);
        uint32 min = sec / 60;
        sec -= min * 60;

        fprintf(fp, "Recording in session %d to %s for up to %d bytes\n",
                rec_in_sid, rec_filename, rec_max_size);
        fprintf(fp, "Status %s, %d bytes, %d TPs, duration %d:%02d\n",
                (rec_on? "ON":"OFF"), rec_size,
                (int32)(rec_prev_tp_idx - rec_start_tp_idx), min, sec);
    }
}


void clkrec_record_tp (uint32 tp_idx, uint16 pid, uint32 arvl_time,
                       boolean pcr_flag, mpeg_clock_t *pcr, boolean disc_flag)
{
    uint8 new_arvl_flag;
    int32 tp_idx_inc;
    uint8 flags;
    uint16 large_inc;

#if RECORD_PCR_ONLY
    if (!pcr_flag) {
        return;
    }
#endif

    if (rec_first_tp) {
        new_arvl_flag = 1;
        rec_first_tp = FALSE;
        tp_idx_inc = 1;
        rec_start_tp_idx = tp_idx;

    } else {
        new_arvl_flag = (arvl_time != rec_prev_arvl_time);
        tp_idx_inc = (int32)(tp_idx - rec_prev_tp_idx);
    }

    flags = (new_arvl_flag << 7) | (pcr_flag << 6);
    large_inc = (tp_idx_inc & 0xffffffc0);
    if (!large_inc) {
        flags |= tp_idx_inc;
    }

    rec_size += write(rec_fd, &flags, 1);

    if (new_arvl_flag) {
        uint32 temp = htonl(arvl_time);
        rec_size += write(rec_fd, &temp, sizeof(uint32));
    }

    if (large_inc) {
        uint16 temp = htons((uint16)tp_idx_inc);
        rec_size += write(rec_fd, &temp, sizeof(uint16));
    }

    if (pcr_flag) {
        uint8 temp = pid & 0xFF;
        uint16 pcr_ext = (uint16)pcr->ext;
        pcr_ext |= disc_flag << 15;
        pcr_ext |= (pid & 0x1F00) << 2;
        uint32 temp2 = htonl(pcr->base);
        uint16 temp3 = htons(pcr_ext);
        rec_size += write(rec_fd, &temp2, sizeof(uint32));
        rec_size += write(rec_fd, &temp3, sizeof(uint16));
        rec_size += write(rec_fd, &temp, sizeof(uint8));
    }

    if (rec_size > rec_max_size) {
        rec_on = FALSE;
        rec_stop_time = time(NULL);
        VIDEO_LOG("Timing info size reached");
        close(rec_fd);
    }

    // Update
    rec_prev_tp_idx = tp_idx;
    rec_prev_arvl_time = arvl_time;
}

