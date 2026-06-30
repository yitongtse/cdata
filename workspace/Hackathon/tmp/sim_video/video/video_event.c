/*-----------------------------------------------------------------------------
 * video_event.c - Video event reporting infra (CLC to SUP)
 *
 * October 2014
 *
 * Copyright (c) 2014-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *-----------------------------------------------------------------------------
 */

#include "video.h"
#include "video_event.h"


static
void video_source_set (video_source_t *src, in_config_t *cfg)
{
    src->ip_ver = cfg->ip_ver;
    src->src_ip[0] = cfg->src_ip[0];
    src->src_ip[1] = cfg->src_ip[1];
    src->src_ip[2] = cfg->src_ip[2];
    src->src_ip[3] = cfg->src_ip[3];
    src->dst_ip[0] = cfg->dst_ip[0];
    src->dst_ip[1] = cfg->dst_ip[1];
    src->dst_ip[2] = cfg->dst_ip[2];
    src->dst_ip[3] = cfg->dst_ip[3];
    src->src_udp = cfg->src_udp;
    src->dst_udp = cfg->dst_udp;
}


void video_event_fire_source_state_change (in_config_t *cfg,
                                           uint8 old_state, uint8 new_state)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: SOURCE_STATE_CHANGE");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_SOURCE_STATE_CHANGE;
    hdr->subtype = 0;
    hdr->owner_id = cfg->owner_id;

    video_event_source_state_change_t* body
            = (video_event_source_state_change_t*)(hdr + 1);
    video_source_set(&body->source, cfg);
    body->old_state = old_state;
    body->new_state = new_state;

    video_event_send(ev);
}


void video_event_fire_session_state_change (uint32 session_id, uint16 prog_num,
                                            uint8 owner_id,
                                            uint8 old_state, uint8 new_state)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: SESSION_STATE_CHANGE");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_SESSION_STATE_CHANGE;
    hdr->subtype = 0;
    hdr->owner_id = owner_id;

    video_event_session_state_change_t* body
            = (video_event_session_state_change_t*)(hdr + 1);
    body->session_id = session_id;
    body->prog_num = prog_num;
    body->old_state = old_state;
    body->new_state = new_state;

    video_event_send(ev);
}


void video_event_fire_config_err (int msg_type, int oper, uint8 owner_id,
                                  uint32 resource_id, int err_code,
                                  uint16 num_item, uint16 *items)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: CONFIG_ERR");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_CONFIG_ERR;
    hdr->subtype = msg_type;
    hdr->owner_id = owner_id;

    video_event_config_err_t* body = (video_event_config_err_t*)(hdr + 1);
    body->oper = oper;
    body->resource_id = resource_id;
    body->err_code = err_code;
    body->num_item = num_item;

    int i;
    for (i=0; i<num_item; i++) {
        body->item[i] = items[i];
    }

    video_event_send(ev);
}


void video_event_fire_source_err (in_session_t *ses, int subtype)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: SOURCE_ERR");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_SOURCE_ERR;
    hdr->subtype = subtype;
    hdr->owner_id = ses->cfg->owner_id;

    video_event_source_err_t* body = (video_event_source_err_t*)(hdr + 1);
    video_source_set(&body->source, ses->cfg);

    switch (subtype) {
        case VIDEO_EVENT_BITRATE_EXCEEDED:
            body->bitrate = ses->avg_bitrate;
            break;
        default:
            break;
    }

    video_event_send(ev);
}


void video_event_fire_session_err (out_session_t *ses)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: SESSION_ERR");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_SESSION_ERR;
    hdr->subtype = 0;
    hdr->owner_id = ses->cfg->owner_id;

    video_event_session_err_t* body = (video_event_session_err_t*)(hdr + 1);
    body->session_id = ses->cfg->client_id;

    video_event_send(ev);
}


void video_event_fire_qam_err (qam_info_t *qam)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: QAM_ERR");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_QAM_ERR;
    hdr->subtype = 0;
    hdr->owner_id = 0;

    video_event_qam_err_t* body = (video_event_qam_err_t*)(hdr + 1);
    body->qam_id = qam->id;

    video_event_send(ev);
}


void video_event_fire_conflict_err (int subtype, uint16 qam_id, uint8 owner_id,
                                    uint32 session_id, uint16 id)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: CONFLICT_ERR");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_CONFLICT_ERR;
    hdr->subtype = subtype;
    hdr->owner_id = owner_id;

    video_event_conflict_err_t* body = (video_event_conflict_err_t*)(hdr + 1);
    body->qam_id = qam_id;
    body->session_id = session_id;
    body->id = id;

    video_event_send(ev);
}


void video_event_fire_operation_err (int err_code)
{
    video_event_t* ev = video_event_alloc();
    if (!ev) {
        VIDEO_DEBUG("Failed to allocate video event: OPERATION_ERR");
        return;
    }

    video_event_hdr_t *hdr = video_event_get_data(ev);
    hdr->type = VIDEO_EVENT_OPERATION_ERR;
    hdr->owner_id = 0;

    video_event_operation_err_t* body = (video_event_operation_err_t*)(hdr + 1);
    body->err_code = err_code;

    video_event_send(ev);
}

