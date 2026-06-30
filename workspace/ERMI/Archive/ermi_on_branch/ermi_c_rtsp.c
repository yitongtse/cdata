/*
 * --------------------------------------------------------------------------
 * ermi_c_rtsp.c
 *     Key Functions to process pre-parsed ERMI-1 (ERMI-C) interface protocol
 *     messages, and send responses to Edge Resource Manager (ERM) using RTSP.
 *
 * Dec 2004, Dean Chen: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 * --------------------------------------------------------------------------
 */
#include <string.h>
#include COMP_INC(kernel/memory, chunk.h)
#include <assert.h>
#include "logger.h"
#include COMP_INC(ip, ip.h)
#include IOS_INC(h/interface_private.h)
#include "../ipmulticast/igmp.h"
#include "../ipmulticast/ipmulticast_registry.h"
#include COMP_INC(rfgw/sup, ../src/qam/qam.h)
#include COMP_INC(rfgw/sup, ../src/qam/qam_db.h)

#include COMP_INC(cisco, ciscolib.h)

#include IOS_INC(src-galaxy/lib/elb/TypesClean.h)
#include INTERNAL_INC(sup/include_private/qam_domain.h)

#include "msg_ermi_c_vcp.c"
#include "msg_ermi_c.c"
#include "ermi_c_global.h"
#include "ermi_c_rtsp_api.h"
#include "ermi_c_master.h"

#include "ermi_video_hash.h"
#include "ermi_video_debug.h"
#include "ermi_video_main.h"

#include "ermi_video_dummy.h"
#include "ermi_c_session_util.h"

#include INTERNAL_INC(sup/src/vclient/session_main.h)
#include INTERNAL_INC(sup/src/vclient/session_util.h)

/* Function Declarations */

/*
 * Translate the ermi return code value to an RTSP return code.
 * Inputs:
 *    return_code = RTSP return code.
 * Returns:
 *    The corresponding RTSP return code. If the input return_code is invalid,
 *    INVALID_PARAMETER is returned.
 */
rtsp_ret_code_e ermi_c_retcode_to_rtsp_retcode (ermi_return_code_t return_code)
{
    switch (return_code) {

    case ERMI_RET_OK:
        return(RTSP_RET_OK);

    case ERMI_RET_INVALID_PARAMETER:
        return(RTSP_RET_INVALID_PARAMETER);

    case ERMI_RET_NOT_ENOUGH_BANDWIDTH:
        return(RTSP_RET_NOT_ENOUGH_BANDWIDTH);

    case ERMI_RET_SESSION_NOT_FOUND:
        return(RTSP_RET_SESSION_NOT_FOUND);

    case ERMI_RET_INTERNAL_SERVER_ERROR:
        return(RTSP_RET_INTERNAL_SERVER_ERROR);

    case ERMI_RET_SERVICE_UNAVAILABLE:
        return(RTSP_RET_SERVICE_UNAVAILABLE);

    default:
        break;
    }

    errmsg(&msgsym(BADPARAM,ERMI_C),__FUNCTION__, "ReturnCode", return_code,"");
    return (RTSP_RET_INVALID_PARAMETER);
}


/*
 * Create the RTSP RESPONSE message in the rtsp_msg_t structure passed in.
 * Inputs:
 *    session = ptr to session control block
 *    msg = ptr to struct in which to create msg.
 *    session_id_op = indicates whether the RTSP protocol layer should add
 *                    the session id to the message or not.
 *    qam_op = indicates whether QAM transport header should be added
 *                    in the response message or not.
 * Returns:
 *    ERMI_OK if successful.
 *    ERMI_FAIL otherwise
 *
 * This function assumes that 1st Transport Header is used for QAM type params
 * and following headers are UDP type.
 */
