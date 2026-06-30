/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "video.h"
#include "video_util.h"
#include "video_cli.h"


// Verify the IP header conforms to the following:
//   - IPv4
//   - no optional fields
//   - no IP fragmentation
//
// Returns TRUE the IP header pass the test
//
static inline
boolean check_ip_header (ip_header_t* hdr)
{
    if (hdr->ver == IPV4_VERSION) {
        return ((hdr->ihl == IPV4_LENGTH)
                && (!(hdr->flags & IP_FLAG_MORE_MASK))
                && (hdr->frag_offset == 0));
    } else {
        ipv6_header_t* ipv6_hdr = (ipv6_header_t*) hdr;
        return (ipv6_hdr->next_header == IP_PROT_UDP);
    }
}


boolean ses_hash_match (void *key, hash_item_t *item)
{
    ses_hash_key_t* key2 = key;
    ses_hash_item_t* item2 = (ses_hash_item_t*)item;

    if (key2->dst_ip[0] != item2->key.dst_ip[0] ||
        key2->ip_ver != item2->key.ip_ver) { 
        return FALSE;
    }

    if (key2->ip_ver == IPV4_VERSION) {
        // IPv4 
        return (item2->cast_type == INPUT_TYPE_MULTICAST_ASM
                || key2->extra[0] == item2->key.extra[0]
                || (!key2->extra[0] &&
                        item2->cast_type == INPUT_TYPE_MULTICAST_SSM));
    
    } else {
        // IPv6
        if (key2->dst_ip[1] != item2->key.dst_ip[1] ||
            key2->dst_ip[2] != item2->key.dst_ip[2] ||
            key2->dst_ip[3] != item2->key.dst_ip[3]) {
            return FALSE;
        }
    
        if (item2->cast_type == INPUT_TYPE_UNICAST) {
            // udp port
            return (key2->extra[0] == item2->key.extra[0]);

        } else if (item2->cast_type == INPUT_TYPE_MULTICAST_SSM) {
            // For SSM: key's source ip either match with item's, 
            //          or is all 0
            return ((key2->extra[0] == item2->key.extra[0] &&
                     key2->extra[1] == item2->key.extra[1] &&
                     key2->extra[2] == item2->key.extra[2] &&
                     key2->extra[3] == item2->key.extra[3])
                    ||
                     (!key2->extra[0] && !key2->extra[1] &&
                      !key2->extra[2] && !key2->extra[3]));

        } else {
            // ASM case , just return true for now
            return TRUE;
        }
    }

    return (FALSE);
}


void ses_hash_print (FILE *fp, hash_item_t *item)
{
    ses_hash_item_t* item2 = (ses_hash_item_t*)item;
    uint16 id = item2 - video_ctx->ses_hash_item;
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;
    char dst_str[INET6_ADDRSTRLEN];
    char src_str[INET6_ADDRSTRLEN];
    int addr_family;

    if(item2->key.ip_ver == IPV4_VERSION) {
        addr_family = AF_INET;
        // generate strings for dest and src ip address
        sa.sin_addr.s_addr = item2->key.dst_ip[0];
        inet_ntop(addr_family, &(sa.sin_addr), dst_str, INET_ADDRSTRLEN);
        if (item2->cast_type == INPUT_TYPE_MULTICAST_SSM) {
            sa.sin_addr.s_addr = item2->key.extra[0];
            inet_ntop(addr_family, &(sa.sin_addr), src_str, INET_ADDRSTRLEN);
        }
    } else {
        printf("IPV6 not supported\n");
        assert(0);
#if 0
        addr_family = AF_INET6;
        memcpy(&sa6.sin6_addr.s6_addr32,
               item2->key.dst_ip, sizeof(item2->key.dst_ip));
        inet_ntop(addr_family, &(sa6.sin6_addr),
                  dst_str, INET6_ADDRSTRLEN);
        if (item2->cast_type == INPUT_TYPE_MULTICAST_SSM) {
            memcpy(sa6.sin6_addr.s6_addr32,
                   item2->key.extra, sizeof(item2->key.extra));
            inet_ntop(addr_family, &(sa6.sin6_addr), 
                      src_str, INET6_ADDRSTRLEN);
        }
#endif
    }
   
    switch (item2->cast_type) {
    case INPUT_TYPE_UNICAST:
        fprintf(fp, "  Session %d: UNICAST, IP %s, udp %d\n",
               id, dst_str, item2->key.extra[0]);
        break;
    case INPUT_TYPE_MULTICAST_ASM:
        fprintf(fp, "  Session %d: ASM, Grp IP %s\n", id, dst_str);
        break;
    case INPUT_TYPE_MULTICAST_SSM:
        fprintf(fp, "  Session %d: SSM, Src IP %s, Grp IP %s\n",
               id, src_str, dst_str);
        break;
    default:
        fprintf(fp, "Bad session type %d\n", item2->cast_type);
    }
}


