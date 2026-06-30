/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: ermi1_msg_gen.c
*    @brief ERRP message generation functions
*    @author Yi Tong Tse
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "ermi1.h"
#include "rfgw_errp.h"


// Generic message format tools
//

// Skip over a 2-byte short
static inline
char* skip_short (char **buf)
{
    char* buf2 = *buf;
    *buf += sizeof(short);
    return buf2;
}


// Skip over a 4-byte long
static inline
char* skip_long (char **buf)
{
    char* buf2 = *buf;
    *buf += sizeof(long);
    return buf2;
}


// Put a byte, and advance the write pointer
static inline
int put_byte (char **buf, uint8 data)
{
    *(*buf)++ = data;
    return 1;
}

// Put a 2-byte short, and advance the write pointer
static inline
int put_short (char **buf, uint16 data)
{
    *(*buf)++ = (data >> 8) & 0xFF;
    *(*buf)++ = data & 0xFF;
    return 2;
}

// Put a 4-byte long, and advance the write pointer
static inline
int put_long (char **buf, uint32 data)
{
    *(*buf)++ = (data >> 24) & 0xFF;
    *(*buf)++ = (data >> 16) & 0xFF;
    *(*buf)++ = (data >> 8) & 0xFF;
    *(*buf)++ = data & 0xFF;
    return 4;
}

// Put a null-terminated string, and advance the write pointer
static inline
int put_string (char **buf, const char *data)
{
    int len = strlen(data) + 1;    // including trailing 0
    memcpy(*buf, data, len);
    *buf += len;
    return len;
}


//! Write an ERRP-styled string (a string preceeded by its 2-byte length field)
//! @return Number of bytes written
//
int errp_put_string (char **buf, const char *string)
{
    int str_len = strlen(string);
    put_short(buf, str_len);
    put_string(buf, string);
    return sizeof(short) + str_len;
}


// ERRP message formatting
//

//! Skip over the ERRP message header
char* errp_skip_msg_hdr (char **buf)
{
    char* buf2 = *buf;
    *buf += ERRP_MSG_HDR_SIZE;
    return buf2;
}


//! Write ERRP message header
int errp_put_msg_hdr (char **buf, int msg_type, int len)
{
    put_short(buf, len);
    put_byte(buf, msg_type);
    return ERRP_MSG_HDR_SIZE;
}


//! Skip over ERRP parameter header (in OPEN message)
char* errp_skip_param_hdr (char **buf)
{
    char* buf2 = *buf;
    *buf += ERRP_PARAM_HDR_SIZE;
    return buf2;
}


//! Write ERRP parameter header (in OPEN message)
int errp_put_param_hdr (char **buf, errp_param_t param_type, int param_len)
{
    put_short(buf, param_type);
    put_short(buf, param_len);
    return ERRP_PARAM_HDR_SIZE;
}


//! Write ERRP Capability Info header
int errp_put_cap_info_hdr (char **buf, errp_cap_code_t cap_code, int cap_len)
{
    put_short(buf, cap_code);
    put_short(buf, cap_len);
    return ERRP_CAP_INFO_HDR_SIZE;
}


//! Write ERRP Route Type Supported Capability
int errp_put_cap_route_type_supported (char **buf, const errp_node_t *node)
{
    int len = errp_put_cap_info_hdr(buf, ERRP_CAP_ROUTE_TYPES_SUPPORTED, 4);
    len += put_short(buf, node->addr_family);
    len += put_short(buf, node->app_protocol);
    return len;
}


//! Write ERRP Send Receive Capability
int errp_put_cap_send_receive (char **buf, const errp_node_t *node)
{
    int len = errp_put_cap_info_hdr(buf, ERRP_CAP_SEND_RECEIVE, 4);
    len += put_long(buf, node->send_receive);
    return len;
}


//! Write ERRP Version Capability
int errp_put_cap_errp_version (char **buf, const errp_node_t *node)
{
    int len = errp_put_cap_info_hdr(buf, ERRP_CAP_ERRP_VERSION, 4);
    len += put_long(buf, node->errp_version);
    return len;
}


//! Write all ERRP Capabilities
int errp_put_param_cap_info (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = errp_put_cap_route_type_supported(buf, node);
    len += errp_put_cap_send_receive(buf, node);
    len += errp_put_cap_errp_version(buf, node);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_CAP_INFO, len);
    return len;
}


//! Write Streaming Zone parameter
int errp_put_param_streaming_zone (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = put_string(buf, node->streaming_zone);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_STREAMING_ZONE, len);
    return len;
}


//! Write Component Name parameter
int errp_put_param_component_name (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = put_string(buf, node->component_name);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_COMPONENT_NAME, len);
    return len;
}


