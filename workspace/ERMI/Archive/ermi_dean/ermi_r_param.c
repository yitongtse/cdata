/*
 *------------------------------------------------------------------
 * ermi_r_param.c
 *
 * Wrapper functions to get QAM parameters
 *
 * Jan 2009, Dean Chen
 *
 * Copyright (c) 2009 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

#include COMP_INC(target-cpu, posix_types.h) 
#include COMP_INC(target-cpu, cpu_types.h) 
#include COMP_INC(kernel, ios_macros.h) 
#include COMP_INC(kernel, queue.h) 
#include COMP_INC(kernel, list.h)
#include COMP_INC(posix, errno.h)
#include COMP_INC(kernel, mgd_timers.h)
#include COMP_INC(idb, interface.h)
#include COMP_INC(rfgw/sup/qam, rfgw_lcc.h)
#include COMP_INC(rfgw, rfgw_ipc_config.h)
#include IOS_INC(tcp/tcp.h)
#include IOS_INC(socket/socket.h)
#include IOS_INC(socket/sock_internal.h)
#include IOS_INC(h/logger.h)
#include "ermi_cli_util.h"
#include "ermi_r_protocol_def.h"
#include "ermi_r_errp_fsm.h"
#include "ermi_r_main.h"
#include "ermi_r_io.h"
#include "ermi_r_param.h"

typedef void * (*errp_get_qam_param_func)(uint param_type, 
                                          struct ermi_r_neighbor_ *nbr, 
                                          void *param_value);

boolean errp_get_param_qam_annex (struct ermi_r_neighbor_ *nbr, uchar *annex)
{
    rfgw_qam_annex_t annex_type;
    boolean rc = FALSE;

    if (lcc_get_qam_annex_type_field(nbr->qam_slot, nbr->qam_port, 
                                     nbr->qam_chan, &annex_type)) {
        switch (annex_type) {
            case RFGW_ANNEX_A:
                *annex = ERRP_QAM_CH_CFG_ANNEX_A;
                rc = TRUE;
                break;
            case RFGW_ANNEX_B:
                *annex = ERRP_QAM_CH_CFG_ANNEX_B;
                rc = TRUE;
                break;
            case RFGW_ANNEX_C:
                *annex = ERRP_QAM_CH_CFG_ANNEX_C;
                rc = TRUE;
                break;
            default:
                break;
        }
    } 
    return(rc);
}

boolean errp_get_param_qam_chan_width (struct ermi_r_neighbor_ *nbr, uchar *chan_width)
{
    rfgw_qam_annex_t annex_type;
    boolean rc = FALSE;

    if (lcc_get_qam_annex_type_field(nbr->qam_slot, nbr->qam_port, 
                                     nbr->qam_chan, &annex_type)) {
        switch (annex_type) {
            case RFGW_ANNEX_A:
                *chan_width = ERRP_QAM_CH_CFG_BW_8MHZ;
                rc = TRUE;
                break;
            case RFGW_ANNEX_B:
            case RFGW_ANNEX_C:
                *chan_width = ERRP_QAM_CH_CFG_BW_6MHZ;
                rc = TRUE;
                break;
            default:
                break;
        }
    } 
    return(rc);
}

boolean errp_get_param_qam_interleaver (struct ermi_r_neighbor_ *nbr, uchar *interleaver)
{
    uint8 fec_i, fec_j;

    if (lcc_get_qam_fec_i_field(nbr->qam_slot, nbr->qam_port, 
                                nbr->qam_chan, &fec_i) &&
        lcc_get_qam_fec_j_field(nbr->qam_slot, nbr->qam_port, 
                                nbr->qam_chan, &fec_j) ) {
        if (fec_i == 8) {
            if (fec_j == 16) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I8_J16;
                return(TRUE);
            }
        }
        if (fec_i == 12) {
            if (fec_j == 7) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I12_J7;
                return(TRUE);
            }
        }
        if (fec_i == 16) {
            if (fec_j == 8) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I16_J8;
                return(TRUE);
            }
        }
        if (fec_i == 32) {
            if (fec_j == 4) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I32_J4;
                return(TRUE);
            }
        }
        if (fec_i == 64) {
            if (fec_j == 2) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I64_J2;
                return(TRUE);
            }
        }
        if (fec_i == 128) {
            if (fec_j == 1) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J1;
                return(TRUE);
            }
            if (fec_j == 2) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J2;
                return(TRUE);
            }
            if (fec_j == 3) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J3;
                return(TRUE);
            }
            if (fec_j == 4) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J4;
                return(TRUE);
            }
            if (fec_j == 5) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J5;
                return(TRUE);
            }
            if (fec_j == 6) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J6;
                return(TRUE);
            }
            if (fec_j == 7) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J7;
                return(TRUE);
            }
            if (fec_j == 8) {
                *interleaver = ERRP_QAM_CH_CFG_INTRLVR_I128_J8;
                return(TRUE);
            }
        }
    } 
    return(FALSE);
}

boolean errp_get_param_qam_mod_mode (struct ermi_r_neighbor_ *nbr, uchar *mod_mode)
{
    uint32 format;
    boolean rc = FALSE;

    if (lcc_get_qam_format_field(nbr->qam_slot, nbr->qam_port, 
                                     nbr->qam_chan, &format)) {
        switch (format) {
            case RFGW_MB_DEFAULT_QAM_FORMAT64:
                *mod_mode = ERRP_QAM_CH_CFG_MOD_MODE_64QAM;
                rc = TRUE;
                break;
            case RFGW_MB_DEFAULT_QAM_FORMAT256:
                *mod_mode = ERRP_QAM_CH_CFG_MOD_MODE_256QAM;
                rc = TRUE;
                break;
            default:
                break;
        }
    } 
    return(rc);
}

boolean errp_get_param_qam_freq (struct ermi_r_neighbor_ *nbr, uint32 *freq)
{
    if (lcc_get_qam_freq_field(nbr->qam_slot, nbr->qam_port, 
                               nbr->qam_chan, freq)) {
        return(TRUE);
    } 
    return(FALSE);
}

boolean errp_get_param_sz_name (struct ermi_r_neighbor_ *nbr, char *sz_name)
{
    char *name;

    if (name = rfgw_get_stream_zone(nbr->server_group)) {
        sstrncpy(sz_name, name, (strlen(name) <= ERRP_MAX_NAME_LEN)?
                strlen(name):ERRP_MAX_NAME_LEN);
        return(TRUE);
    }
    return(FALSE);
}

static void
errp_convert_ipaddr_to_str (char *ipaddr_str, ipaddrtype ipaddr)
{
    struct in_addr addrs;

    addrs.s_addr = ipaddr; 
    sstrncpy( ipaddr_str, (char*) socket_inet_ntoa(addrs), ERRP_MAX_NAME_LEN);
}

boolean errp_get_param_signaling_addr (struct ermi_r_neighbor_ *nbr,
                                       char *mgmt_ip_addr_str)
{
    rfgw_mgmt_conn_p mc_ptr;
    list_element *element;
    ipaddrtype mgmt_ip_addr;
    boolean ip_addr_found = FALSE;

    FOR_ALL_DATA_IN_LIST(&nbr->server_group->mgmt_conn_list, 
                         element, mc_ptr) {
        assert(mc_ptr);
        if (mgmt_ip_addr = mc_ptr->mgmt_conn_info.mgmt_ip_addr) {
            ip_addr_found = TRUE;
            break;
        }
    }
    errp_convert_ipaddr_to_str(mgmt_ip_addr_str, mgmt_ip_addr);

    return(ip_addr_found);
}

boolean errp_get_param_qam_group_name (struct ermi_r_neighbor_ *nbr,
                                       char *group_name) {
    rfgw_ermi_qam_grp_t *qam_group = NULL;
    uint logical_portid, logical_qamid;
    uint abs_qam_id, qam_block, qam_bit;
    list_element *curr_elem;

    logical_portid = LCC_LOGICAL_PORTID(nbr->qam_slot, nbr->qam_port);
    logical_qamid  = LCC_LOGICAL_QAMID(nbr->ch);

    /* Convert the QAM interfaces to absolute QAM ID */
    abs_qam_id = SPQ_TO_OFFSET(nbr->qam_slot, logical_portid, 
                               logical_qamid);
    qam_block = abs_qam_id/NUMB_MB_QAM_PER_QAM_BLOCK;

    /* Each QAM interface is a bit in the qam_bitmap 
       index in the qam_bitmap array is the qam_block the bit
       belongs to */
    qam_bit = abs_qam_id % NUMB_MB_QAM_PER_QAM_BLOCK;

    // iterate thru master list of group_names,
    // until finds the group with qam bit set, return qam name str 
    FOR_ALL_DATA_IN_LIST(&rfgw_ermi_params.qam_grp_list, 
                         curr_elem, qam_group) {
        /* QAM group name already exists */
        if (qam_group->qam_bitmap[qam_block] & (0x1 << qam_bit)) {
            /* found qam group for the given qam */
            sstrncpy(group_name, qam_group->group_name, ERRP_MAX_NAME_LEN);
            return(TRUE);
        }        
    }
    return(FALSE);
}