static rtsp_msg_t *
ermi_c_create_rtsp_response_msg (rtsp_session_t *session,
                                 ermi_return_code_t return_code,
                                 rtsp_msg_t *rcvd_msg,
                                 ermi_session_id_op session_id_op,
                                 boolean qam_op)
{
    ushort i;
    rtsp_msg_t *resp_msg;

    if (!rcvd_msg) {
        return NULL;
    }

    resp_msg = rtsp_new_msg();
    if (!resp_msg) {
        errmsg(&msgsym(NORESOURCE, ERMI_C), "Failed to alloc RTSP Response.");
        return (NULL);
    }

    resp_msg->session_ptr = session;
    resp_msg->msg_type = RTSP_RESPONSE;
    resp_msg->resp_retcode = ermi_c_retcode_to_rtsp_retcode(return_code);
    RPM_SETBIT(resp_msg->valid, RP_BIT_RESP_RETCODE);

    resp_msg->num_trans_hdrs = 0;

    RPM_SETBIT(resp_msg->valid, RP_BIT_CLIENTSESSIONID);

    if (resp_msg->resp_retcode != RTSP_RET_OK) {
        ERMI_TRACE;
        return resp_msg;
    }

    /*
     * For SETUP Multicast Stream Response, we need to include the
     * original rcvd Transport Header into the Response Message.
     */
    if ((rcvd_msg->msg_type == RTSP_SETUP) &&
        (rcvd_msg->transhdr[1].unicast == FALSE)) {

        resp_msg->transhdr[1].unicast = FALSE;

        if (qam_op) {
            /* Transport Header MPEG_QAM */
            memcpy(&resp_msg->transhdr[0], &rcvd_msg->transhdr[0],
                   sizeof(transhdr_t));
            resp_msg->num_trans_hdrs = 1;
        }

        /*
         * Include UDP Transport Header for every rcvd transport header,
         * for which {session create + stream join} succeeded.
         */
        for (i = resp_msg->num_trans_hdrs; i < rcvd_msg->num_trans_hdrs; i++) {
            if ( (rcvd_msg->transhdr[i].type == MPEG_UDP) &&
                  RPM_TESTBIT(rcvd_msg->transhdr[i].tvalid,
                              RP_BIT_TRANS_STREAM_JOINED) ) {

                /* Transport Header MPEG_UDP */
                memcpy(&resp_msg->transhdr[i], &rcvd_msg->transhdr[i],
                       sizeof(transhdr_t));
                resp_msg->num_trans_hdrs++;
            }
        }
    }

    if (session_id_op == ERMI_SESS_ID_INCLUDE) {
        /* need to include the session id */
        RPM_SETBIT(resp_msg->valid, RP_BIT_SESSIONID);
    }

    ERMI_TRACE;

    return resp_msg;
}

/*
 * Send a RTSP SETUP RESPONSE message to the ERM.
 * Inputs:
 *     session = ptr to the session
 *     setup_result = a value from ermi_return_code_t indicating the result
 *                    of the setup command.
 *     qam = ptr to the qam associated with this session.
 * Returns:
 *     ERMI_OK if successful
 *     ERMI_FAIL otherwise.
 */
static ermi_status
ermi_c_send_rtsp_setup_response (rtsp_session_t *session, 
                                 rtsp_msg_t *rcvd_msg,
                                 ermi_return_code_t setup_result)
{
    rtsp_msg_t *resp_msg;

    assert(session != NULL);

    /* construct new RTSP response */        
    resp_msg = ermi_c_create_rtsp_response_msg(session, setup_result,
                                               rcvd_msg,
                                               ERMI_SESS_ID_INCLUDE, TRUE);
    if (resp_msg) {
        resp_msg->session_ptr = session;
        if (rtsp_send_response(session, resp_msg) == ERMI_OK) {
            /* msg sent */
            return (ERMI_OK);
        } else {
            /* failed to send msg */
            ERMI_C_RTSP_API_BUGINF("\n%s: Failed to send msg",__FUNCTION__);
        }            
    } else {
        /* failed to create msg */
        ERMI_C_RTSP_API_BUGINF("\nIn%s, Failed to create msg",__FUNCTION__);
    }

    return (ERMI_FAIL);
}

