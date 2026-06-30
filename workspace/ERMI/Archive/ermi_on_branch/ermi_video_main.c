/*
 * --------------------------------------------------------------------------
 * ermi_video_main.c
 *     Main Code for EQAM - Control and Registration (EQAMR & ERMI_C)
 *     part of the  ERMI-Video Control Plane. More details in EDCS-337213.
 *
 *     EQAMR & ERMI_C communicates with one or more Edge Resource Manager (ERM)
 *     and local video linecards.
 *
 *     There is no main loop for the ERMI_C. All activity is driven by IOS
 *     events (e.g messages and timers).
 * 
 *     File originally ported from eda_main.c:
 *
 * Jan 2004, Ashok Bhaskar, Jun 2008: Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 * --------------------------------------------------------------------------
 */
#include <string.h>
#include COMP_INC(kernel/memory, chunk.h)
#include <assert.h>
#include COMP_INC(kernel, subsys.h)
#include "config.h"
#include COMP_INC(cisco, ciscolib.h)
#include COMP_INC(kernel, list.h)

#include COMP_INC(rfgw/sup, ../src/qam/qam.h)
#include COMP_INC(rfgw/sup, ../src/qam/qam_db.h)
#include COMP_INC(rfgw/sup, rfgw_registry.h)

#include INTERNAL_INC(sup/src/vscm/video_session_api.h)
#include INTERNAL_INC(sup/src/vclient/ermi/ermi_cli_util.h)

#include "ermi_c_global.h"
#include "msg_ermi_c.c"
#include "ermi_video_hash.h"
#include "ermi_c_master.h"
#include "ermi_video_debug.h"
#include "ermi_video_main.h"
#include "ermi_c_rtsp_api.h"


#define VREP_LATER


/*
 * Globals.
 */
ermi_config_t ermi_config;
eda_lc_info_t lc_info; /* line card information */
chunk_type *eda_sg_chunk;
chunk_type *eda_qam_chunk;
chunk_type *eda_port_map_pool;
chunk_type *eda_update_msg_chunk;
chunk_type *eda_udp_map_chunk;
rfgw_ermi_params_t  rfgw_ermi_params;

boolean rfgw_ermi_supported = FALSE;

/*
 * eda_sg_htbl is a ptr to a hash table of service group elements of type
 * eda_svcgrp_t.
 * Note - When a service grp is added to the hash table, it must be inserted 
 *        in the eda_sg_list list as well.  Similarly when it is removed.
 */
vcp_hash_table_t *eda_sg_htbl; 

/*
 * eda_sg_list is the list of all service groups known to the ERMI_C.  This is 
 * not a sorted list - elements are added to the list in the order they 
 * are created.  
 * Note - it shares elements with the eda_sg_htbl.  
 */
list_header eda_sg_list; 

/*
 * Get the following information about the video linecards in the chassis:
 *     . number of video linecards
 *     . for each video linecard, get the pointer to the linecard info 
 * Inputs:
 *     None
 * Returns:
 *     The number of video linecards found.  
 */
uint eda_get_video_card_count (void)
{
    return (VIDEO_MAX_LINECARD); 
}
/*
 * Allocate and initialize a service group table entry.
 * Inputs:
 *     sgid = string holding the service group id/name.
 * Returns:
 *     A ptr to a new service group entry structure with the name initialized.
 *     NULL is returned on error.
 */
svcgrp_t *eda_create_svcgrp (char *sgid)
{
    svcgrp_t *sg_entry;

    sg_entry = chunk_malloc(eda_sg_chunk);
    if (sg_entry != NULL) {
        /* successfully allocated */
        sstrncpy(sg_entry->sgid, sgid, MAX_SGID_LEN);

        if (list_create(&sg_entry->qam_id_list, 0, "eda sg qam list", 
                        LIST_FLAGS_AUTOMATIC)) {
        } else {
            errmsg(&msgsym(NORESOURCE, ERMI_C), 
                   "Failed to create qam id list.");
        } 
    } else {
        /* could not alloc chunk */
        errmsg(&msgsym(NORESOURCE, ERMI_C), 
               "Failed to alloc Svc Grp entry.");
    }
    return(sg_entry);
}
/*
 * Add a service group to the service group table.
 * Inputs:
 *     svcgrp = ptr to the hash element to be added.
 */
