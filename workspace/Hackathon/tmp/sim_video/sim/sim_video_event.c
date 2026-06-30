///  Copyright (c) 2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
//#include <arpa/inet.h>
#include "common.h"
#include "video.h"
#include "video_util.h"
#include "video_event.h"


uint8 video_event_buf[2048];


void video_source_print (video_source_t *src)
{
    char src_ip_str[INET6_ADDRSTRLEN];
    char dst_ip_str[INET6_ADDRSTRLEN];
    int ip_ver = src->ip_ver == IPV4_VERSION ? AF_INET : AF_INET6;
    inet_ntop(ip_ver, src->src_ip, src_ip_str, INET6_ADDRSTRLEN);
    inet_ntop(ip_ver, src->dst_ip, dst_ip_str, INET6_ADDRSTRLEN);
    printf("IP %s -> %s, UDP %d -> %d\n",
           src_ip_str, dst_ip_str, src->src_udp, src->dst_udp);
}


video_event_t* video_event_alloc (void)
{
    return (video_event_t*)video_event_buf;
}


video_event_hdr_t* video_event_get_data (video_event_t *ev)
{
    return (video_event_hdr_t*)ev;
}


void video_event_send (video_event_t* buf)
{
    int rc;
    video_event_msg_t *ev = (video_event_msg_t*)video_event_get_data(buf);

    printf("\nVideo event:\n");
    printf("  Type: %d - ", ev->hdr.type);

    switch (ev->hdr.type) {
        case VIDEO_EVENT_SOURCE_STATE_CHANGE:
            printf("Source state change\n");
            printf("  Owner: %d\n", ev->hdr.owner_id);
            printf("  Source: ");
            video_source_print(&ev->body.src_state_chg.source);
            printf("  State: %d -> %d\n", ev->body.src_state_chg.old_state,
                   ev->body.src_state_chg.new_state);
            break;

        case VIDEO_EVENT_SESSION_STATE_CHANGE:
            printf("Session state change\n");
            printf("  Owner: %d\n", ev->hdr.owner_id);
            printf("  Session ID: %d\n", ev->body.ses_state_chg.session_id);
            printf("  Program number: %d\n", ev->body.ses_state_chg.prog_num);
            printf("  State: %d -> %d\n", ev->body.ses_state_chg.old_state,
                   ev->body.ses_state_chg.new_state);
            break;

        case VIDEO_EVENT_CONFIG_ERR:
            printf("Config error\n");

            printf("  Subtype: %d - ", ev->hdr.subtype);
            switch (ev->hdr.subtype) {
                case MSG_TYPE_VIDEO_ADD_SESSION:
                    printf("Add session\n");
                    break;
                case MSG_TYPE_VIDEO_DELETE_SESSION:
                    printf("Delete session\n");
                    break;
                case MSG_TYPE_VIDEO_UPDATE_PID_FILTER:
                    printf("Update pid filter\n");
                    printf("  Oper: %d\n", ev->body.cfg_err.oper);
                    break;
                case MSG_TYPE_VIDEO_UPDATE_PROG_FILTER:
                    printf("Update program filter\n");
                    printf("  Oper: %d\n", ev->body.cfg_err.oper);
                    break;
                case MSG_TYPE_VIDEO_UPDATE_PID_REMAP:
                    printf("Update pid remap\n");
                    printf("  Oper: %d\n", ev->body.cfg_err.oper);
                    break;
                case MSG_TYPE_VIDEO_UPDATE_PROG_REMAP:
                    printf("Update program remap\n");
                    printf("  Oper: %d\n", ev->body.cfg_err.oper);
                    break;
                case MSG_TYPE_VIDEO_ADD_CAROUSEL:
                    printf("Add carousel\n");
                    break;
                case MSG_TYPE_VIDEO_DELETE_CAROUSEL:
                    printf("Delete carousel\n");
                    break;
                case MSG_TYPE_VIDEO_CONFIG_QAM:
                    printf("Qam config\n");
                    break;
                default:
                    printf("UNKNOWN\n");
            }
            printf("  Owner: %d\n", ev->hdr.owner_id);
            printf("  Resource: %d\n", ev->body.cfg_err.resource_id);
            printf("  Err code: %s\n", strerror(ev->body.cfg_err.err_code));
            break;

        case VIDEO_EVENT_SOURCE_ERR:
            printf("Source error\n");
            printf("  Owner: %d\n", ev->hdr.owner_id);
            printf("  Source: ");
            video_source_print(&ev->body.src_err.source);
            printf("  Subtype: %d - ", ev->hdr.subtype);
            switch (ev->hdr.subtype) {
                case VIDEO_EVENT_BITRATE_EXCEEDED:
                    printf("Bitrate exceeded\n");
                    printf("  Bitrate: %d\n", ev->body.src_err.bitrate);
                    break;
                default:
                    break;
            }
            break;

        case VIDEO_EVENT_SESSION_ERR:
            printf("Session error\n");
            printf("  Owner: %d\n", ev->hdr.owner_id);
            printf("  Session: %d\n", ev->body.ses_err.session_id);
            break;

        case VIDEO_EVENT_QAM_ERR:
            printf("Qam error\n");
            printf("  Owner: %d\n", ev->hdr.owner_id);
            printf("  QAM: %d\n", ev->body.qam_err.qam_id);
            break;

        case VIDEO_EVENT_CONFLICT_ERR:
            printf("Conflict error\n");
            printf("  Subtype: %d - ", ev->hdr.subtype);
            switch (ev->hdr.subtype) {
                case VIDEO_EVENT_PID_CONFLICT:
                    printf("PID conflict\n");
                    break;
                case VIDEO_EVENT_PROGRAM_CONFLICT:
                    printf("Program number conflict\n");
                    break;
                default:
                    printf("UNKNOWN\n");
            }

            printf("  Owner: %d\n", ev->hdr.owner_id);
            printf("  QAM: %d\n", ev->body.conflict_err.qam_id);
            printf("  Session: %d\n", ev->body.conflict_err.session_id);
            printf("  ID in conflict: %d\n", ev->body.conflict_err.id);
            break;

        case VIDEO_EVENT_OPERATION_ERR:
            printf("Operation error\n");
            printf("  Error code: %d\n", ev->body.oper_err.err_code);
            break;

        default:
            printf("UNKNOWN\n");
    }
}