/*
 * Create and Send out to ERM an RTSP ANNOUNCE Request message, to notify of
 * any notification related to: Sessions, Streams and/or QAMs
 *
 *  INPUTS:
 *  OUTPUTS:
 *  RETURN: ERMI_FAIL / ERMI_OK
 */
ermi_status
ermi_c_create_send_announce (lc_event_type_t lc_event_type,
                             scm_session_id scm_sess_id,
                             uint32 last_state,
                             uint32 new_state,
                             uint32 error_code)
{
    rtsp_msg_t     *rtsp_msg;
    rtsp_session_t *session;
    rtsp_announce_code_t announce_code = RTSP_NOTICE_CODE_NONE;
    ermi_c_peer_t *peer_erm;

    peer_erm = ermi_c_get_peer_erm_info(NULL);

    session = rtsp_find_session(scm_sess_id);
    if (!session) {
        /* bad session ptr */
        errmsg(&msgsym(DATAERR, RTSP), "Bad session id", scm_sess_id);
        return (ERMI_FAIL);
    }

    rtsp_msg = (rtsp_msg_t *)rtsp_new_msg();
    if (!rtsp_msg) {
        /* failed to alloc msg memory */
        errmsg(&msgsym(NORESOURCE, ERMI_C),
               "Failed to alloc RTSP Announce Request msg.");
        return (ERMI_FAIL);
    }

    rtsp_msg->msg_type     = RTSP_ANNOUNCE;
    rtsp_msg->session_ptr  = session;
    rtsp_msg->session_ptr->peer_erm = peer_erm;

    sstrncpy(rtsp_msg->url, session->url, MAX_URL_LEN);
    RPM_SETBIT(rtsp_msg->valid, RP_BIT_URL);

    rtsp_msg->ftag = FTAG_ERMI;
    RPM_SETBIT(rtsp_msg->valid, RP_BIT_FTAG);

    /*
     * Mark SessionID Bits.
     * Copy of sessionIds not needed, as done in rtsp_send_response()
     */
    RPM_SETBIT(rtsp_msg->valid, RP_BIT_SESSIONID);
    RPM_SETBIT(rtsp_msg->valid, RP_BIT_CLIENTSESSIONID);

    switch (lc_event_type) {

       case ERMI_C_INPUT_PORT_EVENT:
            announce_code = RTSP_NOTICE_CODE_INPUT_PORT_FAILURE;
            break;

       case ERMI_C_QAM_EVENT:
            break;

       case ERMI_C_LC_SESSION_EVENT:
            announce_code = RTSP_NOTICE_CODE_DELIVERY_SUCCEEDED;

            /*
             * TBD: Sample Code - to be filled in once LC side support is
             * available, with state transition being reported for every 
             * session state change (INIT->IDLE->ACTIVE, etc.) on Video LC.
             * More details @ .../vscm/video_msg.c: scm_ipc_handler() and
             * ipc_msg handling for MSG_TYPE_VIDEO_UPDATE_IN_SESSION under
             * ssm_enqueue_idle_msg() -> scm_handle_linecard_in_sess_update()
             */
            if ((last_state == 1) && (new_state == 0)) {
                announce_code = RTSP_NOTICE_CODE_DOWNSTREAM_FAILURE;
            } else if ((last_state == 1) && (new_state == 2)) {
                announce_code = RTSP_NOTICE_CODE_SESSION_IN_PROGRESS;
            } else if ((last_state == 1) && (new_state == 2)) {
                announce_code = RTSP_NOTICE_CODE_UNABLE_TO_JOIN;
                announce_code = RTSP_NOTICE_CODE_SERVER_RESOURCE_UNAVAILABLE;
                announce_code = RTSP_NOTICE_CODE_INTERNAL_SERVER_ERROR;
                announce_code = RTSP_NOTICE_CODE_FAILOVER_TO_RED_SOURCE;
                announce_code = RTSP_NOTICE_CODE_STREAM_MARKER_MISMATCH;
    
                announce_code = RTSP_NOTICE_CODE_ERROR_CONTENT_PID_CONFLICT;
                announce_code = RTSP_NOTICE_CODE_BANDWIDTH_EXCEEDED_LIMIT;
            }
            break;

       default:
            break;
    }

    rtsp_msg->notice.code = announce_code;
    RPM_SETBIT(rtsp_msg->valid, RP_BIT_NOTICE);

    ERMI_TRACE;

    /* if (rtsp_send_response(rtsp_msg->session_ptr, rtsp_msg) != ERMI_OK) */
    if (ermi_c_rtsp_resp_enque(rtsp_msg) != ERMI_OK) {
        return (ERMI_FAIL);
    }

    ERMI_C_RTSP_API_BUGINF("\nSent ANNOUNCE Session: %d, Notice Code: %d (%s).",
                           scm_sess_id, rtsp_msg->notice.code,
                          ermi_c_rtsp_announcecode2text(rtsp_msg->notice.code)); 
    return (ERMI_OK);
}


