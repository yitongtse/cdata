/*
 *-----------------------------------------------------------------------------
 * ermi_cli_util.h - ERMI-Video EQAM CLI - Utility Functions
 *
 * Oct-2008, Anagha Kandlikar, Neeraj Khurana
 *
 * Copyright (c) 2008 by cisco Systems, Inc.
 * All rights reserved.
 *-----------------------------------------------------------------------------
 */

#ifndef __ERMI_CLI_UTIL_H__
#define __ERMI_CLI_UTIL_H__

#include INTERNAL_INC(sup/qam/include/rfgw_lcc.h)
#include INTERNAL_INC(sup/include_private/qam_domain.h)
#include INTERNAL_INC(sup/src/vclient/ermi/ermi_video_main.h)

#define QAM_GRP_CHUNK   (rfgw_ermi_params.qam_grp_chunk)

#define RFGW_QAM_GRP_CHUNK_COUNT    200

typedef struct {
    chunk_type   *qam_grp_chunk;	   /* rfgw_ermi_qam_grp_t */
    list_header  qam_grp_list;		   /* rfgw_ermi_qam_grp_t */
    list_header  errp_nbr_list;            /* ermi_r_neighbor_t */
}rfgw_ermi_params_t;

extern rfgw_ermi_params_t  rfgw_ermi_params;

extern void ermi_qam_grp_init( void );
extern rfgw_ermi_qam_grp_t* ermi_qam_group_create (char *group_name);
extern boolean ermi_qam_group_delete (char *group_name);
extern rfgw_ermi_qam_grp_t* rfgw_get_qam_group (char *group_name);
extern boolean rfgw_add_qam_list_to_group(rfgw_ermi_qam_grp_t *qam_group, idbtype *start_idb,
                                   idbtype *end_idb, boolean add);
typedef void (*rfgw_action_qam_ifs)( uint slot, uint st_port, uint st_qam, uint end_port, uint end_qam ) ; 

extern boolean rfgw_get_qam_range ( char *group_name, rfgw_action_qam_ifs action_func);

extern boolean rfgw_update_component_name ( rfgw_video_server_group *video_sg, 
                                            char *component_name, boolean add);

extern boolean rfgw_update_stream_zone ( rfgw_video_server_group *video_sg, 
                                         char *stream_zone, boolean add);

extern char* rfgw_get_stream_zone ( rfgw_video_server_group *video_sg);

extern char* rfgw_get_component_name ( rfgw_video_server_group *video_sg);



#endif __ERMI2_SESSION_UTIL_H__