boolean errp_get_param_qam_name (struct ermi_r_neighbor_ *nbr, char *qam_name)
{
    char *name;

    if (snprintf(qam_name, ERRP_MAX_NAME_LEN, "qam%d/%d.%d", 
                 nbr->qam_slot, nbr->qam_port, nbr->qam_chan)) {
        return(TRUE);
    }
    return(FALSE);
}

boolean errp_get_param_qam_chan_bw_cap (struct ermi_r_neighbor_ *nbr, 
                                        ushort *chan_bw)
{
    rfgw_qam_annex_t annex_type;
    boolean rc = FALSE;

    if (lcc_get_qam_annex_type_field(nbr->qam_slot, nbr->qam_port, 
                                     nbr->qam_chan, &annex_type)) {
        switch (annex_type) {
            case RFGW_ANNEX_A:
                *chan_bw = (0x1 << 10);
                rc = TRUE;
                break;
            case RFGW_ANNEX_B:
            case RFGW_ANNEX_C:
                *chan_bw = (0x1 << 8);
                rc = TRUE;
                break;
            default:
                break;
        }
    } 
    return(FALSE);
}

boolean errp_get_param_qam_mod_cap (struct ermi_r_neighbor_ *nbr, 
                                    ushort *mod)
{
    uint32 format;
    boolean rc = FALSE;

    if (lcc_get_qam_format_field(nbr->qam_slot, nbr->qam_port, 
                                     nbr->qam_chan, &format)) {
        switch (format) {
            case RFGW_MB_DEFAULT_QAM_FORMAT64:
                *mod = (0x1 << 8);
                rc = TRUE;
                break;
            case RFGW_MB_DEFAULT_QAM_FORMAT256:
                *mod = (0x1 << 9);
                rc = TRUE;
                break;
            default:
                break;
        }
    } 
    return(FALSE);
}