ermi_status
ermi_c_send_qam_down_announce (uint slot, uint port, uint qam_ch)
{

    list_element *curr_elem;
    void         *curr_data;
    video_session_hdr     *out_hdr = NULL;
    video_session_out_ext *out_ext = NULL;
    uint32        scm_sess_id;
    uint16        qam_id;
    rfgw_qam_t   *qam;

    /* ignore the events not related to qam cards */
    if ( !(rfgw_is_card_qam( slot )) ) {
        ERMI_TRACE;
        return (ERMI_FAIL);
    }

    qam = lcc_qam_get_record(slot, port, LCC_PHYSICAL_QAMID(qam_ch));
    if ( !qam ) {
        ERMI_TRACE;
        return (ERMI_FAIL);
    }

    port = LCC_LOGICAL_PORTID(slot, port);
    qam_id = SPQ_TO_OFFSET(slot, port, qam_ch);

    ERMI_C_RTSP_API_BUGINF("\n%s: ANNOUNCE for: Qam %d/%d.%d (id: %d, tsid %d)",
                           __FUNCTION__, slot, port, qam_ch, qam_id, qam->tsid);
    
    /* if qam empty of streams, just return */
    if (SESSION_LIST_EMPTY_ON_QAM(qam_id)) {
        ERMI_TRACE;
        return ERMI_OK;
    }

    FOR_ALL_SESSIONS_ON_QAM(qam_id, curr_elem, curr_data, out_hdr, out_ext) {
        scm_sess_id = out_hdr->int_sess_id;
        ermi_c_create_send_announce(ERMI_C_LC_SESSION_EVENT,
                                    scm_sess_id,
                                    1, 0, 0);
    }

    ERMI_C_RTSP_API_BUGINF("\nSent ANNOUNCE for QAM (tsid= %d)", qam->tsid);

    return (ERMI_OK);
}

/*
 * Send a TEARDOWN RESPONSE message to the ERM.
 * Inputs:
 *     session = ptr to the session
 *     setup_result = a value from ermi_return_code_t indicating the result
 *                    of the teardown command.
 *     qam = ptr to the qam associated with this session.
 * Returns:
 *     ERMI_OK if successful
 *     ERMI_FAIL otherwise.
 */
static ermi_status
ermi_c_send_rtsp_teardown_response (rtsp_session_t *session, 
                                    rtsp_msg_t *rcvd_msg,
                                    ermi_return_code_t teardown_result)
{
    rtsp_msg_t *resp_msg;

    assert(session != NULL);

    /* valid session - create the message to be sent */
    resp_msg = ermi_c_create_rtsp_response_msg(session, teardown_result,
                                               rcvd_msg,
                                               ERMI_SESS_ID_INCLUDE, FALSE);
    if (resp_msg) {
        resp_msg->session_ptr = session;
        if (rtsp_send_response(session, resp_msg) == ERMI_OK) {
            /* msg sent */
            return (ERMI_OK);
        } else {
            /* failed to send msg */
            ERMI_C_RTSP_API_BUGINF("\n%s: Failed to send msg",__FUNCTION__);
        }
    } else {
        /* failed to create msg */
        ERMI_C_RTSP_API_BUGINF("\n%s: Failed to create msg", __FUNCTION__);
    }

    return (ERMI_FAIL);
}


