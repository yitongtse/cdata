/*
 *------------------------------------------------------------------
 * ermi_r_errp_msg.c
 *
 * ERRP protocol message processing functions ERMI-1 protocol
 *
 * Dec 2008, Dean Chen
 *
 * Copyright (c) 2008 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

#include COMP_INC(kernel, list.h)
#include COMP_INC(kernel, mgd_timers.h)
#include COMP_INC(idb, interface.h)
#include IOS_INC(h/logger.h)
#include "ermi_r_protocol_def.h"
#include "ermi_r_errp_fsm.h"
#include "ermi_r_param.h"

uchar errp_attrib_types[ERRP_ATTRIB_MAX_BIT+1] = {
    ERRP_WITHDRAWN_ROUTE,
    ERRP_REACHABLE_ROUTE,
    ERRP_NEXT_HOP_SERVER,
    ERRP_QAM_NAMES,
    ERRP_CAS_CAPABILITY,
    ERRP_TOTAL_BANDWIDTH,
    ERRP_AVAILABLE_BANDWIDTH,
    ERRP_COST,
    ERRP_EDGE_INPUT,
    ERRP_QAM_CHANNEL_CONFIGURATION,
    ERRP_UDP_MAP,
    ERRP_SERVICE_STATUS,
    ERRP_MAX_MPEG_FLOWS,
    ERRP_NEXT_HOP_ALTERNATE,
    ERRP_PORT_ID,
    ERRP_FIBER_NODE,
    ERRP_QAM_CAPABILITY,
    ERRP_INPUT_MAP
};

uchar*
errp_fill_header (uchar *buf_start, ushort total_length, uchar msg_type)
{
    uchar *buf = buf_start;

    ERRP_PUTSHORT(buf, total_length);
    buf += 2;
    *buf = msg_type;
    buf++;

    return(buf);
}

uchar*
errp_add_open_opt_param (ermi_r_neighbor_t *nbr, 
                         uchar *buf, ushort type)
{
    uint param_len = 0;
    uchar *param_start_ptr;
    ermi_r_master_t *master = nbr->master;

    param_start_ptr = buf;

    switch (type) {
        case ERRP_CAPABILITY_INFORMATION:
            buf += 4;
            /* capability code route_type_supported */
            PUTSHORT(buf, ERRP_CAP_CODE_ROUTE_TYPE_SUPPORTED);
            buf += 2;
            PUTSHORT(buf, 4);
            buf += 2;
            PUTSHORT(buf, master->addr_family);
            buf += 2;
            PUTSHORT(buf, master->app_protocol);
            buf += 2;

            /* capability code send_recv, 4 octet uint */
            PUTSHORT(buf, ERRP_CAP_CODE_SEND_RECEIVE);
            buf += 2;
            PUTSHORT(buf, 4);
            buf += 2;
            if (nbr->fsm_type == SENDER_FSM) {
                PUTLONG(buf, ERRP_SEND_RCV_CAP_SEND_ONLY); 
            } else {
                PUTLONG(buf, ERRP_SEND_RCV_CAP_BOTH); 
            }
            buf += 4;

            /* capability code errp_version */
            PUTSHORT(buf, ERRP_CAP_CODE_ERRP_VERSION);
            buf += 2;
            PUTSHORT(buf, 4);
            buf += 2;
            PUTLONG(buf, master->version_cap);
            buf += 4;

            break;
        case ERRP_STREAMING_ZONE:
            PUTSHORT(buf, ERRP_STREAMING_ZONE);
            buf += 2;
            PUTSHORT(buf, strlen(master->streaming_zone));
            buf += 2;
            memcpy(buf, &master->streaming_zone[0],
                   strlen(master->streaming_zone));
            buf += strlen(master->streaming_zone);
            break;
        case ERRP_COMPONENT_NAME:
            PUTSHORT(buf, ERRP_COMPONENT_NAME);
            buf += 2;
            PUTSHORT(buf, strlen(master->component_name));
            buf += 2;
            memcpy(buf, &master->component_name[0],
                   strlen(master->component_name));
            buf += strlen(master->component_name);
            break;
        case ERRP_VENDOR_SPECIFIC_STRING:
            PUTSHORT(buf, ERRP_VENDOR_SPECIFIC_STRING);
            buf += 2;
            PUTSHORT(buf, strlen(master->vendor_str));
            buf += 2;
            memcpy(buf, &master->vendor_str[0],
                   strlen(master->vendor_str));
            buf += strlen(master->vendor_str);
            break;
        default:
            return(buf);
            break;
    }
    /* fill in optional parameter length */
    PUTSHORT(param_start_ptr, type);
    param_len = buf - param_start_ptr - 4;
    PUTSHORT(param_start_ptr+2, param_len);

    return buf;
}