boolean errp_get_param_qam_j83_cap (struct ermi_r_neighbor_ *nbr, 
                                    ushort *j83)
{
    rfgw_qam_annex_t annex_type;
    boolean rc = FALSE;

    if (lcc_get_qam_annex_type_field(nbr->qam_slot, nbr->qam_port, 
                                     nbr->qam_chan, &annex_type)) {
        switch (annex_type) {
            case RFGW_ANNEX_A:
                *chan_bw = (0x1 << 8);
                rc = TRUE;
                break;
            case RFGW_ANNEX_B:
                *chan_bw = (0x1 << 9);
                rc = TRUE;
                break;
            case RFGW_ANNEX_C:
                *chan_bw = (0x1 << 10);
                rc = TRUE;
                break;
            default:
                break;
        }
    } 
    return(FALSE);
}

boolean errp_get_param_qam_interleaver_cap (struct ermi_r_neighbor_ *nbr, 
                                            uint32 *interleaver)
{
    uint8 fec_i, fec_j;

    if (lcc_get_qam_fec_i_field(nbr->qam_slot, nbr->qam_port, 
                                nbr->qam_chan, &fec_i) &&
        lcc_get_qam_fec_j_field(nbr->qam_slot, nbr->qam_port, 
                                nbr->qam_chan, &fec_j) ) {
        if (fec_i == 8) {
            if (fec_j == 16) {
                *interleaver = (0x1 << 8);
                return(TRUE);
            }
        }
        if (fec_i == 12) {
            if (fec_j == 7) {
                *interleaver = (0x1 << 20);
                return(TRUE);
            }
        }
        if (fec_i == 16) {
            if (fec_j == 8) {
                *interleaver = (0x1 << 9);
                return(TRUE);
            }
        }
        if (fec_i == 32) {
            if (fec_j == 4) {
                *interleaver = (0x1 << 10);
                return(TRUE);
            }
        }
        if (fec_i == 64) {
            if (fec_j == 2) {
                *interleaver = (0x1 << 11);
                return(TRUE);
            }
        }
        if (fec_i == 128) {
            if (fec_j == 1) {
                *interleaver = (0x1 << 12);
                return(TRUE);
            }
            if (fec_j == 2) {
                *interleaver = (0x1 << 13);
                return(TRUE);
            }
            if (fec_j == 3) {
                *interleaver = (0x1 << 14);
                return(TRUE);
            }
            if (fec_j == 4) {
                *interleaver = (0x1 << 15);
                return(TRUE);
            }
            if (fec_j == 5) {
                *interleaver = (0x1 << 16);
                return(TRUE);
            }
            if (fec_j == 6) {
                *interleaver = (0x1 << 17);
                return(TRUE);
            }
            if (fec_j == 7) {
                *interleaver = (0x1 << 18);
                return(TRUE);
            }
            if (fec_j == 8) {
                *interleaver = (0x1 << 19);
                return(TRUE);
            }
        }
    } 
    return(FALSE);
}