/*
 * Send a TEARDOWN RESPONSE message to the ERM.
 * Inputs:
 *     session = ptr to the session
 *     setup_result = a value from ermi_return_code_t indicating the result
 *                    of the teardown command.
 *     qam = ptr to the qam associated with this session.
 * Returns:
 *     ERMI_OK if successful
 *     ERMI_FAIL otherwise.
 */
static ermi_status
ermi_c_send_rtsp_setparam_response (rtsp_session_t *session, 
                                    rtsp_msg_t *rcvd_msg,
                                    ermi_return_code_t setparam_result)
{
    rtsp_msg_t *resp_msg;

    assert(session != NULL);

    /* valid session - create the message to be sent */
    resp_msg = ermi_c_create_rtsp_response_msg(session, setparam_result,
                                              rcvd_msg,
                                              ERMI_SESS_ID_INCLUDE, FALSE);
    if (resp_msg) {
        resp_msg->session_ptr = session;
        if (rtsp_send_response(session, resp_msg) == ERMI_OK) {
            /* msg sent */
            return (ERMI_OK);
        } else {
            /* failed to send msg */
            ERMI_C_RTSP_API_BUGINF("\n%s: Failed to send msg",__FUNCTION__);
        }
    } else {
        /* failed to create msg */
        ERMI_C_RTSP_API_BUGINF("\n%s: Failed to create msg", __FUNCTION__);
    }

    return (ERMI_FAIL);
}

/*
 * Process the RTSP SETUP message.  Also handle sending back the Response
 * message in most cases.
 * Inputs:
 *     session = ptr to the associated session control block
 *     msg = ptr to the setup message structure
 * Returns:
 *     ERMI_OK if successful 
 *     ERMI_FAIL otherwise.
 */
