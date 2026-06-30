/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: ermi1_test.c
*    @brief ERRP Test program
*    @author Yi Tong Tse
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "ermi1.h"
#include "rfgw_errp.h"


errp_node_t eqam_node;
errp_node_t erm_node;

char msg_buf[1024];

#define NUM_ROUTES              1
errp_route_t route[NUM_ROUTES] =
    { ERRP_ADDR_FAMILY, ERRP_PROT_DYNAMIC, "rtsp://192.16.0.1/" };

#define NUM_QAMS                1
rfgw_qam_t qam[NUM_QAMS];

errp_qam_cfg_t qam_cfg;
errp_qam_cap_t qam_cap;


void print_hex (void* ptr, int nbytes)
{
    int i;
    uint8 *temp = ptr;

    for (i=0; nbytes>0; nbytes--) {
        printf("%02x ", *temp++);
        if (++i==16) {
            i = 0;
            printf("\n");
        }
    }
    if (i) {
        printf("\n");
    }
}


void erm_node_init (void)
{
    memset(&erm_node, 0, sizeof(errp_node_t));
}


void eqam_node_init (void)
{
    errp_node_init(&eqam_node);
    strcpy(eqam_node.streaming_zone, "san_jose");
    strcpy(eqam_node.component_name, "RFGW-10");
    strcpy(eqam_node.vendor_string, "CISCO EQAM product RFGW-10");
}


void eqam_qam_init (void)
{
    // QAM channel initialization
    qam[0].slot = 3;
    qam[0].port = 1;
    qam[0].channel = 1;
    qam[0].annex_type = RFGW_ANNEX_B;
    qam[0].srate = 5;
    qam[0].type = LCC_QAM_TYPE_VIDEO;
    qam[0].video_qam_map = LCC_VIDEO_QAM_MAP_24;
    qam[0].tsid = 1024;
    qam[0].bw = 6;
    qam[0].fec_i = 128;
    qam[0].fec_j = 1;
    qam[0].freq = 500000000;
}


void eqam_send_open_msg (void)
{
    char* ptr = msg_buf;
    int msg_len = errp_put_open_msg(&ptr, &eqam_node);
    print_hex(msg_buf, msg_len);
}


void eqam_send_update_msg (void)
{
    char* ptr = msg_buf;
    char* msg_hdr = errp_skip_msg_hdr(&ptr);
    int msg_len = ERRP_MSG_HDR_SIZE;

    // Add all attributes here

    // Reachable route
    msg_len += errp_put_attr_reachable_routes(&ptr, 1, route);

    // Next hop server
    msg_len += errp_put_attr_next_hop_server(&ptr, eqam_node.addr_domain,
                       eqam_node.component_name, eqam_node.streaming_zone);

    // QAM name
    msg_len += errp_put_attr_qam_names(&ptr, "Qam3/1.1");

    // QAM config
    rfgw_errp_get_qam_cfg(&qam[0], &qam_cfg);
    msg_len += errp_put_attr_qam_cfg(&ptr, &qam_cfg);

    // QAM capability
    rfgw_errp_get_qam_cap(&qam[0], &qam_cap);
    msg_len += errp_put_attr_qam_cap(&ptr, &qam_cap);

    // Total bandwidth
    // TODO: get the bandwidth (in kbps) from rfgw_qam_t
    msg_len += errp_put_attr_total_bandwidth(&ptr, 38800000);

    errp_put_msg_hdr(&msg_hdr, ERRP_UPDATE, msg_len);

    print_hex(msg_buf, msg_len);
}


void erm_receive_msg (void)
{
    char* ptr = msg_buf;
    errp_get_msg((const char **)&ptr, &erm_node);
}



int main (int argc, char** argv)
{
    // ERM set up
    erm_node_init();

    // EQAM set up
    eqam_node_init();
    eqam_qam_init();

    // OPEN message
    printf("EQAM: send OPEN message\n");
    eqam_send_open_msg();

    printf("\nERM: receive OPEN message\n");
    erm_receive_msg();
    errp_node_print(&erm_node);

    // UPDATE message
    printf("\nEQAM: send UPDATE message\n");
    eqam_send_update_msg();

    printf("\nERM: receive UPDATE message\n");
    erm_receive_msg();
}

