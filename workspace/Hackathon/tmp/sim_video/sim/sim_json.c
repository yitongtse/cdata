///  Copyright (c) 2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <stdlib.h>
#include "common.h"
#include "crc.h"
#include "video.h"
#include "video_util.h"
#include "sim_video.h"


void pid_stat_convert_json (pid_stat_t *stat, char *buf)
{
    int i;
    int len;
    char* ptr = buf;

    len = snprintf(ptr, 100, "[ ");
    ptr += len;

    for (i=0; i<NUM_PIDS; i++, stat++) {
        if (stat->tp_cnt > 0) {
           len = snprintf(ptr, 100,
                          "  {\"pid\":%d, \"tp_cnt\":%d},", i, stat->tp_cnt);
           ptr += len;
        }
    }

    ptr--;		// remove trailing ","
    len = snprintf(ptr, 100, "]\n");
    ptr += len;
    *ptr = 0;

//    printf("%s\n", buf);
}


void prog_stat_convert_json (char *buf)
{
    in_session_t* ses = get_in_session(0);
    pat_info_t* pat = &ses->pat;

    int len;
    char* ptr = buf;
    int i, j, id;

    len = snprintf(ptr, 100, "[ ");
    ptr += len;
    for (i=0; i<pat->prog_cnt; i++) {
        len = snprintf(ptr, 100, "{ \"program\":%d, \"tp_cnt\": [",
                      pat_get_prog_num(pat->prog_info[i]));
        ptr += len;
        for (j = 0, id = time_id; j < MAX_TIME_ID; j++) {
            len = snprintf(ptr, 100, " %d,", prog_tp_cnt[i][id]);
            ptr += len;
            if (++id == MAX_TIME_ID)  id = 0;
        }
        ptr--;		// remove trailing ","
        len = snprintf(ptr, 100, " ] },");
        ptr += len;
    }

    ptr--;		// remove trailing ","
    len = snprintf(ptr, 100, "]\n");
    ptr += len;
    *ptr = 0;

//    printf("Result: len %d\n", strlen(buf));
//    printf("%s\n", buf);
}


void psi_table_convert_html (char *buf)
{
    int len;
    char* ptr = buf;

    in_session_t* ses = get_in_session(0);
    pat_info_t* pat = &ses->pat;

    len = snprintf(ptr, 100,
                   "<ul>\n"
                   "  <li>PAT</li>\n"
                   "    <ul>\n"
                   "      <li>Version: %d</li>\n"
                   "      <li>TSID: %d</li>\n",
                   pat->ver, pat->tsid);
    ptr += len;

    int i, j;
    for (i=0; i<pat->prog_cnt; i++) {
        len = snprintf(ptr, 100,
                       "      <li class=\"prog\">Program %d</li>\n"
                       "      <ul>\n"
                       "        <li>PMT pid: %d</li>\n",
                       pat_get_prog_num(pat->prog_info[i]),
		       pat_get_pmt_pid(pat->prog_info[i]));
		ptr += len;

        in_prog_t* prog = in_prog_lookup_by_prog_num(&ses->prog_list,
                                  pat_get_prog_num(pat->prog_info[i]));
        if (prog) {
            pmt_info_t* pmt = &prog->pmt;
            pmt_header_t* pmt_hdr = (pmt_header_t*)pmt->sect->sect_hdr;
            len = snprintf(ptr, 100,
                           "        <li>Version: %d</li>\n"
                           "        <li>PCR pid: %d</li>\n",
                           pmt_hdr->ver, pmt_get_pcr_pid(pmt_hdr));
            ptr += len;

            for (j=0; j<pmt->es_cnt; j++) {
                pmt_es_info_t* es_info = pmt_info_get_es(pmt, j);
                len = snprintf(ptr, 100,
                               "        <li class=\"es\">ES pid %d, Type %d",
                               pmt_get_es_pid(es_info), es_info->es_type);
                ptr += len;
                if (es_info->es_type == 2) {
                    len = snprintf(ptr, 100, " (Video)");
                    ptr += len;
                } else {
                    int info_len = pmt_get_es_info_len(es_info);
                    uint8* ptr2 = (uint8*)(es_info + 1);
                    while (info_len > 0) {
                        descriptor_header_t* desc = (descriptor_header_t*)ptr2;
                        if (desc->tag == DESC_TAG_LANG) {
                            uint8* ptr3 = (uint8*)(desc + 1);
                            len = snprintf(ptr, 100, " (Audio %c%c%c)",
                                           ptr3[0], ptr3[1], ptr3[2]);
                            ptr += len;
                        }
                        ptr2 += sizeof(descriptor_header_t) + desc->len;
                        info_len -= sizeof(descriptor_header_t) + desc->len;
                    }
                }
                len = snprintf(ptr, 100, "</li>\n");
                ptr += len;
            }
        }

        len = snprintf(ptr, 100, "      </ul>\n");
        ptr += len;
    }

    len = snprintf(ptr, 100,
                   "  </ul>\n"
                   "</ul>\n");
    ptr += len;
    *ptr = 0;

//    printf("%s\n", buf);
}