boolean errp_get_param_qam_docsis_vid_cap (struct ermi_r_neighbor_ *nbr, 
                                           uint32 *docsis_vid_cap)
{
    uint32 qam_mode;
    boolean rc = FALSE;

    if (lcc_get_qam_mode_field(nbr->qam_slot, nbr->qam_port, 
                               nbr->qam_chan, &qam_mode)) {
        switch (qam_mode) {
            case QAM_MODE_VIDEO:
                *docsis_vid_cap = (0x1 << 16);
                rc = TRUE;
                break;
            case QAM_MODE_MPT:
                *docsis_vid_cap = (0x1 << 17);
                rc = TRUE;
                break;
            case QAM_MODE_PSP:
                *docsis_vid_cap = (0x1 << 18);
                rc = TRUE;
                break;
                // TBD HOT-HOT stream redundancy need to be indicated here, bit20
            default:
                break;
        }
    }
    return(rc);
}

boolean errp_get_param_qam_rf_port_id (struct ermi_r_neighbor_ *nbr, 
                                       uint32 *rf_port_id)
{
    *rf_port_id = LCC_LOGICAL_PORTID(nbr->qam_slot, nbr->qam_port);
    return(TRUE);
}

boolean errp_get_param_qam_total_bw (struct ermi_r_neighbor_ *nbr, 
                                     uint32 *total_bw)
{
    rfgw_qam_annex_t annex_type;
    uint32 format, srate;

    if (lcc_get_qam_annex_type_field(nbr->qam_slot, nbr->qam_port, 
                                     nbr->qam_chan, &annex_type) &&
        lcc_get_qam_format_field(nbr->qam_slot, nbr->qam_port, 
                                 nbr->qam_chan, &format) &&
        lcc_get_qam_srate_field(nbr->qam_slot, nbr->qam_port, 
                                nbr->qam_chan, &srate) ) {
        *total_bw = get_qam_bw(annex, format, srate)/1000; /* Kbps */
        return(TRUE);
    }
    return(FALSE);
}

boolean errp_get_param_qam_tsid (struct ermi_r_neighbor_ *nbr, 
                                 uint32 *tsid)
{
    if (lcc_get_qam_tsid_field(nbr->qam_slot, nbr->qam_port, 
                               nbr->qam_chan, tsid)) {
        return(TRUE);
    }
    return(FALSE);
}

boolean errp_get_param_qam_service_status (struct ermi_r_neighbor_ *nbr, 
                                           uint32 *service_status)
{
    boolean qam_enabled = FALSE;
    // rfgw_qam_owner_t qam_owner;

    if (!lcc_get_qam_is_enabled_field(nbr->qam_slot, nbr->qam_port, 
                                      nbr->qam_chan, 
                                      &qam_enabled)) {
        return(FALSE);
    }
    if (qam_enabled) {
        // TBD, need to check owner?
        // if (lcc_get_qam_owner_field(nbr->qam_slot, nbr->qam_port, 
        //                            nbr->qam_chan, &qam_owner)) {
        // }
        *service_status = ERRP_QAM_STATUS_OPERATIONAL;
        return(TRUE);
    } else {
        // TBD, graceful shutdown 
        // qam not taking new sessions, and shut after existing sessions end
        // *service_status = ERRP_QAM_STATUS_SHUTTING_DOWN;
        // TBD, redundant resource available on standby
        // *service_status = ERRP_QAM_STATUS_STANDBY;
    }
    return(FALSE);
}

boolean errp_get_param_max_mpeg_flows (struct ermi_r_neighbor_ *nbr, 
                                           uint32 *mpeg_flows)
{
    *mpeg_flows = MAX_VIDEO_FLOWS_PER_QAM;
    return(TRUE);
}

boolean errp_get_param_eig_group_name (struct ermi_r_neighbor_ *nbr, 
                                       char *group_name)
{
    // similar concept to a qam domain, 1-20
    // use qam domain number as edge input group name
    char *name;

    if (snprintf(group_name, ERRP_MAX_NAME_LEN, "qam%d/%d.%d", 
                 nbr->qam_slot, nbr->qam_port, nbr->qam_chan)) {
        return(TRUE);
    }
    returnFALSE);
}

/*
 * Assume caller provides storage for param_value
 */
