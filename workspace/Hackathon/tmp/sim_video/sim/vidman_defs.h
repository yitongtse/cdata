/*
 *------------------------------------------------------
 * vidman_defs.h - definitions for common
 *                 VIDEO related parameters
 *
 * August 2014
 *
 * Copyright (c) 2014-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------
 */

#ifndef __VIDMAN_DEFS_H__
#define __VIDMAN_DEFS_H__

#define VIDEO_INPUT_MAX_SOURCES (3)

#define VIDEO_CHANNEL_PER_PORT  (158)
#define VIDEO_PORT_PER_CLC (8)
#define VIDEO_MAX_SESSIONS_PER_QAM (40)
#define VIDEO_MAX_CHANNELS_PER_CLC (768)
#define VIDEO_INVALID_QAM_ID (-1)
#define VIDEO_MAX_CAROUSEL_INSERT_PER_LC (512)
#define VIDEO_MAX_CAROUSEL_TP_PER_LC (1024)
#define VIDEO_MAX_PID_FILTERS  (32)
#define VIDEO_MAX_PROG_FILTERS (16)
#define VIDEO_MAX_FILTERS      (VIDEO_MAX_PID_FILTERS)
#define VIDEO_MAX_PID_MAP      (256)

typedef struct vidman_lc_location_t_ {
    uint16_t slot;
    uint16_t bay;
} vidman_lc_location_t;

/*
 * vidman_qam_location_t - Structure that holds the slot, port and rf channel of a
 *   QAM carrier configured in the system.
 */
typedef struct vidman_qam_location_t_ {
    uint16_t slot;
    uint16_t bay;
    uint16_t port;
    uint16_t channel;
} vidman_qam_location_t;

/*
 * Useful macro
 */
#define VIDMAN_DUMP_RAW_DATA_IN_HEX(header, line_header, data, length)\
{\
    uint ii = 0;\
    printf("%s: Length: %d\n", header, length);\
    if (length > 0) {\
        printf("%s0x%04x: ", line_header, ii);\
    }\
    for (ii = 0; ii < length; ii++) {\
        if (!((ii+1) % 16)) {\
            printf("%02x\n", data[ii]);\
            if ((ii+1) < length) {\
                printf("%s0x%04x: ", line_header, ii+1);\
            }\
        } else {\
            printf("%02x ", data[ii]);\
        }\
    }\
   printf("\n");\
}

#endif /* __VIDMAN_DEFS_H__ */