//! Write Vendor Specific String parameter (optional)
int errp_put_param_vendor_string (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = put_string(buf, node->vendor_string);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_VENDOR_STRING, len);
    return len;
}


//! Write ERRP OPEN message
int errp_put_open_msg (char **buf, const errp_node_t *node)
{
    char* param_len_ptr;
    int param_len;

    char* msg_hdr = errp_skip_msg_hdr(buf);
    int msg_len = put_byte(buf, ERRP_OPEN_MSG_VERSION);
    msg_len += put_byte(buf, ERRP_RESERVED);
    msg_len += put_short(buf, node->hold_time);
    msg_len += put_long(buf, node->addr_domain);
    msg_len += put_long(buf, node->errp_id);

    param_len_ptr = skip_short(buf);
    param_len = errp_put_param_cap_info(buf, node);
    if (strlen(node->streaming_zone)) {
        param_len += errp_put_param_streaming_zone(buf, node);
    }
    if (strlen(node->component_name)) {
        param_len += errp_put_param_component_name(buf, node);
    }
    if (strlen(node->vendor_string)) {
        param_len += errp_put_param_vendor_string(buf, node);
    }
    msg_len += put_short(&param_len_ptr, param_len);
    msg_len += ERRP_MSG_HDR_SIZE + param_len;

    errp_put_msg_hdr(&msg_hdr, ERRP_OPEN, msg_len);
    return msg_len;
}


//! Skip over ERRP attribute header
static inline
char* errp_skip_attrib_hdr (char **buf)
{
    char* buf2 = *buf;
    *buf += ERRP_ATTR_HDR_SIZE;
    return buf2;
}


//! Write ERRP attribute header
int errp_put_attr_hdr (char **buf, uint8 attr_flags, uint8 attr_type_code,
                       int attr_len)
{
    int len = put_byte(buf, attr_flags);
    len += put_byte(buf, attr_type_code);
    len += put_short(buf, attr_len);
    return len;
}


//! Write a route attribute
int errp_put_route (char **buf, const errp_route_t *route)
{
    int len = put_short(buf, route->addr_family);
    len += put_short(buf, route->app_protocol);
    len += errp_put_string(buf, route->addr);
    return len;
}


//! Write WithdrawnRoutes attribute
int errp_put_attr_withdrawn_routes (char **buf, int num_routes,
                                    const errp_route_t *route)
{
    int len = 0;
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    for ( ; num_routes > 0; num_routes--) {
        len += errp_put_route(buf, route);
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_WITHDRAWN_ROUTE, len);
    return len;
}


//! Write ReachableRoutes attribute
int errp_put_attr_reachable_routes (char **buf, int num_routes,
                                    const errp_route_t *route)
{
    int len = 0;
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    for ( ; num_routes > 0; num_routes--) {
        len += errp_put_route(buf, route++);
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_REACHABLE_ROUTE, len);
    return len;
}


//! Write NextHopServer attribute
int errp_put_attr_next_hop_server (char **buf, uint32 addr_domain,
                                   const char* component_addr,
                                   const char* streaming_zone)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_long(buf, addr_domain);
    len += errp_put_string(buf, component_addr);
    len += errp_put_string(buf, streaming_zone);
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_NEXT_HOP_SERVER, len);
    return len;
}


//! Write QAM Names attribute
//  Note: According to ERMI, only one QAM Name can be included in an
//        attribute in the UDPATE from EQAM to ERM.
int errp_put_attr_qam_names (char **buf, const char *qam_name)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = errp_put_string(buf, qam_name);
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_QAM_NAMES, len);
    return len;
}


//! Write Total Bandwidth attribute
int errp_put_attr_total_bandwidth (char **buf, int bandwidth)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_QAM_CFG, sizeof(long));
    len += put_long(buf, bandwidth);
    return len;
}


//! Write an Edge Input
int errp_put_edge_input (char **buf, const errp_edge_input_t *input)
{
    int len = put_long(buf, input->subnet_mask);
    len += errp_put_string(buf, input->host_name);
    len += put_long(buf, input->if_id);
    len += put_long(buf, input->max_bandwidth);
    len += errp_put_string(buf, input->group_name);
    return len;
}


//! Write Edge Input attribute
int errp_put_attr_edge_input (char **buf, int num_inputs,
                              const errp_edge_input_t *input)
{
    int len = 0;
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    for ( ; num_inputs > 0; num_inputs--) {
        len += errp_put_edge_input(buf, input++);
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_EDGE_INPUT, len);
    return len;
}


