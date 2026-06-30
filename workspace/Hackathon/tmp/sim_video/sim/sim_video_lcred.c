///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "video.h"
#include "video_util.h"
#include "video_cli.h"
#include "sim_video.h"


const char* lcred_mode_label[] = {
    "NON-REDUNDANT",
    "PRIMARY",
    "SECONDARY",
    "UNDEFINED"
};

const char* lcred_role_label[] = {
    "ACTIVE",
    "STANDBY",
    "UNDEFINED"
};


lcred_mode_t lcred_mode = LCRED_MAX_MODE;
lcred_role_t lcred_role = LCRED_MAX_ROLE;
int lcred_primary_id = INVALID_ID;


boolean lcred_is_primary_id (int primary_slot_id);


// Check if a primary_slot_id matches lcred_primary_id
//
boolean lcred_is_primary_id (int primary_slot_id)
{
    return (primary_slot_id == lcred_primary_id);
}


// Check if a primary_id is valid
//   - id must be a valid MB slot
//   - for active role, id must match lcred_primary_id
//
boolean lcred_is_valid_primary (int id)
{
    // check to see if it valid slot
    if ( id < MIN_NG_SLOT_ID || id > MAX_NG_SLOT_ID ) {
        return FALSE;
    }

    switch ( lcred_role ) {
    case LCRED_ACTIVE_ROLE:
        return ( id == lcred_primary_id );

    case LCRED_STANDBY_ROLE:
        // if we havn't gone hot we can accept any config, otherwise 
        // just accept for the slot that has gone hot.
        return (video_ctx)? (id == lcred_primary_id) : TRUE;
        break;

    case LCRED_MAX_ROLE:
    default:
        break;
    }

    return FALSE;
}


boolean check_lcred_state (FILE* fp)
{
    if (lcred_mode >= LCRED_MAX_MODE || lcred_role >= LCRED_MAX_ROLE) {
        fprintf(fp, "Command not available for invalid lcred mode/role\n");
        return FALSE;
    }

    if (lcred_role == LCRED_STANDBY_ROLE &&
        lcred_mode == LCRED_SECONDARY_MODE) {
        fprintf(fp, "Command not available in SECONDARY STANDBY role\n");
        return FALSE;
    }

    return TRUE;
}


// Update output time of all output sessions
static void out_session_update_time (void)
{
    int i;
    for (i=0; i<MAX_SESSIONS; i++) {
        if (get_out_session_config(video_ctx, i)->used_flag) {
            out_session_lock(i);
            get_out_session(i)->out_time = sys_clk.base;
            out_session_unlock(i);
        }
    }
}


// Change lcred mode role
//   From lcred_mode/lcred_role to mode/role
//   Note: mode/role may be modified by the call!
//
int sim_video_lcred_change_mode_role (lcred_mode_t* mode, lcred_role_t* role)
{
    switch (*mode) {

    case LCRED_NON_REDUNDANT_MODE:
        // verify that we are in a valid mode and role, and that 
        // we receive a proper message
        if (*role != LCRED_ACTIVE_ROLE) {
            break;
        }
        if ( lcred_mode == LCRED_MAX_MODE && 
             lcred_role == LCRED_MAX_ROLE ) {
            video_prepare_go_hot(slot_id);
            video_go_hot(slot_id);
            video_go_active(slot_id);
#if 0
            // Set up the QAMs
            int i;
            for (i=0; i<NUM_QAMS; i++) {
                get_qam_config(video_ctx, i)->enable_flag = 0;
                get_qam_info(i)->flags |= QAM_FLAG_STATE_CHANGE;
            }
#endif
            goto lcred_done;
        } else if ( lcred_mode == LCRED_PRIMARY_MODE && 
                    lcred_role == LCRED_ACTIVE_ROLE ) {
            // we do nothing
            goto lcred_done;
        } 
        break;

    case LCRED_PRIMARY_MODE:
        switch (*role) {
        // PRIMARY ACTIVE
        case LCRED_ACTIVE_ROLE:
            // verify that we are in a valid mode and role
            if ( lcred_mode == LCRED_PRIMARY_MODE && 
                    lcred_role == LCRED_STANDBY_ROLE) {
                video_go_active(slot_id);
                out_session_update_time();
                goto lcred_done;
                
            } else if ( lcred_mode == LCRED_MAX_MODE && 
                        lcred_role == LCRED_MAX_ROLE ) {
                video_prepare_go_hot(slot_id);
                video_go_hot(slot_id);
                video_go_active(slot_id);
                goto lcred_done;

            } else if ( lcred_mode == LCRED_NON_REDUNDANT_MODE && 
                        lcred_role == LCRED_ACTIVE_ROLE ) {
                // we do nothing
                goto lcred_done;
            }
            break;
        
        // PRIMARY STANDBY
        case LCRED_STANDBY_ROLE:
            // verify that we are in a valid mode and role
            if ( lcred_mode != LCRED_MAX_MODE || 
                 lcred_role != LCRED_MAX_ROLE) {
                break;
            }
            video_prepare_go_hot(slot_id);
            video_go_hot(slot_id);
            goto lcred_done;

        default:
            // Unknown role
            break;
        }
        break;

    case LCRED_SECONDARY_MODE:
        switch (*role) {
        // SECONDARY ACTIVE
        case LCRED_ACTIVE_ROLE:
            // if we havn't received a go hot we cannot go active
            // so instead we go secondary standby
            if (!video_ctx) {
                if ( lcred_mode != LCRED_MAX_MODE || 
                     lcred_role != LCRED_MAX_ROLE ) {
                    break;
                }
                *mode = LCRED_SECONDARY_MODE;
                *role = LCRED_STANDBY_ROLE;
                
                goto lcred_done;
            }

            if ( lcred_mode != LCRED_SECONDARY_MODE || 
                 lcred_role != LCRED_STANDBY_ROLE ) {
                break;
            }
            video_go_active(lcred_primary_id);
            goto lcred_done;
        
        // SECONDARY STANDBY
        case LCRED_STANDBY_ROLE:
            if ( lcred_mode != LCRED_MAX_MODE || 
                 lcred_role != LCRED_MAX_ROLE ) {
                break;
            }
            goto lcred_done;

        default:
            // Unknown role
            break;
        }
        break;

    default:
        // Unknown mode
        return EINVAL;
    }

    return EOK;

lcred_done:
    lcred_mode = *mode;
    lcred_role = *role;
    return EOK;
}


