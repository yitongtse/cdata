/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "video.h"
#include "video_util.h"
#include "video_config_db.h"


int prepare_session_key (uint16 rid, in_config_t *cfg, ses_hash_key_t *key)
{
    switch (cfg->ip_ver) {

    case IPV4_VERSION:
        key->mcast_flag = is_multicast_v4(cfg->dst_ip[0]);
        break;

    case IPV6_VERSION:
        key->mcast_flag = is_multicast_v6(cfg->dst_ip);
        break;

    default:
        VIDEO_LOG("IN %d: Invalid IP Version %d", rid, cfg->ip_ver);
        return EINVAL;
    }

    switch (cfg->input_type) {

    case INPUT_TYPE_UNICAST:
        if (key->mcast_flag) {
            VIDEO_LOG("IN %d: Invalid unicast IP address %08x, ver %02d",
                      rid, cfg->dst_ip[0], cfg->ip_ver);
             return EINVAL;
        }
        if (cfg->dst_udp == 0) {
            VIDEO_LOG("IN %d: Invalid UDP port %d", rid, cfg->dst_udp);
            return EINVAL;
        }
        break;

    case INPUT_TYPE_MULTICAST_ASM:
    case INPUT_TYPE_MULTICAST_SSM:
        if (!key->mcast_flag) {
            VIDEO_LOG("IN %d: Invalid multicast IP address %08x, ver %02d",
                      rid, ntohl(cfg->dst_ip[0]), cfg->ip_ver);
            return EINVAL;
        }
        break;

    default:
        VIDEO_LOG("IN %d: Invalid cast type %d", rid, cfg->input_type);
        return EINVAL;
    }

    // Set up session key from in_config
    key->ip_ver = cfg->ip_ver;

    if (cfg->ip_ver == IPV4_VERSION) {
        key->dst_ip[0] = cfg->dst_ip[0];
        key->dst_ip[1] = key->dst_ip[2] = key->dst_ip[3] = 0;
        key->extra[0] = key->mcast_flag ? cfg->src_ip[0] : cfg->dst_udp;
        key->extra[1] = key->extra[2] = key->extra[3] = 0;

    } else {
        memcpy(key->dst_ip, cfg->dst_ip, 16);
        if (key->mcast_flag) {
            memcpy(key->extra, cfg->src_ip, 16);
        } else {
            key->extra[0] = cfg->dst_udp;
            key->extra[1] = key->extra[2] = key->extra[3] = 0;
        }
    }

    return EOK;
}


int provision_in_session (video_context_t *ctx, uint16 *rid, in_config_t *cfg)
{
    hash_table_t *hash_tab;
    ses_hash_key_t key;
    ses_hash_item_t *item;

    int rc = prepare_session_key(*rid, cfg, &key);
    if (rc != EOK)  return rc;

    // Check if session is already provisioed
    hash_tab = ctx->ses_hash_tab[cfg->input_port];
    item = (ses_hash_item_t*)hash_table_lookup(hash_tab, &key);
    if (item) {
        if (item->cast_type != cfg->input_type)  return EINVAL;
        *rid = item - ctx->ses_hash_item;
        return EEXIST;
    }

    // Set up session item
    item = &ctx->ses_hash_item[*rid];
    item->key = key;
    item->cast_type = cfg->input_type;
    item->flow_rd = item->flow_wr = 0;
    item->info = ip_flow_buf + *rid * IP_FLOW_BUF_SIZE;
    item->hash_code = ses_hash_code(&key);    // TODO: temporarily workaround

    // Add session to hash table
    hash_table_add(hash_tab, &key, (hash_item_t*)item);

    return EOK;
}


int unprovision_in_session (video_context_t *ctx, uint16 *rid, in_config_t *cfg)
{
    ses_hash_key_t key;
    ses_hash_item_t *item;

    int rc = prepare_session_key(*rid, cfg, &key);
    if (rc != EOK)  return rc;

    item = (ses_hash_item_t*)
               hash_table_delete(ctx->ses_hash_tab[cfg->input_port], &key);
    if (!item) {
        return ENOENT;
    }

    *rid = item - ctx->ses_hash_item;
    return EOK;
}


int provision_qam_flow (video_context_t *ctx, uint16 rid, out_config_t *cfg)
{
    uint16 qam_id = cfg->qam_id;
    qam_config_t* qam_cfg = get_qam_config(ctx, qam_id);
    if (qam_cfg->flow_cnt >= MAX_VIDEO_FLOWS_PER_QAM) {
        em_vidman_VIDEO_FLOW_ALLOC_FAILED("PROV", rid, cfg->qam_id);
        return ENOMEM;
    }

    int i;
    int32* flow_map = get_qam_flow_map(ctx, cfg->qam_id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            cfg->flow_id = i;
            flow_map[i] = (cfg->in_ses_id << 16) | rid;
            qam_cfg->flow_cnt++;

            video_db_qam_cnfg_checkin(ctx->id, qam_id, qam_cfg);

            //VIDEO_DEBUG("\nProv qamflow %d-%d: in %d, out %d",
            //            cfg->qam_id, i, cfg->in_ses_id, rid);
            return EOK;
        }
    }

    return ENOMEM;
}


int unprovision_qam_flow (video_context_t *ctx, out_config_t *cfg)
{
    uint16 qam_id = cfg->qam_id;
    qam_config_t *qam_cfg = get_qam_config(ctx, qam_id);

    assert(qam_id < NUM_QAMS);
    assert(cfg->flow_id < MAX_VIDEO_FLOWS_PER_QAM);

    //VIDEO_DEBUG("Unprov qam flow %d-%d: cnt %d\n",
    //            qam_id, cfg->flow_id, qam_cfg->flow_cnt);
    get_qam_flow_map(ctx, qam_id)[cfg->flow_id] = INVALID_ID;
    qam_cfg->flow_cnt--;
    cfg->flow_id = INVALID_FLOW_ID;

    video_db_qam_cnfg_checkin(ctx->id, qam_id, qam_cfg);
    return EOK;
}