void get_in_session_key (ip_header_t *ip_hdr, boolean mcast_flag,
                         ses_hash_key_t *key)
{
    key->ip_ver = ip_hdr->ver;
    key->mcast_flag = mcast_flag;

    if (ip_hdr->ver == IPV4_VERSION) {
        key->dst_ip[0] = ip_hdr->dst_ip;

        if (mcast_flag) {
            key->extra[0] = ip_hdr->src_ip;
        } else {
            udp_header_t* udp_hdr = (udp_header_t*)(ip_hdr + 1);
            key->extra[0] = udp_hdr->dst_port;
        }

    } else {
        ipv6_header_t *ipv6_hdr = (ipv6_header_t *)ip_hdr;
        memcpy(key->dst_ip, ipv6_hdr->dst_ip, sizeof(ipv6_hdr->dst_ip));

        if (mcast_flag) {
            // memcpy(key->extra, ipv6_hdr->src_ip, sizeof(ipv6_hdr->src_ip));
            key->extra[0] = ipv6_hdr->src_ip[0];
            key->extra[1] = ipv6_hdr->src_ip[1];
            key->extra[2] = ipv6_hdr->src_ip[2];
            key->extra[3] = ipv6_hdr->src_ip[3];
        } else {
            udp_header_t* udp_hdr = (udp_header_t*)(ipv6_hdr + 1);
            key->extra[0] = udp_hdr->dst_port;
        }
    }
}


// Demux an input IP packet
//
void demux_ip_pkt (uintptr_t ip_pkt_addr, hash_table_t *ses_hash_tab,
                   video_ip_stat_t *my_stat)
{
    ses_hash_key_t ses_key;
    ses_hash_item_t *ses_item;
    ip_header_t* hdr = (ip_header_t*)ip_pkt_addr;

    if (!check_ip_header(hdr)) {
        my_stat->err_cnt++;
        check_capture(hdr, CLI_VIDEO_CAPTURE_DROP);
        return;
    }

    boolean mcast_flag;
    if (hdr->ver == IPV4_VERSION) {
        my_stat->ipv4_cnt++;
        mcast_flag = is_multicast_v4(hdr->dst_ip);
    } else {
        my_stat->ipv6_cnt++;
        mcast_flag = is_multicast_v6(((ipv6_header_t*)hdr)->dst_ip);
    }

    if (mcast_flag) {
        my_stat->mcast_cnt++;
    } else {
        my_stat->ucast_cnt++;
    }

    // Look up the session
    get_in_session_key(hdr, mcast_flag, &ses_key);
    ses_item = (ses_hash_item_t*)hash_table_lookup(ses_hash_tab, &ses_key);

    if (!ses_item) {
        // Not such session
        my_stat->unref_cnt++;
        check_capture(hdr, CLI_VIDEO_CAPTURE_DROP);
        return;
    }

    (ses_item->info++)->addr = ip_pkt_addr;
    if (++ses_item->flow_wr == IP_FLOW_BUF_SIZE) {
        ses_item->flow_wr = 0;
        ses_item->info -= IP_FLOW_BUF_SIZE;
    }

    // Check for IP flow buffer overrun
    if (ses_item->flow_wr == ses_item->flow_rd) {
        my_stat->overrun_cnt++;
    }

    check_capture(hdr, CLI_VIDEO_CAPTURE_ANY);

}