//! Write QAM Configuration attribute
int errp_put_attr_qam_cfg (char **buf, const errp_qam_cfg_t *cfg)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_long(buf, cfg->freq);
    len += put_byte(buf, cfg->modulation);
    len += put_byte(buf, cfg->interleaver);
    len += put_short(buf, cfg->tsid);
    len += put_byte(buf, cfg->annex);
    len += put_byte(buf, cfg->chan_bw);
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_QAM_CFG, len);
    return len;
}


//! Write UPD Map for a static port
int errp_put_udp_map_static_port (char **buf,
                                  const errp_udp_map_static_port_t *map)
{
    int len = put_short(buf, map->udp_port);
    len += put_short(buf, map->prog_num);
    return len;
}


//! Write UPD Map for a static port range
int errp_put_udp_map_static_range (char **buf,
                                   const errp_udp_map_static_range_t *map)
{
    int len = put_short(buf, map->start_udp_port);
    len += put_short(buf, map->start_prog_num);
    len += put_short(buf, map->count);
    return len;
}


//! Write UPD Map for a dynamic port range
int errp_put_udp_map_dynamic_range (char **buf,
                                    const errp_udp_map_dynamic_range_t *map)
{
    int len = put_short(buf, map->start_udp_port);
    len += put_short(buf, map->count);
    return len;
}


//! Write UDP Map attribute
int errp_put_attr_udp_map (char **buf, int num_maps, const errp_udp_map_t *map)
{
    int i;
    int len = 0;
    uint32 count;
    char *ptr;

    char* attr_hdr = errp_skip_attrib_hdr(buf);

    // Scan for static ports
    count = 0;
    ptr = skip_long(buf);
    for (i=0; i<num_maps; i++) {
        if (map->type == ERRP_UDP_MAP_TYPE_STATIC_PORT) {
            count++;
            len += errp_put_udp_map_static_port(buf, &map->static_port);
        }
    }
    len += put_long(&ptr, count);

    // Scan for static ranges
    count = 0;
    ptr = skip_long(buf);
    for (i=0; i<num_maps; i++) {
        if (map->type == ERRP_UDP_MAP_TYPE_STATIC_RANGE) {
            count++;
            len += errp_put_udp_map_static_range(buf, &map->static_range);
        }
    }
    len += put_long(&ptr, count);

    // Scan for dynamic ranges
    count = 0;
    ptr = skip_long(buf);
    for (i=0; i<num_maps; i++) {
        if (map->type == ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE) {
            count++;
            len += errp_put_udp_map_dynamic_range(buf, &map->dynamic_range);
        }
    }
    len += put_long(&ptr, count);

    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_UDP_MAP, len);
    return len;
}


//! Write Interface ID attribute
int errp_put_attr_port_id (char **buf, uint32 port_id)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_PORT_ID, sizeof(long));
    len += put_long(buf, port_id);
    return len;
}


//! Write QAM Capability attribute
int errp_put_attr_qam_cap (char **buf, const errp_qam_cap_t *cap)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_short(buf, *(uint16*)(&cap->chan_bw));
    len += put_short(buf, *(uint16*)(&cap->j83));
    len += put_long(buf, *(uint32*)(&cap->interleaver));
    len += put_long(buf, *(uint32*)(&cap->docsis_video));
    len += put_short(buf, *(uint16*)(&cap->modulation));
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_QAM_CAP, len);
    return len;
}


#if 0
//! Write ERRP UPDATE message
int errp_put_update_msg (char **buf, const errp_node_t *node)
{
    int len = 0;
    char* msg_hdr = errp_skip_msg_hdr(buf);

    for (i=0; i<num_attr; i++) {
        len += errp_put_attr(buf, &attr[i]);
    }

    errp_put_msg_hdr(&msg_hdr, ERRP_UPDATE, len);
    return len;
}
#endif


//! Write ERRP NOTIFICATION message
int errp_put_notification_msg (char **buf, uint32 error)
{
    int len = ERRP_MSG_HDR_SIZE;
    char* msg_hdr = errp_skip_msg_hdr(buf);
    len += put_long(buf, error);
    errp_put_msg_hdr(&msg_hdr, ERRP_NOTIFICATION, len);
    return len;
}


//! Write ERRP KEEPALIVE message
int errp_put_keepalive_msg (char **buf)
{
    return errp_put_msg_hdr(buf, ERRP_KEEPALIVE, ERRP_MSG_HDR_SIZE);
}



//! Initialize an ERRP node
void errp_node_init (errp_node_t *node)
{
    memset(node, 0, sizeof(errp_node_t));

    node->addr_family = ERRP_ADDR_FAMILY;
    node->app_protocol = ERRP_PROT_DYNAMIC;
    node->errp_version = ERRP_VERSION;

    node->errp_id = 0x12345678;
    node->hold_time = 6;
    node->addr_domain = 12;

    node->send_receive = ERRP_SEND_ONLY;
}