ermi_status eda_add_svcgrp (svcgrp_t *svcgrp)
{
    if (vcp_add_hash_obj(eda_sg_htbl, svcgrp)) {
        /* added to hash table, now add it to the list of svc grps */
        if (list_enqueue(&eda_sg_list, NULL, svcgrp)) {
            return(ERMI_OK); 
        } else {
            /* failed to add to list */
            errmsg(&msgsym(NORESOURCE, ERMI_C), "Failed to insert Svc Grp entry.");
        }
    }
    return(ERMI_FAIL); /* ??? */
}

/*
 * delete a service group from the service group table.
 * Inputs:
 *     svcgrp = ptr to the hash element to be deleted.
 */
ermi_status eda_del_svcgrp (svcgrp_t *svcgrp)
{
    if (vcp_del_hash_obj(eda_sg_htbl, svcgrp)) {
        /* added to hash table, now add it to the list of svc grps */
        if (list_remove(&eda_sg_list, 0, svcgrp)) {
            return(ERMI_OK); 
        } else {
            /* failed to delete to list */
             errmsg(&msgsym(OP_FAIL, ERMI_C), __FUNCTION__, "Delete",
               "Failed to delete service group", 0);
        }
    }
    return(ERMI_FAIL); /* ??? */
}

/*
 * Find a service group in the service group table.
 * Inputs:
 *     sgid = string holding the service group id
 * Returns:
 *     a ptr to the service group struct
 *     NULL if any error occurred.
 */
svcgrp_t *eda_find_svcgrp (char *sgid)
{
    svcgrp_t *sgptr;

    sgptr = vcp_find_hash_obj(eda_sg_htbl, sgid);
    if (sgptr != NULL) {
        /* found */
        return(sgptr);
    } else {
        /* failed to find service group - no error message needed. */
        return(NULL);
    }
}
/*
 * Add a qam to a service group.
 * Inputs:
 *    qam = the qam
 *    svcgrp = ptr to service group entry.
 * Returns:
 *    ERMI_OK if successful
 *    ERMI_FAIL otherwise.
 */
ermi_status eda_add_qam_to_svcgrp (rfgw_qam_t *qam, svcgrp_t *svcgrp)
{
    qam_id_entry_t *qam_id_entry;

    /*
     * Alloc an entry for the qam id list.
     */
    qam_id_entry = chunk_malloc(eda_qam_chunk);
    if (qam_id_entry == NULL) {
        /* could not alloc chunk */
        errmsg(&msgsym(NORESOURCE, ERMI_C), 
               "Failed to alloc qam id entry.");
        return(ERMI_FAIL);
    } else {
        /* successfully allocated */
         qam_id_entry->ts_id = qam->tsid;
        /*
         * Add the qam id to the qam list on this service group.
         */
        if (list_enqueue(&svcgrp->qam_id_list, NULL, qam_id_entry) == TRUE) {
            return(ERMI_OK);
        } else {
            /* failed to enqueue */
            errmsg(&msgsym(NORESOURCE, ERMI_C), 
                   "Failed to queue qam id list entry.");
            chunk_free(eda_qam_chunk, qam_id_entry);
            return(ERMI_FAIL);
        }
    }
}

/*
 * Assign a udp port number for this session.  This is obtained from
 * the free list associated with the linecard. If there are no udp port 
 * numbers available, an error is returned.
 * Inputs:
 *    qam = ptr to the qam struct
 *    udp_port_ptr = where the udp port number is to be returned.
 * Returns:
 *    A 16 bit udp port number at the specified address.
 *    A pointer to the port_map entry that was assigned if successful.
 *    NULL otherwise.
 */