boolean
errp_get_buffer (uint len, uchar **buf)
{
    *buf = malloc(len);
    
    if (*buf) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * Given own node parameter, format an ERRP OPEN message
 *
 * Input: Own node paramter
 *
 * Output: Returns TRUE if successful, FALSE if not;
 *         modifies output as pointer to buffer to formatted OPEN msg
 *         modifies len as length of OPEN message
 */
boolean 
errp_format_open (ermi_r_neighbor_t *nbr, uchar *output, 
                  uint *len)
{
        ushort total_msg_len = 3;
        ushort total_opt_param_len = 0;
        uchar *total_opt_param_len_ptr;
        uchar *data, *data_start;
        ermi_r_master_t *master;

        data = output;
        data_start = data;

        data = errp_fill_header(data_start, 0, ERRP_OPEN);

        master = nbr->master;
	*data++ = master->version;
	*data++ = 0; /* reserved */
        ERRP_PUTSHORT(data, master->hold_time);
	data += 2;
	ERRP_PUTLONG(data, master->addr_domain );
	data += 4;
	ERRP_PUTLONG(data, master->errp_id );
	data += 4;

        /* check if we have any optional parameters */	
	ERRP_PUTSHORT(data, 0 ); /* by default */
	total_opt_param_len_ptr = data;
	data += 2;

        data = errp_add_open_opt_param(nbr, data, 
                              ERRP_CAPABILITY_INFORMATION);
        data = errp_add_open_opt_param(nbr, data, 
                              ERRP_STREAMING_ZONE);
        data = errp_add_open_opt_param(nbr, data, 
                              ERRP_COMPONENT_NAME);        
        data = errp_add_open_opt_param(nbr, data, 
                              ERRP_VENDOR_SPECIFIC_STRING);

        total_opt_param_len = data - total_opt_param_len_ptr - 2;
        PUTSHORT(total_opt_param_len_ptr, total_opt_param_len);
        total_msg_len = data - data_start;

        (void)errp_fill_header(data_start, total_msg_len, ERRP_OPEN);


        /* sanity check total len */
        if (total_msg_len > ERRP_MAXBYTES) 
            return FALSE;

        *len = total_msg_len;
        return TRUE;
}

int
errp_parse_cap_info (ermi_r_neighbor_t *nbr, uchar *buf, 
                     int cap_info_max_len)
{
    uchar *buf_cur;
    ushort cap_code, cap_len;

    buf_cur = buf;
    cap_code = ERRP_GETSHORT(buf_cur);
    buf_cur += 2;
    cap_len = ERRP_GETSHORT(buf_cur);
    buf_cur += 2;

    ERMI_R_DEBUG("ERMI-1 parsing cap code %d, cap len %d\n",
                cap_code, cap_len);
    switch (cap_code) {
        case ERRP_CAP_CODE_ROUTE_TYPE_SUPPORTED:
            nbr->addr_family = ERRP_GETSHORT(buf_cur);
            buf_cur += 2;
            nbr->addr_protocol = ERRP_GETSHORT(buf_cur);
            buf_cur += 2;
            break;
        case ERRP_CAP_CODE_SEND_RECEIVE:
            nbr->send_receive_cap = ERRP_GETLONG(buf_cur);
            buf_cur += 4;
            break;
        case ERRP_CAP_CODE_ERRP_VERSION:
            nbr->version_cap = ERRP_GETLONG(buf_cur);
            buf_cur += 4;
            break;
        default:
            break;
    }

    /* 4 bytes for cap_code and cap_len fields */
    if ((buf_cur - buf) != (cap_len + 4)) { 
        ERMI_R_DEBUG("ERMI-1 mistmatched capability info length");
    }
    return(buf_cur - buf);
}

/*
 * Parses an ERRP OPEN optional parameter
 * 
 * Input: buffer to start of an optional paramater; length limit
 *
 * Output: returns number of bytes parsed
 */
int 
errp_parse_open_opt_param (ermi_r_neighbor_t *nbr, uchar *buf, 
                           int max_len)
{
    uchar *buf_cur;
    ushort opt_param_type, opt_param_len, opt_param_cap_len;
    ushort parsed_cap_len = 0;

    buf_cur = buf;
    opt_param_type = ERRP_GETSHORT(buf_cur);
    buf_cur += 2;
    opt_param_len =  ERRP_GETSHORT(buf_cur);
    buf_cur += 2;

    ERMI_R_DEBUG("ERMI-1 parsing opt param type %d, len %d of %d\n",
                opt_param_type, opt_param_len + 4, max_len);
    if (opt_param_len > max_len) {
        /* malformed message, just return max_len */
        ERMI_R_DEBUG("ERMI-1 malformed opt param type %d, len %d\n",
                opt_param_type, opt_param_len);
        return(max_len);
    }
    switch (opt_param_type) {
        case ERRP_CAPABILITY_INFORMATION:
            while (opt_param_len > parsed_cap_len) {
                opt_param_cap_len = 
                  errp_parse_cap_info(nbr, buf_cur, opt_param_len);
                parsed_cap_len += opt_param_cap_len; 
                buf_cur += opt_param_cap_len;
            }
            break;
        case ERRP_STREAMING_ZONE:
            memcpy(&nbr->streaming_zone[0], buf_cur, opt_param_len);
            buf_cur += opt_param_len;
            break;
        case ERRP_COMPONENT_NAME:
            memcpy(&nbr->component_name[0], buf_cur, opt_param_len);
            buf_cur += opt_param_len;
            break;
        case ERRP_VENDOR_SPECIFIC_STRING:
            memcpy(&nbr->vendor_str[0], buf_cur, opt_param_len);
            buf_cur += opt_param_len;
            break;
        default:
            break;
    }

    return(buf_cur - buf);
}


/*
 * Parses an ERRP OPEN message
 *
 * Input: buffer to, and length of OPEN message
 *
 * Output: Returns TRUE if successful, FALSE if not;
 *         modifies nbr data structure to contain the parsed peer info
 */
boolean
errp_parse_open (ermi_r_neighbor_t *nbr, uchar *buf, uint len)
{
    ushort total_msg_len = 0;
    ushort total_opt_param_len = 0;
    uchar errp_msg_type;
    uchar *buf_cur, *opt_param_start;

    buf_cur = buf;
    /* parse header */
    total_msg_len = ERRP_GETSHORT(buf_cur);
    buf_cur += 2;
    if (total_msg_len != len) {
        ERMI_R_DEBUG("ERMI-1 parse OPEN msg length mismatch\n");
        return FALSE;
    }
    errp_msg_type = *buf_cur;
    buf_cur++;
    if (errp_msg_type != ERRP_OPEN) {
        ERMI_R_DEBUG("ERMI-1 parse OPEN msg type mismatch\n");
        return FALSE;
    }
    
    /* parse OPEN */
    nbr->version = *buf_cur;
    buf_cur++;
    /* skip reserved field */
    buf_cur++;
    nbr->hold_time = ERRP_GETSHORT(buf_cur);
    if (nbr->hold_time == 0) {
        /* no hold time */
        if (nbr->timer_stop_func) {
            nbr->timer_stop_func(nbr, ERRP_HOLD_TIMER_TYPE);
        }
    } else {
        if (nbr->hold_time < 3) {
            /* reject hold time less than 3 */
            return FALSE;
        }
        /* hold time should be the min of configed and recv'd */
        if (nbr->hold_time > nbr->master->hold_time) {
            nbr->hold_time = nbr->master->hold_time;
        }
        nbr->keep_alive_time = nbr->hold_time/3;

        /* start hold timer */
        if (nbr->timer_stop_func && nbr->timer_start_func) {
            nbr->timer_stop_func(nbr, ERRP_HOLD_TIMER_TYPE);
            nbr->timer_start_func(nbr, ERRP_HOLD_TIMER_TYPE,
                                  nbr->hold_time);
        }
    }
    buf_cur += 2;
    nbr->addr_domain = ERRP_GETLONG(buf_cur);
    buf_cur += 4;
    nbr->errp_id = ERRP_GETLONG(buf_cur);
    buf_cur += 4;

    total_opt_param_len = ERRP_GETSHORT(buf_cur);
    buf_cur += 2;
    opt_param_start = buf_cur;

    /* parse optional parameters */
    if ((total_opt_param_len > 0) && 
        (total_opt_param_len < ERRP_MAXBYTES)) {
        while ((buf_cur - opt_param_start) < total_opt_param_len) {
            buf_cur += 
              errp_parse_open_opt_param(nbr, buf_cur, 
                                        total_opt_param_len);
        }
    }
    
    ERMI_R_DEBUG("ERMI-1 parsed OPEN, sendrcv %d\n", 
                nbr->send_receive_cap);
    return TRUE;
}


/*
 * Given own node parameter, format an ERRP KEEPALIVE message
 *
 * Input: Own node paramter
 *
 * Output: Returns TRUE if successful, FALSE if not;
 *         modifies output as pointer to buffer to formatted KEEPALIVE msg
 *         modifies len as length of KEEPALIVE message
 */
boolean 
errp_format_keepalive (ermi_r_neighbor_t *nbr, uchar *output, 
                       uint *len)
{
    if (!output)
        return FALSE;

    (void)errp_fill_header(output, 3, ERRP_KEEPALIVE);
    *len = 3;
    return TRUE;
}

boolean 
errp_parse_keepalive (ermi_r_neighbor_t *nbr, uchar *output,
                      uint len)
{
    /* restart hold timer */
    if (nbr->hold_time) {
        if (nbr->timer_stop_func && nbr->timer_start_func) {
            nbr->timer_stop_func(nbr, ERRP_HOLD_TIMER_TYPE);
            nbr->timer_start_func(nbr, ERRP_HOLD_TIMER_TYPE,
                                  nbr->hold_time);
        }

    }
    return TRUE;
}

boolean 
errp_format_notification (ermi_r_neighbor_t *nbr, uchar *output, 
                       uint *len)
{
    // TBD
    (void)errp_fill_header(output, 3, ERRP_NOTIFICATION); // variable len
    *len = 3;
    return TRUE;
}

boolean 
errp_parse_notification (ermi_r_neighbor_t *nbr, uchar *output, 
                         uint len)
{
    // TBD
    /* close connection, terminate session */
    return TRUE;
}

uchar *
errp_fill_reachable_route_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0, name_len = 0;
    char qam_group_name[ERRP_MAX_NAME_LEN] = "DFLT_GROUP_NAME";
    // char *qam_group_name;
    char flag = 0; /* well known flag */

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_REACHABLE_ROUTE;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    /* route is the QAM group name */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_GROUP_NAME, nbr, &qam_group_name);
    ERRP_PUTSHORT(buf_cur, nbr->master->addr_family); /* addr family */
    buf_cur += 2;
    ERRP_PUTSHORT(buf_cur, nbr->master->app_protocol); /* app protocol */
    buf_cur += 2;
    name_len = strlen(qam_group_name);
    ERRP_PUTSHORT(buf_cur, name_len);
    buf_cur += 2;
    memcpy(buf_cur, qam_group_name, name_len);
    buf_cur += name_len;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_withdrawn_route_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0, name_len = 0;
    char qam_group_name[ERRP_MAX_NAME_LEN] = "DFLT_GROUP_NAME";
    // char *qam_group_name;
    char flag = 0; /* well known flag */

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag; /* well known flag */
    buf_cur++;
    *buf_cur = ERRP_WITHDRAWN_ROUTE;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    /* route is the QAM group name */
    // TBD
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_GROUP_NAME, nbr, &qam_group_name);
    ERRP_PUTSHORT(buf_cur, nbr->master->addr_family); /* addr family */
    buf_cur += 2;
    ERRP_PUTSHORT(buf_cur, nbr->master->app_protocol); /* app protocol */
    buf_cur += 2;
    name_len = strlen(qam_group_name);
    ERRP_PUTSHORT(buf_cur, name_len);
    buf_cur += 2;
    memcpy(buf_cur, qam_group_name, name_len);
    buf_cur += name_len;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_next_hop_server_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */    

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_NEXT_HOP_SERVER;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    /* address domain */
    ERRP_PUTLONG(buf_cur, nbr->master->addr_domain);
    buf_cur += 4;
    
    /* signaling IP addr */
    // TBD
    // ERRP_PUTSHORT(buf_cur, strlen(nbr->master->signalling_ip));
    ERRP_PUTSHORT(buf_cur, strlen("1.1.1.2"));
    buf_cur += 2;
    // memcpy(buf_cur, nbr->master->signalling_ip, strlen(nbr->master->signalling_ip));
    memcpy(buf_cur, "1.1.1.2", strlen("1.1.1.2"));
    buf_cur += strlen("1.1.1.2");

    /* streaming zone */
    ERRP_PUTSHORT(buf_cur, strlen(nbr->master->streaming_zone));
    buf_cur += 2;
    memcpy(buf_cur, nbr->master->streaming_zone, strlen(nbr->master->streaming_zone));
    buf_cur += strlen(nbr->master->streaming_zone);

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_next_hop_server_alt_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */  
    int i, num_alternates = 2;
    char next_hop_alt[ERRP_MAX_NAME_LEN] = "1.1.1.3";

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_NEXT_HOP_ALTERNATE;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    /* alt signaling IP addr */
    // TBD
    // num_alternates = ?;
    ERRP_PUTSHORT(buf_cur, num_alternates);
    buf_cur += 2;
    for (i = 0; i < num_alternates; i++) {
        // nbr->errp_get_qam_param(ERRP_PARAM_ALT_SIGNALING_ADDR, nbr, next_hop_alt);
        ERRP_PUTSHORT(buf_cur, strlen(next_hop_alt));
        buf_cur += 2;
        memcpy(buf_cur, next_hop_alt, strlen(next_hop_alt));
        buf_cur += strlen(next_hop_alt);
    }

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_fiber_node_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */  
    int i, num_fiber_nodes = 2;
    char fiber_node_name[ERRP_MAX_NAME_LEN] = "DFLT FIBERNODENAME";

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_FIBER_NODE;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // TBD
    // num_fiber_nodes = ?;
    for (i = 0; i < num_fiber_nodes; i++) {
        // nbr->errp_get_qam_param(ERRP_QAM_FIBER_NODE_NAME, nbr, fiber_node_name);
        ERRP_PUTSHORT(buf_cur, strlen(fiber_node_name));
        buf_cur += 2;
        memcpy(buf_cur, fiber_node_name, strlen(fiber_node_name));
        buf_cur += strlen(fiber_node_name);
    }

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_input_map_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */  
    int i;
    int32 num_input = 2;
    char input_addr[ERRP_MAX_NAME_LEN] = "1.1.1.5";

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_INPUT_MAP;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // TBD
    // num_input = ?;
    ERRP_PUTLONG(buf_cur, num_input);
    buf_cur += 4;
    for (i = 0; i < num_input; i++) {
        // nbr->errp_get_qam_param(ERRP_QAM_INPUT_MAP, nbr, input_addr);
        ERRP_PUTSHORT(buf_cur, strlen(input_addr));
        buf_cur += 2;
        memcpy(buf_cur, input_addr, strlen(input_addr));
        buf_cur += strlen(input_addr);
    }

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_qam_names_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    char qam_name[ERRP_MAX_NAME_LEN] = "DFLT QAM NAME";

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_QAM_NAMES;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    /* QAM name, one per update, if from EQAM to ERM (speaker mode) */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_NAME, nbr, &qam_name);
    ERRP_PUTSHORT(buf_cur, strlen(qam_name));
    buf_cur += 2;
    memcpy(buf_cur, qam_name, strlen(qam_name));
    buf_cur += strlen(qam_name);
    
    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_total_bandwidth_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    uint32 total_bw = 30000; /* 30000 Kbps */

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_TOTAL_BANDWIDTH;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_TOTAL_BW, nbr, &total_bw);
    ERRP_PUTLONG(buf_cur, total_bw);
    buf_cur += 4;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_avail_bandwidth_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    uint32 avail_bw = 30000; /* 30000 Kbps */

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_AVAILABLE_BANDWIDTH;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_AVAIL_BW, nbr, &avail_bw);
    ERRP_PUTLONG(buf_cur, avail_bw);
    buf_cur += 4;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_port_id_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    uint32 port_id;

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_PORT_ID;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_RF_PORT_ID, nbr, &port_id);
    ERRP_PUTLONG(buf_cur, port_id);
    buf_cur += 4;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_service_status_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    uint32 service_status;

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_SERVICE_STATUS;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_SERVICE_STATUS, nbr, &service_status);
    ERRP_PUTLONG(buf_cur, service_status);
    buf_cur += 4;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_max_mpeg_flows_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    uint32 max_mpeg_flows = 10000;

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_MAX_MPEG_FLOWS;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_MAX_MPEG_FLOWS, nbr, &max_mpeg_flows);
    ERRP_PUTLONG(buf_cur, max_mpeg_flows);
    buf_cur += 4;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_cost_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    uchar cost = 0;

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_COST;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_COST, nbr, &cost);
    *buf_cur = cost;
    buf_cur++;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_edge_input_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    // N inputs, each needs:
    // subnet mask (4 bytes); input n host ip (str); input n interface id (4 bytes); input n max bw (4 bytes); input n input group name (str)
    uint32 subnet_mask = 0xFFFF0000, intf_id = 1234 /* unique, idb? */, max_bw = 1000000 /* Kbps */;
    char input_host_addr[ERRP_MAX_NAME_LEN] = "1.1.1.1";
    char input_group_name[ERRP_MAX_NAME_LEN] = "DFLT GROUP NAME";

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_EDGE_INPUT;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // TBD, for all QAM domains
    // for_all_interface_id() {
    // nbr->id = intf_id;
    // nbr->errp_get_qam_param(ERRP_PARAM_EDGE_INPUT_SUBNET_MASK, nbr, &subnet_mask);
    ERRP_PUTLONG(buf_cur, subnet_mask);
    buf_cur += 4;
    // nbr->errp_get_qam_param(ERRP_PARAM_EDGE_INPUT_ADDR, nbr, input_host_addr);
    ERRP_PUTSHORT(buf_cur, strlen(input_host_addr));
    buf_cur += 2;
    memcpy(buf_cur, input_host_addr, strlen(input_host_addr));
    buf_cur += strlen(input_host_addr);
    // nbr->errp_get_qam_param(ERRP_PARAM_EDGE_INPUT_INTF_ID, nbr, &intf_id);
    ERRP_PUTLONG(buf_cur, intf_id);
    buf_cur += 4;
    // nbr->errp_get_qam_param(ERRP_PARAM_EDGE_INPUT_BW, nbr, &max_bw);
    ERRP_PUTLONG(buf_cur, max_bw);
    buf_cur += 4;
    // nbr->errp_get_qam_param(ERRP_PARAM_EDGE_INPUT_GROUP_NAME, nbr, input_group_name);
    ERRP_PUTSHORT(buf_cur, strlen(input_group_name));
    buf_cur += 2;
    memcpy(buf_cur, input_host_addr, strlen(input_group_name));
    buf_cur += strlen(input_group_name);
    // } /* for_all_interface_id */

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_qam_channel_configuration_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    // frequency (hz, 4 bytes); mod mode (1 byte); interleaver (1 byte); tsid (2 bytes); annex (1 byte); chan width (1 byte);
    uint32 freq = 527000000;
    uchar mod_mode = 4, interleaver = 7, annex = 0, chan_width = 0;
    ushort qam_tsid = 1234;

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_QAM_CHANNEL_CONFIGURATION;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_FREQ, nbr, &freq);
    ERRP_PUTLONG(buf_cur, freq);
    buf_cur += 4;
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_MOD_MODE, nbr, &mod_mode);
    *buf_cur = mod_mode;
    buf_cur++;
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_INTERLEAVER, nbr, &interleaver);
    *buf_cur = interleaver;
    buf_cur++;
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_TSID, nbr, &qam_tsid);
    ERRP_PUTSHORT(buf_cur, qam_tsid);
    buf_cur += 2;
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_ANNEX, nbr, &annex);
    *buf_cur = annex;
    buf_cur++;
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_CHAN_WIDTH, nbr, &chan_width);
    *buf_cur = chan_width;
    buf_cur++;
    ERRP_PUTSHORT(buf_cur, 0); /* reserved */
    buf_cur += 2;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_qam_capability_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    // chan_bw (2 bytes), j83 (2 bytes), interleaver (4 bytes), docsis_video_cap (4 bytes), modulation (2 bytes)
    ushort chan_bw = 256, j83 = 512, modulation = 512;
    uint32 interleaver = 4096, docsis_video_cap = 65536 /* video */;

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_QAM_CAPABILITY;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_CHAN_BW_CAP, nbr, &chan_bw);
    ERRP_PUTSHORT(buf_cur, chan_bw);
    buf_cur += 2;
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_J83_CAP, nbr, &j83);
    ERRP_PUTSHORT(buf_cur, j83);
    buf_cur += 2;

    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_INTERLEAVER_CAP, nbr, &interleaver);
    ERRP_PUTLONG(buf_cur, interleaver);
    buf_cur += 4;

    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_DOCSIS_VIDEO_CAP, nbr, &docsis_video_cap);
    ERRP_PUTLONG(buf_cur, docsis_video_cap);
    buf_cur += 4;

    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_MODULATION_CAP, nbr, &modulation);
    ERRP_PUTSHORT(buf_cur, modulation);
    buf_cur += 2;

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar *
errp_fill_qam_udp_map_attrib (ermi_r_neighbor_t *nbr, ushort tsid, uchar *buf) {
    uchar *buf_cur, *len_ptr;
    ushort attrib_len = 0;
    char flag = 0; /* well known flag */
    static_udp_map s_map;
    static_range_udp_map sr_map;
    dynamic_range_udp_map dr_map;
    int i;

    if (!nbr)
        return(buf);

    buf_cur = buf;

    /* attribute type and len */
    *buf_cur = flag;
    buf_cur++;
    *buf_cur = ERRP_UDP_MAP;
    buf_cur++;
    len_ptr = buf_cur;
    buf_cur += 2;

    /* attribute value */
    /* static udp map */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_STATIC_UDP_MAP, nbr, &s_map);
    // TBD
    s_map.num_static_port = 2;
    s_map.udp_port[0] = 2048;
    s_map.mpeg_prog[0] = 4096;
    s_map.udp_port[1] = 2050;
    s_map.mpeg_prog[1] = 95;
    ERRP_PUTLONG(buf_cur, s_map.num_static_port);
    buf_cur += 4;
    for (i = 0; i < s_map.num_static_port; i++) {
        ERRP_PUTSHORT(buf_cur, s_map.udp_port[i]);
        buf_cur += 2;
        ERRP_PUTSHORT(buf_cur, s_map.mpeg_prog[i]);
        buf_cur += 2;
    }

    /* static range udp map */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_STATIC_RANGE_UDP_MAP, nbr, &sr_map);
    // TBD
    sr_map.num_static_port_ranges = 2;
    sr_map.starting_udp_port[0] = 1024;
    sr_map.starting_mpeg_prog[0] = 16384;
    sr_map.count[0] = 1000;
    sr_map.starting_udp_port[1] = 2048;
    sr_map.starting_mpeg_prog[1] = 17384;
    sr_map.count[1] = 2000;
    ERRP_PUTLONG(buf_cur, sr_map.num_static_port_ranges);
    buf_cur += 4;
    for (i = 0; i < sr_map.num_static_port_ranges; i++) {
        ERRP_PUTSHORT(buf_cur, sr_map.starting_udp_port[i]);
        buf_cur += 2;
        ERRP_PUTSHORT(buf_cur, sr_map.starting_mpeg_prog[i]);
        buf_cur += 2;
        ERRP_PUTLONG(buf_cur, sr_map.count[i]);
        buf_cur += 4;
    }

    /* dynamic range udp map */
    // nbr->errp_get_qam_param(ERRP_PARAM_QAM_DYNAMIC_RANGE_UDP_MAP, nbr, &dr_map);
    // TBD
    dr_map.num_dynamic_port_ranges = 1;
    dr_map.starting_udp_port[0] = 32768;
    dr_map.count[0] = 2000;
    ERRP_PUTLONG(buf_cur, dr_map.num_dynamic_port_ranges);
    buf_cur += 4;
    for (i = 0; i < dr_map.num_dynamic_port_ranges; i++) {
        ERRP_PUTSHORT(buf_cur, dr_map.starting_udp_port[i]);
        buf_cur += 2;
        ERRP_PUTSHORT(buf_cur, dr_map.count[i]);
        buf_cur += 2;
    }

    /* fill in attribute length */
    attrib_len = buf_cur - buf - 4;
    ERRP_PUTSHORT(len_ptr, attrib_len);

    return(buf_cur);
}