ermi_status
ermi_c_process_rtsp_setup_msg (rtsp_session_t *session, rtsp_msg_t *msg)
{
    uint16 slot, port, i;
    ulong  sess_result;
    rfgw_qam_t *qam = NULL;
    uint32 new_sess_id;
    rfgw_session_setup_param_t sess_setup_req;
    ermi_return_code_t setup_status = ERMI_RET_INTERNAL_SERVER_ERROR;

    if (msg->num_trans_hdrs > 0) {
        if ( !RPM_TESTBIT(msg->transhdr[0].tvalid, RP_BIT_TRANS_QAM_TSID) ||
             !RPM_TESTBIT(msg->transhdr[1].tvalid, RP_BIT_TRANS_BIT_RATE) ||
             (!msg->transhdr[0].tsid) ) {
            setup_status = ERMI_RET_INVALID_PARAMETER;
            goto send_setup_response;
        }
    } else {
        setup_status = ERMI_RET_INVALID_PARAMETER;
        goto send_setup_response;
    }

    /* Using the TSID based hash search - QAM lookup routine */
    qam = tsid_qam_get_record( msg->transhdr[0].tsid );
    if (qam == NULL) {
        /* couldn't derive qam ptr */
        errmsg(&msgsym(BADPARAM, ERMI_C), __FUNCTION__, "tsid", 
               msg->transhdr[0].tsid, "Unable to get qam ptr from tsid.");
                   
        setup_status = ERMI_RET_INVALID_PARAMETER;
        goto send_setup_response;
    }
    slot = qam->slot;
    port = qam->port;

    session->qam_tsid = msg->transhdr[0].tsid;

    /* NK_TBD:
     * Add check to validate if a QAM with this tsid is configured to be
     * owned under the ERMI Server-group server (ERM) using configured ERM
     * IP Address and checking it against the IP Address of RTSP session.
     */

    /* NK_TBD:
     * Also check if a session with similar params on this very QAM, already
     * exists; "DUPLICATE_QAM" scenario
     */

    /* Fill Up the struct sess_create_req, with parsed RTSP params */

    memset(&sess_setup_req, 0, sizeof(rfgw_session_setup_param_t));

    sess_setup_req.output_port  = SPQ_TO_OFFSET(slot,
                                               LCC_LOGICAL_PORTID(slot, port),
                                               LCC_LOGICAL_QAMID(qam->channel));
    sess_setup_req.session_rate = msg->transhdr[1].bit_rate;
    sess_setup_req.is_multicast = ((msg->transhdr[1].unicast) ? FALSE : TRUE);
    sess_setup_req.dst_IP_addr  = msg->transhdr[1].dest;
    sess_setup_req.dst_UDP_port = msg->transhdr[1].dest_port;
    sess_setup_req.no_pid_remap = (msg->pid_remap ? FALSE : TRUE);

    // TBD: Changes might be required when ProgramRemap is enabled in SETUP
    sess_setup_req.outgoing_pgm_number = msg->transhdr[1].mpts_program;
    sess_setup_req.incoming_pgm_number = msg->transhdr[1].qam_prognum;

    /* Multiple Source Addresses for SSM Mcast; 1 Src Addr for Unicast */
    sess_setup_req.src_IP_addr_count = (msg->num_trans_hdrs - 1);

    /* assume atleast two transport header, one QAM and one UDP */
    if (msg->transhdr[1].unicast) {
        /* 
         * SETUP Request - for a Unicast service.
         */
        ERMI_C_RTSP_API_BUGINF("\n%s: Unicast setup request.", __FUNCTION__);

    } else {

        if (!IPMULTICAST(msg->transhdr[1].mcast_address)) {
            ERMI_C_RTSP_API_BUGINF(
                 "\nERROR: Mcast SETUP with Non-Mcast destination address %i",
                  msg->transhdr[1].mcast_address);
            setup_status = ERMI_RET_INVALID_PARAMETER;
            goto send_setup_response;
        }

        /* SETUP Request - for a Multicast service */
        /**********
        if ( !RPM_TESTBIT(msg->transhdr[1].tvalid, RP_BIT_TRANS_SRC) ) {
            // setup_status = ERMI_RET_INVALID_PARAMETER;
            // goto send_setup_response;
        }
        **********/

        /* Start @ 1, Skip 1st Header as it's MPEG_QAM Header */

        ERMI_C_RTSP_API_BUGINF("\n%s: %sSM Multicast setup request.",
                               __FUNCTION__, (msg->transhdr[1].src ? "S":"A"));

        for (i = 1; i < sess_setup_req.src_IP_addr_count; i++) {
            sess_setup_req.src_IP_addr[i-1] = msg->transhdr[i].src;
            sess_setup_req.dst_IP_addr      = msg->transhdr[i].mcast_address;
        }
    }

    ERMI_C_RTSP_API_BUGINF("\nERMI: New Session Req On QAM "
                           "[ Qam %d/%d.%d QamId: %d Tsid: %d ]",
                           slot, LCC_LOGICAL_PORTID(slot, port),
                           LCC_LOGICAL_QAMID(qam->channel),
                           sess_setup_req.output_port, session->qam_tsid);

    ERMI_TRACE;
    assert (session->session_id == VIDEO_SCM_NO_SESSION_ID);

    /*
     * Create the video session (In, Out, Map) using SCM API wrappers
     *
     * Common Code for creating - Unicast and MultiCast Sessions
     */
    new_sess_id = VIDEO_SCM_NO_SESSION_ID;
    sess_result = rfgw_ssp_create_video_session(RFGW_VIDEO_PROTOCOL_ERMI,
                                                &sess_setup_req, NULL,
                                                session,
                                                0, &new_sess_id);

    if ( (sess_result != VIDEO_RESULT_OK) ||
         (new_sess_id == VIDEO_SCM_NO_SESSION_ID) ) {

        /* Error: VSCM could not allocate a valid Session Id */
        setup_status = ERMI_RET_INTERNAL_SERVER_ERROR;

    } else {

        /* Hurray! Video session creation succeeded. Send Session OK Response */
        setup_status = ERMI_RET_OK;

        session->session_id = new_sess_id;
        msg->sessionID = new_sess_id;
    
        ERMI_C_RTSP_API_BUGINF("\nERMI: Added new RTSP session id: %d [%08X] to"
                    " ses_tbl[%d]",
                    new_sess_id, session->session_id, session->index);

        /*
         * IGMP/PIM join processing is already done by VSCM API code.
         * Based on session-creation and mcast-join success of the session,
         * Mark the successful sessions.
         */
        RPM_SETBIT(msg->transhdr[1].tvalid, RP_BIT_TRANS_STREAM_JOINED);

        /* TBD: Later (with SSM Hot-Hot support), mark for each session
         * created by VSCM.
         * for (i = 0; i < rcvd_msg->num_trans_hdrs; i++)
         * RPM_SETBIT(rcvd_msg->transhdr[map_sess->active].tvalid,
                      RP_BIT_TRANS_STREAM_JOINED)
         */
    }

send_setup_response:

    /*
     * Update the ERMI session control block. (Message Stats, etc.)
     */

    /* 
     * Send a setup response msg back to the ERM. setup_status
     * holds the return code.
     */
    if (ermi_c_send_rtsp_setup_response(session, msg, 
                                        setup_status) == ERMI_OK) {
        /* setup response sent */
        if (setup_status == ERMI_RET_OK) {
            return (ERMI_OK);
        } else {
            /* Setup Resp sent (with Error Code - during session creation) */
            ERMI_C_RTSP_API_BUGINF("\n%s: SETUP Response Sent with Error Code",
                                    __FUNCTION__);
            /* free session context */
            rtsp_server_close_session(session);
            return (ERMI_FAIL);
        }
    } else {
        /* some failure in sending setup response */
        ERMI_C_RTSP_API_BUGINF("\n%s: Failure in sending Setup Response", 
                               __FUNCTION__);
        return (ERMI_FAIL);
    }
}