udp_port_map *eda_assign_udp_port_num (rfgw_qam_t *qam, uint16 *udp_port_ptr)
{
    udp_port_map *port_map = NULL;

#ifdef CHANGE_ME // Code needs to change to get the UDP_port from new LCC infra.
    /*
     * Get an entry off the free list.
     */
    port_map = list_dequeue(&mgr->cp_port_map_free_list);
    if (port_map) {
        /* got a valid port map entry */
        *udp_port_ptr = port_map->udp_port;
        /*
         * At this point, the port map is not bound to a specific qam, so
         * do that by putting this entry on the active list for the qam.
         */
        list_enqueue(&qam->cp_port_map_list, NULL, port_map);
        /*
         * Also store it in the existing session map hash table.
         */
        if (!add_hash_obj(mgr->session_map, (vn_link_obj *)port_map)) {
/* TODO move it from the active to the free list */
            errmsg(&msgsym(NORESOURCE, ERMI_C), 
                   "Failed to add udp port to session map htbl");
            return(NULL);
        }
        /* set the flag so that CC knows that this port map is used by CP */
        port_map->flags |= PORT_CP;
    } else {
        /* invalid entry */
        errmsg(&msgsym(NORESOURCE, ERMI_C), 
               "Failed to get free udp port map entry.");
    }
#endif // CHANGE_ME

    return(port_map);
}

/*
 * Set up a udp port map given the udp port number and prog num.
 */
udp_port_map *eda_setup_udp_port (rfgw_qam_t *qam, uint16 udp_port_num,
				  uint16 prog_num)
{
    udp_port_map *port_map, *in_use_port_map;
    // qam_prog_t *qam_prog;

    port_map = in_use_port_map = NULL;

#ifdef CHANGE_ME // Code needs to change to get the UDP_port from new LCC infra.
    /*
     * First see if the udp port is still available.
     */
    port_map = cc_find_udp_port(&mgr->cp_port_map_free_list, 
				udp_port_num);
    if (port_map == NULL) {
	errmsg(&msgsym(BADPARAM, ERMI_C), __FUNCTION__, "udp_port_num", 
	       udp_port_num, "UDP port num has already been used.");
	return (NULL);
    } else {
	/*
	 * Get an entry off the free list.
	 */	
	port_map = cc_find_udp_port(&mgr->cp_port_map_free_list, 
			            udp_port_num);
    }
    
    if (!port_map) {
        /* invalid entry */
        errmsg(&msgsym(NORESOURCE, ERMI_C), 
	       "Cannot assign specific udp port");
	return (NULL);
    }

    /* make sure the prog_num is not in use yet */
    in_use_port_map = cc_find_qam_prog_num(qam, prog_num);
    if (in_use_port_map != NULL) {
        errmsg(&msgsym(NORESOURCE, ERMI_C),
               "Cannot allocate port map, the prog_num is already in use", 
                prog_num);
        return NULL;
    }    

    /* Set the program number */
    qam_prog = (qam_prog_t *)chunk_malloc(mgr->qam_prog_chunk);
    if (qam_prog == NULL) {
        errmsg(&msgsym(NORESOURCE, ERMI_C),
               "Failed to get memory for qam_prog", prog_num);
        return NULL;
    }

    list_remove(&mgr->cp_port_map_free_list, NULL, port_map);
    /* put this entry on the active list */
    list_enqueue(&qam->cp_port_map_list, NULL, port_map);	

    qam_prog->qam = qam;
    qam_prog->prog_num = prog_num;
    list_insert(&port_map->qam_prog_list, NULL, qam_prog, cc_qam_prog_insert);

    /*
     * Also store it in the existing session map hash table.
     */
    if (!add_hash_obj(mgr->session_map, (vn_link_obj *)port_map)) {
        /* TODO move it from the active to the free list */
	errmsg(&msgsym(NORESOURCE, ERMI_C), 
	       "Failed to add udp port to session map htbl");
	eda_free_udp_port_num(qam, udp_port_num);	
	return (NULL);	
    }

    /* set the flag so that CC knows that this port map is used by CP */
    port_map->flags |= PORT_CP;
#endif // CHANGE_ME

    return(port_map);
}