uchar*
errp_add_update_attribute (ermi_r_neighbor_t *nbr, uchar attrib_type,
                           uchar *buf)
{
    uchar *output;
    ushort tsid;

    // TBD
    output = buf;
    switch (attrib_type) {
        case ERRP_WITHDRAWN_ROUTE:
            buginf("\n=== formatting withdrawn route");
            tsid = (ushort)nbr->id;
            output = errp_fill_withdrawn_route_attrib(nbr, tsid, buf);
            break;
        case ERRP_REACHABLE_ROUTE:
            buginf("\n=== formatting reachable route");
            tsid = (ushort)nbr->id;
            output = errp_fill_reachable_route_attrib(nbr, tsid, buf);
            break;
        case ERRP_NEXT_HOP_SERVER:
            buginf("\n=== formatting nexthopserver");
            tsid = (ushort)nbr->id;
            output = errp_fill_next_hop_server_attrib(nbr, tsid, buf);
            break;
        case ERRP_QAM_NAMES:
            buginf("\n=== formatting qam names");
            tsid = (ushort)nbr->id;
            output = errp_fill_qam_names_attrib(nbr, tsid, buf);
            break;
        case ERRP_CAS_CAPABILITY:
            // TBD, not supported by RFGW-10
            buginf("\n=== formatting cas cap");
            break;
        case ERRP_TOTAL_BANDWIDTH:
            buginf("\n=== formatting total bw");
            tsid = (ushort)nbr->id;
            output = errp_fill_total_bandwidth_attrib(nbr, tsid, buf);
            break;
        case ERRP_AVAILABLE_BANDWIDTH:
            buginf("\n=== formatting avail bw");
            tsid = (ushort)nbr->id;
            output = errp_fill_avail_bandwidth_attrib(nbr, tsid, buf);
            break;
        case ERRP_COST:
            // TBD, may not need
            buginf("\n=== formatting cost");
            tsid = (ushort)nbr->id;
            output = errp_fill_cost_attrib(nbr, tsid, buf);
            break;
        case ERRP_EDGE_INPUT:
            buginf("\n=== formatting edge input");
            output = errp_fill_edge_input_attrib(nbr, tsid, buf);
            break;
        case ERRP_QAM_CHANNEL_CONFIGURATION:
            buginf("\n=== formatting chan cfg");
            tsid = (ushort)nbr->id;
            output = errp_fill_qam_channel_configuration_attrib(nbr, tsid, buf);
            break;
        case ERRP_UDP_MAP:
            buginf("\n=== formatting udp map");
            tsid = (ushort)nbr->id;
            output = errp_fill_qam_udp_map_attrib(nbr, tsid, buf);
            break;
        case ERRP_SERVICE_STATUS:
            buginf("\n=== formatting service status");
            tsid = (ushort)nbr->id;
            output = errp_fill_service_status_attrib(nbr, tsid, buf);
            break;
        case ERRP_MAX_MPEG_FLOWS:
            buginf("\n=== formatting mpeg flows");
            output = errp_fill_max_mpeg_flows_attrib(nbr, tsid, buf);
            break;
        case ERRP_NEXT_HOP_ALTERNATE:
            buginf("\n=== formatting hop alternate");
            output = errp_fill_next_hop_server_alt_attrib(nbr, tsid, buf);
            break;
        case ERRP_PORT_ID:
            buginf("\n=== formatting port id");
            tsid = (ushort)nbr->id;
            output = errp_fill_port_id_attrib(nbr, tsid, buf);
            break;
        case ERRP_FIBER_NODE:
            buginf("\n=== formatting fiber node");
            tsid = (ushort)nbr->id;
            output = errp_fill_fiber_node_attrib(nbr, tsid, buf);
            break;
        case ERRP_QAM_CAPABILITY:
            buginf("\n=== formatting qam cap");
            tsid = (ushort)nbr->id;
            output = errp_fill_qam_capability_attrib(nbr, tsid, buf);
            break;
        case ERRP_INPUT_MAP:
            buginf("\n=== formatting input map");
            tsid = (ushort)nbr->id;
            output = errp_fill_input_map_attrib(nbr, tsid, buf);
            break;
        default:
            buginf("\n=== formatting unknown attrib type %d", attrib_type);
            break;
    }
    return(output);
}

