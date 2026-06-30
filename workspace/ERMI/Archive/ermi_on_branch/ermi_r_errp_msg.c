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

boolean 
errp_format_update (ermi_r_neighbor_t *nbr, uchar *output, 
                    uint *len)
{
    // TBD
    (void)errp_fill_header(output, 3, ERRP_UPDATE); // variable len
    *len = 3;
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