/*
 * Free the udp port number for this session.  This consists of moving
 * the entry from the active list to the free list associated with the qam.
 * Inputs:
 *    qam = ptr to the qam struct.
 *    udp_port_num = the udp port number that is to be freed.
 * Returns:
 *    A return value of ERMI_OK if successful
 *    ERMI_FAIL otherwise.
 */
ermi_status eda_free_udp_port_num (rfgw_qam_t *qam, uint16 udp_port_num)
{
#ifdef CHANGE_ME // Code needs to change to get the UDP_port from new LCC infra.
    udp_port_map *port_map;
    list_element *current;
    list_element *next;
    list_element *to_move;
    qam_prog_t *qam_prog;
    qam_prog_t *qam_prog_removed;

    /*
     * Find the specified udp port number on the active list and move it
     * to the free list.
     */
    FOR_ALL_ELEMENTS_IN_LIST_SAVE_NEXT(&qam->cp_port_map_list, current, next) {
        port_map = current->data;
        if (port_map->udp_port == udp_port_num) {
	    qam_prog = cc_find_qam_prog(port_map, qam);
	    qam_prog_removed = list_remove(&port_map->qam_prog_list, NULL, 
                                           qam_prog);
            if (qam_prog_removed) {
                /* return memory */
                chunk_free(mgr->qam_prog_chunk, qam_prog_removed);
            }

            /* 
             * Found the list element corresponding to the port map entry, 
             * so remove it from qam and hash table session_map
             */
            to_move = list_remove(&qam->cp_port_map_list, NULL, port_map);
            del_hash_obj(mgr->session_map, port_map->udp_port);    

            /* 
             * put this entry at the start of the free list. This is to allow
             * for a demo scenario, but should work fine otherwise as well.
             */
            /* clear the flag so that this port map is not used by CP */
            port_map ->flags &= ~PORT_CP;
            if (list_requeue(&mgr->cp_port_map_free_list, NULL, 
                             port_map) == TRUE) {
                return(ERMI_OK);
            } else {
                return(ERMI_FAIL); 
            }
        }
    }
    /* did not find the entry */
    errmsg(&msgsym(OP_FAIL, ERMI_C), __FUNCTION__, 
           "Find and/or free udp port num", udp_port_num);
#endif // CHANGE_ME

    return(ERMI_FAIL);
}


/*
 * Deprecated 11/30/04, D.C.
 *
 * Register QAM and other resources with the ERM.  This is done via the 
 * VREP ANNOUNCE message.  This fn sends a burst of announce messages (one
 * per resource) without waiting for the ERM to send responses to them.
 * Each announce message uses a separate session, which is closed when the
 * corresponding response message is received.
 * 
 * Inputs:
 *    None
 * Returns:
 *    None
 */
void eda_register_resources_with_ERM (void)
{
#ifdef VREP_LATER
    int qam_num;
    uint16 slot;
    rfgw_lc_mgr *mgr;
    rfgw_qam_t *qam;

    for (slot = MIN_MB_SLOT_ID; slot <= MAX_MB_SLOT_ID; slot++) {
        lcman_get_lcmgr(slot, &mgr);
        if (mgr) {
            /* valid video linecard */
            for (qam_num = 0 ; qam_num < NUMB_MB_QAM_PER_LC ; qam_num++) {
                /* for all qams */
                qam = mgr->qam[qam_num];
                /* 
                 * At init time, the announce messages are sent only for:
                 *     . QAMs that are control plane enabled.
                 *     . QAMs that are up and running.  
                 * This is because the ERM does not bother with resources that 
                 * it cannot allocate.
                 */
/* TODO - add check for bandwidth available as a criteria for reporting this qam */
                if (cc_is_qam_cp_enabled(qam) && (qam->enabled == TRUE)) {
                    /* qam is control plane enabled and up */
                    if (eda_send_announce_msg(qam) == ERMI_FAIL) {
                        /* 
                         * Failed to send announce msg for this qam.  Since
                         * we don't know why it failed, don't continue. 
                         */
                        ERMI_C_BUGINF("
                               \n%s: Failed to send announce msg for qam", 
                               __FUNCTION__);
                        return;
                    }
                } else {
                    /* qam not eligible to be reported to erm */
                }
            }
        }
    }
#endif // VREP_LATER
}