/*
 * ermi_c_rtsp_delete_session:
 *
 * Function to delete an RTSP session created using VSCM API.
 *
 * Parameters:
 * rtsp_session_t * pointer, session_id, rtsp_message_sequence_num
 */
video_operation_result
ermi_c_rtsp_delete_session (rtsp_session_t *session, uint32 out_id,
                            uint32 msg_seq_id)
{
    video_session_request *scm_req;
    char sessid_str[MAX_SESSID_LEN] = "";
    session_proto_ctx *req_ctx = NULL;

    if (out_id == VIDEO_SCM_NO_SESSION_ID) {
        ERMI_C_RTSP_API_BUGINF("\nERMI: %%ERROR: DELETE Tx [%08X]: Sess ID [%s]"
                               " not found.", msg_seq_id,
                               sessid_str);
        return (VIDEO_RESULT_UNKNOWN_SESSION);
    }

    snprintf(sessid_str, MAX_SESSID_LEN, "%d", out_id);

    /* Print the request parameters */
    ERMI_C_RTSP_API_BUGINF("\nERMI: Received ERMI-2 Delete Session:"
                           "\nSession ID:  %s (%d)",
                           sessid_str, out_id);

    /* Create the request for the session we want to delete */
    scm_req = scm_video_get_request(VIDEO_REQUEST_DELETE_SESSION,
                                    VIDEO_REQUEST_BLOCKED,
                                    VSESSION_TYPE_MAP,
                                    VIDEO_CLIENT_REMOTE,
                                    NULL, req_ctx, NULL,
                                    out_id);
    if (!scm_req) {
        ERMI_C_RTSP_API_BUGINF("\nERMI: %%ERROR: DELETE Tx [%08X]: Unable to "
                  "allocate a request", msg_seq_id);
        return (VIDEO_RESULT_NO_MEMORY);
    }

//NK_TBD: Check if these are needed before calling rfgw_ssp_delete_video_session

    ERMI_TRACE;

    return rfgw_ssp_delete_video_session(RFGW_VIDEO_PROTOCOL_ERMI,
                                         msg_seq_id, out_id, NULL, scm_req,
                                         sessid_str);
}