boolean 
errp_format_update (ermi_r_neighbor_t *nbr, uchar *output,
                    uint *len)
{
    uchar *data;
    ulong bitset;
    int i;

    data = errp_fill_header(output, 3, ERRP_UPDATE); 

    // TBD, different handling for speaker vs. listener
    /* loop through attributes that need to be encoded */
    while (BITMASK_FIND_FIRST_SET((uchar *)nbr->attrib_bitmask,
                                  ERRP_ATTRIB_MAX_BIT+1, &bitset)) {
        BITMASK_CLEAR(nbr->attrib_bitmask, bitset);
        buginf("\n=== cleared %d th bit, attrib type %d", bitset, errp_attrib_types[bitset]);
        data = errp_add_update_attribute(nbr, errp_attrib_types[bitset], data);
    }

    *len = data - output;
    (void)errp_fill_header(output, *len, ERRP_UPDATE); 
    buginf("\n=== filled %d bytes", *len);
    for (i=0;i<*len;i++) {
        if (i%4 == 0) {
            buginf("\n");
        }
        buginf("%2x ",output[i]);
    }
    return TRUE;
}

boolean 
errp_parse_update (ermi_r_neighbor_t *nbr, uchar *output, 
                   uint len)
{
    /* as ERRP speaker, no need to parse UPDATE msg */
    /* restart hold timer */
    if (nbr->hold_time) {
        if (nbr->timer_stop_func && nbr->timer_start_func) {
            nbr->timer_stop_func(nbr, ERRP_HOLD_TIMER_TYPE);
            nbr->timer_start_func(nbr, ERRP_HOLD_TIMER_TYPE,
                                  nbr->hold_time);
        }

    }
    return TRUE;
}