/*
 * Set the specified qam to Control Plane disabled mode (legacy mode).  
 * Inputs:
 *     qam = ptr to qam struct
 * Returns:
 *     None.  No error checking is performed.
 */
void eda_set_qam_cp_disabled (rfgw_qam_t *qam)
{
#ifdef ERMI_VIDEO_LATER
    if ( (qam->flags & QAM_FLAGS_SESSION_SIGNALING) == 
	 QAM_FLAGS_SESSION_SIGNALING ) {
	qam->state = LCC_QAM_STATE_DOWN;
	eda_send_qam_state_to_erm(qam);
	qam->flags &= ~QAM_FLAGS_SESSION_SIGNALING;
	qam->erm_state_last_reported = ERMI_C_QAM_STATUS_UNKNOWN;
	/*
	 * Force port map lists to the default non-control plane mode.
	 */
	if (cc_init_default_session_map(qam) == FALSE) {
	    /* unable to init default session map */
	    errmsg(&msgsym(OP_FAIL, ERMI_C), __FUNCTION__, "Init",
		   "Failed to init default session map", qam->tsid);
	}
    }
#endif // ERMI_VIDEO_LATER
}
extern ermi_status ermi_c_send_qam_down_announce (uint slot, uint port, uint);

void ermi_handle_qam_state_change (uint slot, uint port, uint qam_ch,
                                   boolean updown)
{
     // lcc_set_qam_state_field(slot, port, qam_ch, state);
     // .. More stuff. On SCM session cleanup to be plugged into SCM & LCC APIs
}

/*
 * Handle the "no shut" state change on a qam.
 * Inputs:
 *     qam  = ptr to the affected qam struct.
 * Returns:
 *     None.
 */
void ermi_handle_qam_state_up (uint card_type,
                               uint slot, uint port, uint qam_ch)
{
    rfgw_qam_t *qam;

    if ( !(rfgw_is_card_type_qam( card_type )) ) {
        return;
    }

    /* ignore the events not related to qam subinterfaces */
    if ( !(rfgw_is_physical_qam_port( port, card_type ))  || !qam_ch) {
        return;
    }

    ERMI_C_BUGINF("ERMI: Processed Qam%d/%d.%d (%d) - state UP.",
                  slot, LCC_LOGICAL_PORTID(slot, port), qam_ch,
                  LCC_PHYSICAL_QAMID(qam_ch));

    qam = lcc_qam_get_record(slot, port, LCC_PHYSICAL_QAMID(qam_ch));
    ermi_handle_qam_state_change(slot, port, qam_ch, qam->is_enabled /*TRUE*/);
}

/*
 * Handle the "shut" state change on a qam.
 * Inputs:
 *     qam  = ptr to the affected qam struct.
 * Returns:
 *     None.
 */
void ermi_handle_qam_state_down (uint card_type,
                                 uint slot, uint port, uint qam_ch)
{
    // rfgw_qam_t *qam = lcc_qam_get_record(slot, port, qam_ch);

    ermi_handle_qam_state_change(slot, port, qam_ch, FALSE);

    ermi_c_send_qam_down_announce(slot, port, qam_ch);
}

#ifdef ERMI_VIDEO_LATER
/*
 * Handle the "maintenance wait over" state change on a qam.
 * Inputs:
 *     qam  = ptr to the affected qam struct.
 * Returns:
 *     None.
 */
void ermi_handle_qam_event_maint_wait_over (rfgw_qam_t *qam)
{
    ermi_handle_qam_state_change(qam->slot, qam->port, qam->channel,
                                 LCC_QAM_STATE_MAINTENANCE_WAIT_OVER);
}

/*
 * Handle the "qam failed" state change on a qam.
 * Inputs:
 *     qam  = ptr to the affected qam struct.
 * Returns:
 *     None.
 */