// MSG_TYPE_VIDEO_LCRED_MODE_ROLE message handler
void sim_video_lcred_mode_role_handler (ipc_message *ipc_msg)
{
    msg_card_lcred_mode_role_t* msg = IPC_DATA(ipc_msg);
    VIDEO_MSG_DEBUG("LCRED_MODE_ROLE: mode %s, role %s",
                    lcred_mode_label[msg->mode], lcred_role_label[msg->role]);

    // Note: if we are asked to transition to same mode/role we just ignore
    if ( msg->mode != lcred_mode || msg->role != lcred_role ) {
        int rc = sim_video_lcred_change_mode_role(&msg->mode, &msg->role);
        if (rc == EOK) {
            VIDEO_LOG("Video lcred done");
            return;
        }
    }

    errmsg(ERRMSG_ERR, LC_ERRMSG_LCRED_BAD_MODE_ROLE_TRANSITION, 
           lcred_mode_label[lcred_mode], lcred_role_label[lcred_role],
           lcred_mode_label[msg->mode], lcred_role_label[msg->role]);
}


// MSG_TYPE_VIDEO_LCRED_GO_HOT message handler
void sim_video_lcred_go_hot_handler (ipc_message *ipc_msg)
{
    msg_card_lcred_go_hot_t* msg = IPC_DATA(ipc_msg);
    VIDEO_MSG_DEBUG("LCRED_GO_HOT: primary %d, group %d",
                    msg->primary_slot, msg->group);
    printf("LCRED_GO_HOT: primary %d, group %d",
           msg->primary_slot, msg->group);

    if (lcred_mode != LCRED_SECONDARY_MODE || 
        lcred_role != LCRED_STANDBY_ROLE || 
        (video_ctx)) {
        errmsg(ERRMSG_ERR, LC_ERRMSG_LCRED_UNEXPECTED_GO_HOT,
               "VIDEO", lcred_mode, lcred_role);
        return;
    }

    video_prepare_go_hot(msg->primary_slot);
    video_go_hot(msg->primary_slot);
}


// MSG_TYPE_VIDEO_LCRED_LC_JOINED_GROUP_REQ message handler
void sim_video_lcred_lc_joined_group_handler (ipc_message *ipc_msg)
{
    msg_card_lcred_joined_left_group_t* msg = IPC_DATA(ipc_msg);
    VIDEO_MSG_DEBUG("LCRED_JOINED_GROUP: primary %d, group %d",
                    msg->primary_slot, msg->group);

    if (lcred_mode != LCRED_SECONDARY_MODE ||
        lcred_role != LCRED_STANDBY_ROLE) {
        errmsg(ERRMSG_ERR, LC_ERRMSG_LCRED_UNEXPECTED_JOINED_GROUP,
               "VIDEO", lcred_mode, lcred_role);
        return;
    }

    // Simply ignore the message
}


// MSG_TYPE_VIDEO_LCRED_LC_LEFT_GROUP_REQ message handler
void sim_video_lcred_lc_left_group_handler (ipc_message *ipc_msg)
{
    int rc;
    msg_card_lcred_joined_left_group_t* msg = IPC_DATA(ipc_msg);
    VIDEO_MSG_DEBUG("LCRED_LEFT_GROUP: primary %d, group %d",
                    msg->primary_slot, msg->group);

    if (lcred_mode != LCRED_SECONDARY_MODE ||
        lcred_role != LCRED_STANDBY_ROLE) {
        errmsg(ERRMSG_ERR, LC_ERRMSG_LCRED_UNEXPECTED_LEFT_GROUP,
               "VIDEO", lcred_mode, lcred_role);
        return;
    }

    // Remove all configuration for the corresponding context
    rc = video_context_cleanup(msg->primary_slot);
    if (rc != EOK) {
        errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_CONTEXT_RESET_FAILED,
                "VIDEO", msg->primary_slot, strerror(rc));
    }
}