boolean
errp_get_qam_param (uint param_type,
                    struct ermi_r_neighbor_ *nbr, 
                    void *param_value)
{
    boolean rc = FALSE;

    switch (param_type) {
        case ERRP_PARAM_QAM_GROUP_NAME:
            // get from CLI
            rc = errp_get_param_qam_group_name(nbr, (char *)param_value);
            break;
        case ERRP_PARAM_STREAMING_ZONE_NAME:
            // get from CLI
            rc = errp_get_param_sz_name(nbr, (char *)param_value);
            break;
        case ERRP_PARAM_SIGNALING_ADDR:
            // get from CLI, management IP addr, should be in
            // rfgw_video_server_group->rfgw_mgmt_conn_p->mgmt_conn_info->mgmt_ip_addr
            rc = errp_get_param_signaling_addr(nbr, (char *)param_value);
            break;
        case ERRP_PARAM_ALT_SIGNALING_ADDR:
            // may need CLI extension
            // TBD
            break;
        case ERRP_PARAM_QAM_NAME:
            // use qamx/y.z designation
            rc = errp_get_param_qam_name(nbr, (char *)param_value);
            break;
        case ERRP_PARAM_QAM_COST:
            // NA
            break;
        case ERRP_PARAM_QAM_CHAN_BW_CAP:
            // get from QAM API
            rc = errp_get_param_qam_chan_bw_cap(nbr, (ushort *)param_value);
            break;
        case ERRP_PARAM_QAM_J83_CAP:
            // get from QAM API
            rc = errp_get_param_qam_j83_cap(nbr, (ushort *)param_value);
            break;
        case ERRP_PARAM_QAM_INTERLEAVER_CAP:
            // get from QAM API
            rc = errp_get_param_qam_interleaver_cap(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_QAM_DOCSIS_VIDEO_CAP:
            // get from QAM API
            rc = errp_get_param_qam_docsis_vid_cap(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_QAM_MODULATION_CAP:
            // get from QAM API
            rc = errp_get_param_qam_mod_cap(nbr, (ushort *)param_value);
            break;
        case ERRP_PARAM_QAM_TOTAL_BW:
            // get from QAM API
            rc = errp_get_param_qam_total_bw(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_QAM_AVAIL_BW:
            // get from QAM API, optional
            break;
        case ERRP_PARAM_QAM_RF_PORT_ID:
            // get from QAM API
            rc = errp_get_param_qam_rf_port_id(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_QAM_SERVICE_STATUS:
            rc = errp_get_param_qam_service_status(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_QAM_FREQ:
            rc = errp_get_param_qam_freq(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_QAM_MOD_MODE:
            rc = errp_get_param_qam_mod_mode(nbr, (uchar *)param_value);
            break;
        case ERRP_PARAM_QAM_INTERLEAVER:
            rc = errp_get_param_qam_interleaver(nbr, (uchar *)param_value);
            break;
        case ERRP_PARAM_QAM_TSID:
            rc = errp_get_param_qam_tsid(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_QAM_ANNEX:
            rc = errp_get_param_qam_annex(nbr, (uchar *)param_value);
            break;
        case ERRP_PARAM_QAM_CHAN_WIDTH:
            rc = errp_get_param_qam_chan_width(nbr, (uchar *)param_value);
            break;
        case ERRP_PARAM_QAM_STATIC_UDP_MAP:
            // DDDDD
            break;
        case ERRP_PARAM_QAM_STATIC_RANGE_UDP_MAP:
            // DDDDD
            break;
        case ERRP_PARAM_QAM_DYNAMIC_RANGE_UDP_MAP:
            // DDDDD
            break;
        case ERRP_PARAM_QAM_FIBER_NODE_NAME:
            // DDDDD
            break;
        case ERRP_PARAM_QAM_INPUT_MAP:
            // DDDD
            break;
        case ERRP_PARAM_EDGE_INPUT_SUBNET_MASK:
            // DDDD
            break;
        case ERRP_PARAM_EDGE_INPUT_ADDR:
            // DDDD
            break;
        case ERRP_PARAM_EDGE_INPUT_BW:
            // DDDD
            break;
        case ERRP_PARAM_EDGE_INPUT_INTF_ID:
            // DDDD
            break;
        case ERRP_PARAM_EDGE_INPUT_GROUP_NAME:
            // DDDD
            rc = errp_get_param_eig_group_name(nbr, (uint32 *)param_value);
            break;
        case ERRP_PARAM_MAX_MPEG_FLOWS:
            rc = errp_get_param_max_mpeg_flows(nbr, (uint32 *)param_value);
            break;
        default:
            break;
    }
    return(rc);
}