void ermi_handle_qam_event_qam_fail (rfgw_qam_t *qam)
{
    ermi_handle_qam_state_change(qam->slot, qam->port, qam->channel,
                                 LCC_QAM_STATE_FAILED);
}
#endif // ERMI_VIDEO_LATER
/*
 * Handle the change in "control plane enabled/disabled" state of a QAM.
 * Inputs:
 *     qam  = ptr to the affected qam struct.
 * Returns:
 *     None.
 */
void ermi_handle_qam_cp_state_change (rfgw_qam_t *qam)
{
#ifdef VREP_LATER
    /* 
     * Check the last reported state of the QAM.  If there is a change,
     * it has to be reported to the ERM via an ANNOUNCE message.
     */
    if (qam->flags_last_reported != qam->flags)  {
        if (eda_send_announce_msg(qam) == ERMI_FAIL) {
            /* 
             * Failed to send announce msg for this qam.  
             */
            ERMI_C_BUGINF("\n%s: Failed to send announce msg for qam", 
                   __FUNCTION__);
        }
        qam->flags_last_reported = qam->flags;
    } else {
        ERMI_C_BUGINF("\nCP State change filtered...");
    }
#endif // VREP_LATER
}

/*
 * Invoked when there is a change in the video routing configuration (via
 * registry).
 * Inputs:
 *     slot = slot number of the linecard that is affected 
 *     data_ip = ip address on which this slot receives video
 *     udp_low = start of the udp port number range that can be used.
 *     udp_high = end of the udp port number range that can be used.
 * Returns:
 *     None
 */
void ermi_handle_video_route_change (uint8 slot, ipaddrtype data_ip,
                                    uint16 udp_low, uint16 udp_high)
{
#ifdef NOT_NEEDED // AS this is already handled by RFGW infra code.
    ....
    cc_init_cp_udp_port_numbers(slot, slot);
#endif // NOT_NEEDED
}

/*
 * Invoked when there are no sessions left on the qam. Usually there is
 * nothing to do, but if this qam is in the "maintenance shutdown mode",
 * then the qam has to change state.
 * Inputs:
 *     qam = ptr to the qam
 * Returns:
 *     ERMI_OK if successful 
 *     ERMI_FAIL otherwise.
 */
#ifdef ERMI_VIDEO_LATER
void eda_all_sessions_over_on_qam (rfgw_qam_t *qam)
{
    if (qam->state == LCC_QAM_STATE_MAINTENANCE) {
        ermi_handle_qam_event_maint_wait_over(qam);
    }
}
#endif // ERMI_VIDEO_LATER


/*
 * Initialization for the ERMI-Video Protocola. Invoked from the initialization
 * part of the control process or even from the CLI routines.  
 * Inputs:
 *     None.
 * Returns:
 *     ERMI_OK if successful.  Any other value implies a failure.
 */
ermi_status ermi_video_start (void)
{
    // list_header *list;
    // int slot;
 
    if (ermi_config.ermi_enabled) {
         return (ERMI_OK);
    }
    ermi_config.ermi_enabled = TRUE;

    ERMI_TRACE;

    /*
     * Set up the ERMI control block.
     */
    // ermi_cb.num_cards = VIDEO_MAX_LINECARD;

    /*
     * Hook into various registries as needed.
     */
    reg_add_rfgw_interface_up(ermi_handle_qam_state_up, 
                              "ERMI_C handler for QAM UP state change");
    reg_add_rfgw_interface_down(ermi_handle_qam_state_down, 
                                "ERMI_C handler for QAM DOWN state change");

#ifdef NOT_YET_SUPPORTED_ON_RFGW
    reg_add_video_qam_event(LCC_QAM_STATE_MAINTENANCE_WAIT_OVER, 
                            ermi_handle_qam_event_maint_wait_over,
                            "ERMI_C handler for maint shutdown over");
    reg_add_video_qam_event(LCC_QAM_STATE_FAILED,
                            ermi_handle_qam_event_qam_fail,
                            "ERMI_C handler for qam failure event");
#endif // NOT_YET_SUPPORTED_ON_RFGW

#ifdef ONLY_FOR_VREP
    /* 
     * init as an VREP Speaker
     */
     // eda_init_vrep(eda_handle_vrep_error);

    /* 
     * check if ERM's ip and my_ip are available, if not start the timer
     */
     // eda_retry_vrep_open(&ermi_cb.vrep_open_timer, 0);
#endif // ONLY_FOR_VREP

    /*
     * init as RTSP server, 0 to accept any IP
     */

    ERMI_C_BUGINF("\n%s(): rtsp_init_server(ermi_c_rtsp_msg_handler).",
                  __FUNCTION__);

    rtsp_init_server(0, DEFAULT_RTSP_PORT, (void *)ermi_c_rtsp_msg_handler);

    return(ERMI_OK);
}