/*
 * Process the RTSP TEARDOWN message.
 * Inputs:
 *     session = ptr to the associated session control block
 *     msg = ptr to the setup message structure
 * Returns:
 *     ERMI_OK if successful 
 *     ERMI_FAIL otherwise.
 */
ermi_status
ermi_c_process_rtsp_teardown_msg (rtsp_session_t *session, 
                                  rtsp_msg_t *msg)
{
    rfgw_qam_t *qam = NULL;
    ulong       delete_result;
    ermi_return_code_t tear_status = ERMI_RET_INTERNAL_SERVER_ERROR;

    ERMI_C_RTSP_API_BUGINF("\n%s: Got teardown msg", __FUNCTION__);

    if (session && msg) {

        /*
         * Teardown associated with a multi-cast/uni-cast service.
         * Internally invokes VSCM session delete API.
         */
        delete_result = ermi_c_rtsp_delete_session(session,
                                                   session->session_id,
                                                   msg->cseq);

        if (delete_result == VIDEO_RESULT_OK) {

            /* Hurray! Video session deletetion succeeded. Send OK Response */
            tear_status = ERMI_RET_OK;

            /* IGMP/PIM Leave processing is also done by VSCM */

        } else {
            ERMI_C_RTSP_API_BUGINF("\n%s: Error(%d) deleting session %08X",
                             __FUNCTION__, delete_result, session->session_id);
            return (ERMI_FAIL);
        }

    } else {
        /* Bad msg or Bad ptrs */
        errmsg(&msgsym(BADPARAM, ERMI_C), __FUNCTION__, 
               "Session or app_session", session, "in teardown request");

        /* not sending a response for this */
        return (ERMI_FAIL);
    }

    /* send_teardown_response */
    /* 
     * Send a teardown response msg back to the ERM. tear_status
     * holds the return code.
     */
    if (ermi_c_send_rtsp_teardown_response(session, msg, tear_status)
                                           == ERMI_OK) {

        rtsp_server_close_session(session);

        /* response sent */
        if (tear_status == ERMI_RET_OK) {
            if (qam->total_sessions == 0) {
#ifdef ERMI_R_VIDEO_LATER
                eda_all_sessions_over_on_qam(qam);
#endif // ERMI_R_VIDEO_LATER
            }
            return (ERMI_OK);
        } else {
            /* some failure */
            return (ERMI_FAIL);
        }
    } else {
        /* some failure in sending response */
        ERMI_C_RTSP_API_BUGINF("\n%s: Failure in sending teardown response msg",
                      __FUNCTION__);
        return (ERMI_FAIL);
    }
}


void
ermi_c_cleanup_rsp_context(rtsp_msg_t *resp_msg)
{
    rtsp_free_msg(resp_msg);

    // free(rtsp_resp_msg->session_ptr);

    // Remove from req_table
}


/*
 * ermi_c_rtsp_msg_handler:
 *
 * Main Entry Point for Handling RTSP Messages for ERMI Video Interface to ERM.
 */
void ermi_c_rtsp_msg_handler (rtsp_session_t *session, rtsp_msg_t *msg)
{
    rp_msg_type_e type;
    // rp_msg_type_e pending_msg;

    type = ((rtsp_msg_t*)msg)->msg_type;
    ERMI_C_RTSP_API_BUGINF("\nERMI: Processing RTSP msg, type %s (%#08x)\n",
                           ermi_c_rtsp_msg_type_str(type), type);

    switch (type) {

        case RTSP_SETUP:
            ermi_c_process_rtsp_setup_msg(session, (rtsp_msg_t*)msg);
            break;

        case RTSP_TEARDOWN:
            ermi_c_process_rtsp_teardown_msg(session, (rtsp_msg_t*)msg);
            break;

        case RTSP_SETPARAMETER:
            ermi_c_send_rtsp_setparam_response(session, (rtsp_msg_t*)msg,
                                               ERMI_RET_OK);
            break;

        case RTSP_GETPARAMETER:
            /* Already handled in outside caller function */
            break;

        case RTSP_RESPONSE:
            /* Response already handled by ermi_c_rtsp_svr_process_msg(). */
            break;

        default:
            break;
    }

}