void
ermi_r_parse_dispatch_msg (ermi_r_neighbor_t *nbr, uchar *buf, uint len)
{
    ushort errp_msg_len;
    uchar errp_msg_type;
    uchar *buf_start, *buf_cur;

    if (len < ERRP_HEADERBYTES) {
        return;
    }
    
    buf_start = buf;
    buf_cur = buf;
    errp_msg_len = ERRP_GETSHORT(buf_start);
    buf_cur += 2;
    if (errp_msg_len != len) {
        return;
    }
   
    errp_msg_type = *buf_cur;
    buf_cur++;

    switch (errp_msg_type) {
        case ERRP_OPEN:
            ERMI_R_DEBUG("PARSING OPEN\n");
            if (errp_parse_open(nbr, buf_start, errp_msg_len)) {
                ermi_r_event(nbr, E_OPEN_RCVD, NULL);
            } else {
                // TBD, error codes
                ermi_r_event(nbr, E_MSG_ERROR, NULL);
            }
            break;
        case ERRP_UPDATE:
            ERMI_R_DEBUG("PARSING UPDATE\n");
            if (errp_parse_update(nbr, buf_start, errp_msg_len)) {
                ermi_r_event(nbr, E_UPDATE_RCVD, NULL);
            }  else {
                // TBD, error codes
                ermi_r_event(nbr, E_MSG_ERROR, NULL);
            }
            break;
        case ERRP_NOTIFICATION:
            ERMI_R_DEBUG("PARSING NOTIFICATION\n");
            if (errp_parse_notification(nbr, buf_start, 
                                        errp_msg_len)) {

                // TBD
            }
            ermi_r_event(nbr, E_NOTIF_RCVD, NULL);
            break;
        case ERRP_KEEPALIVE:
            ERMI_R_DEBUG("PARSING KEEALIVE\n");
            if (errp_parse_keepalive(nbr, buf_start, errp_msg_len)) {
                ermi_r_event(nbr, E_KEEPALIVE_RCVD, NULL);
            } else {
                // TBD, error codes
                ermi_r_event(nbr, E_MSG_ERROR, NULL);
            }
            break;
        default:
            break;
    }
}