/*
 * Shutdown the ERMI_C.  Invoked from the CLI routines.  
 * Inputs:
 *     None.
 * Returns:
 *     ERMI_OK if successful.  Any other value implies a failure.
 */
ermi_status ermi_video_stop (void)
{
#if 1

    int slot;
    rfgw_lc_mgr *mgr;
  
    /* all the chunk and tables are created in subsys init,
       we cannot destroy them. Since if we do and enable ERMI_C in the future,
       thess would be missing  
    */
    vcp_destroy_hash_table(eda_sg_htbl);

    list_destroy(&eda_sg_list);

    chunk_destroy(eda_sg_chunk);
    chunk_destroy(eda_qam_chunk);

    /*
     * Delete the free udp port map list for each linecard.
     */
    for (slot = MIN_MB_SLOT_ID; slot <= MAX_MB_SLOT_ID; slot++) {
        lcman_get_lcmgr(slot, &mgr);
        if (mgr) {
            /* valid video linecard */
            if (&mgr->cp_port_map_free_list) {
                list_destroy(&mgr->cp_port_map_free_list);
            }
        }
    }
#endif

    ermi_config.ermi_enabled = FALSE;

    // if (mgd_timer_running(&ermi_cb.vrep_open_timer)) {
    //    mgd_timer_stop(&ermi_cb.vrep_open_timer);
    // }

    // ermi_timer_remove(ERMI_C_VREP_OPEN_TIMER);
    // vrep_shutdown(ermi_cb.vrep_handle);

    /* to be added, stop the RTSP server */
    
    return(ERMI_OK);
}

static
boolean ermi_proto_cfg_nbr_create (rfgw_video_server_group *video_sg)
{
    ermi_r_neighbor_t *nbr;

    nbr = (ermi_r_neighbor_t *)malloc(sizeof(ermi_r_neighbor_t));
    if (!nbr) {
        printf("%%ERROR: ERRP neighbor for video-server %s NOT created. No Memory.",
               video_sg->sg_name);
        assert(nbr);
        return FALSE;
    }
    nbr->erm_param.server_group = video_sg;

    video_sg->proto_specific_info = (void *)malloc(sizeof(ermi_protocol_info_t));

    /* Setup the reverse lookup of server-group from ERRP neighbor info */
    ((ermi_protocol_info_t *)video_sg->proto_specific_info)->errp_mynode = nbr;
    return TRUE;

}

static
boolean ermi_proto_cfg_nbr_delete (rfgw_video_server_group *video_sg)
{
     ermi_protocol_info_t *ermi_info;

     ermi_info = video_sg->proto_specific_info;
     if (!ermi_info) {
         return TRUE; /* already deleted */
     }

     if (ermi_info->errp_mynode) {
         free(ermi_info->errp_mynode);
     }

     /* Free other memory if any */
     free(video_sg->proto_specific_info);
     video_sg->proto_specific_info = NULL;
     return TRUE;
}

boolean ermi_sg_create (rfgw_video_server_group *video_sg)
{
     /* Add other ERMI specific function calls here */
     return (ermi_proto_cfg_nbr_create(video_sg));
}

boolean ermi_sg_delete (rfgw_video_server_group *video_sg)
{
     /* Add other ERMI specific function calls here */
     return (ermi_proto_cfg_nbr_delete(video_sg));
}


boolean errp_init ( void )
{
    if (!list_create(&rfgw_ermi_params.errp_nbr_list, 0,
                     "ERRP neighbor List", 0x0)) {
        /* Print an error message */
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "RFGW SSP: %%ERROR: Unable to create ERRP neighbor list");
        assert(0);
        return FALSE;
    }

    return TRUE;
}

extern boolean rfgw_ermi_supported;

/*
 * ERMI_C (ERMI-C) Subsystem initialization.
 */
static void ermi_c_subsys_init(subsystype* subsys)
{

    if (!rfgw_ermi_supported) {
        return;
    }

#ifdef NOT_NEEDED_FOR_ERMI_C
    /*
     * Create the Service Group table.  This table will be populated
     * as qams are added to a service group.
     */
    eda_sg_htbl = vcp_create_hash_table(EDA_SIZE_SVC_GRP_HASH_TBL, 
                                        ERMI_C_HASH_TYPE_STRING,
                                        vcp_string_hash_func, 
                                        vcp_default_id_func,
                                        vcp_string_lookup_func);
    if (eda_sg_htbl == NULL) {
        /* failed to create */
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "Failed to create Svc Grp Hash table.");
        return;
    }

    /*
     * Create the list of service groups.
     */
    if (list_create(&eda_sg_list, 0, "eda sg list", 
                    LIST_FLAGS_AUTOMATIC) == NULL) {
        /* failed to create */
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "Failed to create Svc Grp List.");
        return;
    }
    /*
     * Create the chunk pool for the Service Group table's entries.
     */
    eda_sg_chunk = chunk_create(sizeof(svcgrp_t), EDA_MAX_SVC_GROUPS,
                                CHUNK_FLAGS_DYNAMIC, NULL, 0, 
                                "ERMI_C SvcGrp");
    if (eda_sg_chunk == NULL) {
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "Failed to create Svc Grp Chunk Pool");
        return;
    }

    /*
     * Create the chunk pool for the qam id list's entries.
     */
    eda_qam_chunk = chunk_create(sizeof(qam_id_entry_t), 
                                 EDA_MAX_QAMS_IN_BOX,
                                 CHUNK_FLAGS_DYNAMIC, NULL, 0, 
                                 "ERMI_C Qam Id List");
    if (eda_qam_chunk == NULL) {
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "Failed to create Qam Id List Chunk Pool");
        return;
    }
#endif // NOT_NEEDED_FOR_ERMI_C

    ermi_config.ermi_enabled = FALSE;

#ifdef VREP_LATER
    /* create eda_update_msg_chunk pool */
    eda_update_msg_chunk = chunk_create(sizeof(ermi_vrep_eda_update_msg_t), 
                                 EDA_MAX_SESSIONS,
                                 CHUNK_FLAGS_DYNAMIC, NULL, 0, 
                                 "ERMI_C VREP update msg");
    if (eda_update_msg_chunk == NULL) {
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "Failed to create ERMI_C VREP update msg chunk pool");
        return;
    }


    /* create VREP udp_map_chunk pool */
    eda_udp_map_chunk = chunk_create(VREP_UDP_MAP_SIZE, 
                                     5*NUM_QAMS_PER_BLADE,
                                     CHUNK_FLAGS_DYNAMIC, NULL, 0, 
                                     "ERMI_C udp map");
    if (eda_udp_map_chunk == NULL) {
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "Failed to create ERMI_C udp map chunk pool");
        return;
    }
#endif // VREP_LATER

    // VREP_LATER_TBD: These two init routines need to move to ERMI-R subsys init
    errp_init();
    /* Init ERMI QAM group */
    ermi_qam_grp_init();

    ERMI_TRACE;

}


#define ERMI_C_MAJVERSION 1
#define ERMI_C_MINVERSION 0
#define ERMI_C_EDITVERSION 1

SUBSYS_HEADER(ERMI_C, ERMI_C_MAJVERSION, ERMI_C_MINVERSION, ERMI_C_EDITVERSION,
              ermi_c_subsys_init, SUBSYS_CLASS_PROTOCOL, NULL);
 
