/*
 *---------------------------------------------------------------------------
 * ermi_cli_utils.c
 * ERMI CLI Util functions
 *
 * Oct 2008 , Anagha Kandlikar, Neeraj Khurana
 *
 * Copyright (c) 2007-2008 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *---------------------------------------------------------------------------
 */
#include INTERNAL_INC(sup/src/vscm/video_session_api.h)
#include INTERNAL_INC(sup/src/vclient/session_main.h)
#include INTERNAL_INC(sup/src/vclient/ermi/ermi_r_protocol_def.h)
#include INTERNAL_INC(sup/src/vclient/ermi/ermi_cli_util.h)
#include IOS_INC(switch/idbman/sw_idbman.h) 
#include "msg_ermi_c.c"
#include INTERNAL_INC(sup/src/idb/idbman.h)


/* Function to return a swidbs for a given abs_qam_id range
   The end abs_qam_id can be 0 to indicate it is not valid 
   If the end_abs_qam_id is not 0 it should belong to the same slot 
   This function does not work as is , because rfgw_idbman_get_slot_idbs
   does not give the QAM IDBs */   
boolean rfgw_get_qam_idb (uint start_abs_qam_id , uint end_abs_qam_id , 
                          idbtype **start_idb , idbtype **end_idb)
{
    uint slot,port,qam;
    ulong sub_number=0;
    uint count,i;  
    idbtype **idbs;

    if (!start_idb)
        return FALSE;
        
    *start_idb = NULL;
       
    if (end_idb)
       *end_idb = NULL;
       
    slot = OFFSET_TO_CHASSIS_SLOT(start_abs_qam_id);

    /* Check that start and end qam Ids belong to the same slot 
       and start qam id < end qam id */
    if ((end_abs_qam_id != 0) 
    &&  (((OFFSET_TO_CHASSIS_SLOT(end_abs_qam_id) != slot))
    ||   (end_abs_qam_id < start_abs_qam_id)))
        return FALSE;

    port =  OFFSET_TO_PORT(start_abs_qam_id);
    qam =   OFFSET_TO_QAM(start_abs_qam_id);
    printf("%s Logical Start - Qam %d/%d.%d",__FUNCTION__,slot,port,qam);
    
    if (end_abs_qam_id != 0) {
        port =  OFFSET_TO_PORT(end_abs_qam_id);
        qam =   OFFSET_TO_QAM(end_abs_qam_id);
        printf("%s Logical End - Qam %d/%d.%d",__FUNCTION__,slot,port,qam);
    }
    
    
    port =  OFFSET_TO_PHYSICAL_PORT(start_abs_qam_id);
    qam =   OFFSET_TO_PHYSICAL_QAM(start_abs_qam_id);
    sub_number = LCC_PHYSICAL_QAMID(qam);
    rfgw_idbman_get_slot_idbs(slot, &count, &idbs);
    printf("\n %s start qam id %d   %d/%d/%d idb count %d\n",__FUNCTION__,start_abs_qam_id,slot,port,qam,count);

    for (i = 0; i < count; i++) {
       printf("hwidb %s unit %d sub_number %d \n", (idbs[i])->namestring,idbs[i]->hwptr->unit,idbs[i]->sub_number);
       if ((idbs[i]->hwptr->type == IDBTYPE_QAM)
       &&  (idbs[i]->hwptr->unit == port)
       &&  (idbs[i]->sub_number == qam)) {
           /* If start_idb is not set set that first */
           if (!*start_idb) {
               *start_idb = idbs[i];
               buginf("\n Start idb %s",(*start_idb)->hwptr->name);
               printf("\n Start idb %s\n",(*start_idb)->hwptr->name);
               /* If end qam id not specified or
                  end qam id same as start qam id */
               if ((end_abs_qam_id == 0) 
               ||  (end_abs_qam_id == start_abs_qam_id))
                   return TRUE;
                   
               port = OFFSET_TO_PHYSICAL_PORT(end_abs_qam_id);
               qam  = OFFSET_TO_PHYSICAL_QAM(end_abs_qam_id);
               sub_number = LCC_PHYSICAL_QAMID(qam);
               buginf("\n %s end qam id %d - %d/%d/%d ",
               __FUNCTION__,end_abs_qam_id,slot,port,qam);
               printf("\n %s end qam id %d - %d/%d/%d \n",
               __FUNCTION__,end_abs_qam_id,slot,port,qam);
           }
           else { /* set end idb */
               *end_idb = idbs[i];
               buginf("\n end idb %s",(*end_idb)->hwptr->name);
               printf("\n end idb %s\n",(*end_idb)->hwptr->name);
               return TRUE;
           }
       }
    }
    return FALSE;
}

/* Function to return a if names for a given abs_qam_id range
   The end abs_qam_id can be 0 to indicate it is not valid 
   If the end_abs_qam_id is not 0 it should belong to the same slot */
boolean rfgw_print_qam_ifs (uint start_abs_qam_id , uint end_abs_qam_id , 
                            rfgw_action_qam_ifs action_func)
{
    uint slot,start_port,start_qam;
    uint end_port=0,end_qam=0;

    if (!action_func)
        return FALSE;
       
    slot = OFFSET_TO_CHASSIS_SLOT(start_abs_qam_id);

    /* Check that start and end qam Ids belong to the same slot 
       and start qam id < end qam id */
    if ((end_abs_qam_id != 0) 
    &&  (((OFFSET_TO_CHASSIS_SLOT(end_abs_qam_id) != slot))
    ||   (end_abs_qam_id < start_abs_qam_id)))
        return FALSE;

    start_port =  OFFSET_TO_PORT(start_abs_qam_id);
    start_qam =   OFFSET_TO_QAM(start_abs_qam_id);
    if (end_abs_qam_id != 0) {
        end_port =  OFFSET_TO_PORT(end_abs_qam_id);
        end_qam =   OFFSET_TO_QAM(end_abs_qam_id);
    }    
    action_func(slot,start_port,start_qam,end_port,end_qam);
    
    return FALSE;
}


rfgw_ermi_qam_grp_t* rfgw_get_qam_group (char *group_name)
{
    rfgw_ermi_qam_grp_t *qam_group=NULL;
    list_element *curr_elem=NULL;

    if (!group_name)
       return NULL;

    FOR_ALL_DATA_IN_LIST(&rfgw_ermi_params.qam_grp_list, curr_elem, qam_group) {
        /* QAM group name already exists */
        if (!strcmp(qam_group->group_name, group_name)) {
            return qam_group;
        }        
    }
    return NULL;
}

/* TBD: qam-group list needs to be converted to a hash table
    once we start testing with multiple qam-groups in the system. 
    As list search doesn't scale when trying to find a qam-group 
    when a SETUP for a qam in any qam-group comes.
*/    
rfgw_ermi_qam_grp_t* ermi_qam_group_create (char *group_name)
{
    rfgw_ermi_qam_grp_t *qam_group;
    
    if (!group_name)
       return NULL;

    qam_group = rfgw_get_qam_group(group_name);
    /* QAM group name already exists so return that */
    if (qam_group) {
        return qam_group;
    }

    qam_group = chunk_malloc(QAM_GRP_CHUNK);

    if (!qam_group) {
        printf("QAM group %s cannot be created due to lack of memory",group_name);
        assert(qam_group);
        return NULL;
    }

    strncpy(qam_group->group_name,group_name,RFGW_QAM_GRP_NAME_LEN);
    memset(qam_group->qam_bitmap,0,
    sizeof(qam_bitmap_t)*NUMB_MB_QAM_BLOCK_PER_CHASSIS);

    /* Add the new label to the label db */
    if (!list_enqueue(&rfgw_ermi_params.qam_grp_list, &qam_group->list_el, 
                      qam_group)) {
        printf("%%ERROR: Unable to add QAM group [%s] to the list.\n", 
                group_name);
        chunk_free(QAM_GRP_CHUNK,qam_group);
        return (NULL);
    }
    return qam_group;
}


boolean ermi_qam_group_delete (char *group_name)
{
    rfgw_ermi_qam_grp_t *qam_group;
    
    if (!group_name)
       return FALSE;

    /* QAM group name already exists */
    qam_group = rfgw_get_qam_group(group_name);
    if (!qam_group) {
        printf("%%ERROR: QAM group %s does not exist.\n",group_name);
        return (FALSE);
    }

    if (!list_remove(&rfgw_ermi_params.qam_grp_list, &qam_group->list_el, NULL)) {
        printf("%%ERROR: Unable to remove group [%s] from the list.\n",
               qam_group->group_name);
        return (FALSE);
    }
    
    chunk_free(QAM_GRP_CHUNK,qam_group);

    return TRUE;
}


/* Function to add QAM interfaces to the QAM group
 */
boolean rfgw_add_qam_list_to_group(rfgw_ermi_qam_grp_t *qam_group, 
                                   idbtype *start_idb,
                                   idbtype *end_idb, boolean add)
{
    uint slot,port,qam;
    uint logical_portid,logical_qamid;
    uint abs_qam_id,qam_block;
    uint end_abs_qam_id,end_qam_block;
    uint start_bit,end_bit,i; 
   
    if (!start_idb)
        return FALSE;

    slot = start_idb->hwptr->slot;
    port = start_idb->hwptr->unit;
    qam = (start_idb->sub_number)?(start_idb->sub_number - 1) : (start_idb->sub_number);
    logical_portid = LCC_LOGICAL_PORTID(slot, port);
    logical_qamid  = LCC_LOGICAL_QAMID(qam);

    /* Convert the QAM interfaces to absolute QAM ID */
    abs_qam_id = SPQ_TO_OFFSET(slot,logical_portid,logical_qamid);
    qam_block = abs_qam_id/NUMB_MB_QAM_PER_QAM_BLOCK;
    /* Each QAM interface is a bit in the qam_bitmap 
       index in the qam_bitmap array is the qam_block the bit
       belongs to */
    start_bit = abs_qam_id % NUMB_MB_QAM_PER_QAM_BLOCK;

    if (end_idb) {
        slot = end_idb->hwptr->slot;
        port = end_idb->hwptr->unit;
        qam = (end_idb->sub_number)?(end_idb->sub_number - 1) : (end_idb->sub_number);

        logical_portid = LCC_LOGICAL_PORTID(slot, port);
        logical_qamid  = LCC_LOGICAL_QAMID(qam);

        end_abs_qam_id = SPQ_TO_OFFSET(slot,logical_portid,logical_qamid);
        end_qam_block = end_abs_qam_id/NUMB_MB_QAM_PER_QAM_BLOCK;
        if (qam_block != end_qam_block) {
            printf("%%ERROR: QAMs in a list should belong to the same QAM block\n");
            return FALSE;
        } 
        end_bit = end_abs_qam_id % NUMB_MB_QAM_PER_QAM_BLOCK;
        
        if (end_bit < start_bit) {
            printf("%%ERROR: First QAM interface greater than the last QAM.\n");
            return (FALSE);
        }
    }
    else
        end_bit = start_bit;

    for (i = start_bit; i <= end_bit; i++) {
        if (add)
            qam_group->qam_bitmap[qam_block] |= (0x1 << i);
        else
            qam_group->qam_bitmap[qam_block] &= ~(0x1 << i);
    }
    return TRUE;
}

boolean rfgw_get_qam_range ( char *group_name, rfgw_action_qam_ifs action_func)
{
    uint start_abs_qam_id,end_abs_qam_id=0;
    uint qam_block,bit;
    rfgw_ermi_qam_grp_t *qam_group=NULL;
    int start_bit=-1,end_bit=-1;
/*    idbtype  *start_idb=NULL, *end_idb=NULL;*/
   

    if (!group_name)
       return FALSE;

    /* QAM group name already exists */
    qam_group = rfgw_get_qam_group(group_name);
    if (!qam_group) {
        printf("%%ERROR: QAM group %s does not exist.\n",group_name);
        return (FALSE);
    }

    for (qam_block = 0; qam_block < NUMB_MB_QAM_BLOCK_PER_CHASSIS; qam_block++) {
        for (bit = 0; bit < NUMB_MB_QAM_PER_QAM_BLOCK; bit++) {
            if ((qam_group->qam_bitmap[qam_block] >> bit) & 0x1) {
                if (start_bit == -1)  /* Start a new range */ {
                    start_bit = bit;
                }    
                end_bit = bit;
            }
            
            /* found a gap in the range or last qam bit so print the range */
            if ((start_bit != -1)
            &&  ((end_bit != bit) 
            ||   (bit == (NUMB_MB_QAM_PER_QAM_BLOCK - 1)))) {
                start_abs_qam_id = (qam_block * NUMB_MB_QAM_PER_QAM_BLOCK) + start_bit;
                if (start_bit != end_bit) /* it is a range of QAM IDs and not a single QAM */
                    end_abs_qam_id = (qam_block * NUMB_MB_QAM_PER_QAM_BLOCK) + end_bit;
                rfgw_print_qam_ifs(start_abs_qam_id , end_abs_qam_id,action_func);
                start_bit = -1;
                end_abs_qam_id = 0;
            }
        }
    }
    return TRUE;
}


void ermi_qam_grp_init ( void )
{
    /* create QAM group chunk pool */
    rfgw_ermi_params.qam_grp_chunk = chunk_create(sizeof(rfgw_ermi_qam_grp_t), 
                                      RFGW_QAM_GRP_CHUNK_COUNT,
                                     CHUNK_FLAGS_DYNAMIC, NULL, 0, 
                                     "ERMI QAM group");
    if (rfgw_ermi_params.qam_grp_chunk == NULL) {
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "Failed to create ERMI QAM group chunks");
        assert(0);
        return;
    }

    if (!list_create(&rfgw_ermi_params.qam_grp_list, 0, "QAM group List", 0x0)){
        /* Print an error message */
        errmsg(&msgsym(NOCREATE, ERMI_C), 
               "RFGW SSP: %%ERROR: Unable to create QAM group list");
        assert(0);
        return;
    }

}

boolean rfgw_update_stream_zone (rfgw_video_server_group *video_sg, 
                                 char *stream_zone, boolean add)
{
    ermi_r_neighbor_t      *errp_mynode;
    ermi_protocol_info_t *ermi_info;

    ermi_info = video_sg->proto_specific_info;
    if ((!ermi_info) || (!ermi_info->errp_mynode)) {
        printf("%%ERROR: Streaming zone allowed for ERMI protocol only.\n");
        return FALSE;
    }

    errp_mynode = ermi_info->errp_mynode;

    if (add) {
        strncpy(errp_mynode->streaming_zone, stream_zone, ERRP_MAX_NAME_LEN);
    } else {
        memset(errp_mynode->streaming_zone, 0, ERRP_MAX_NAME_LEN);
    }
    
    return TRUE;
}

boolean rfgw_update_component_name (rfgw_video_server_group *video_sg, 
                                    char *component_name, boolean add)
{
    ermi_r_neighbor_t     *errp_mynode;
    ermi_protocol_info_t *ermi_info;

    ermi_info = video_sg->proto_specific_info;
    if ((!ermi_info) || (!ermi_info->errp_mynode)) {
        printf("%%ERROR: Component name allowed for ERMI protocol only.\n");
        return FALSE;
    }

    errp_mynode = ermi_info->errp_mynode;

    if (add) {
        strncpy(errp_mynode->component_name, component_name, ERRP_MAX_NAME_LEN);
    } else {
        memset(errp_mynode->component_name, 0, ERRP_MAX_NAME_LEN);
    }
    
    return TRUE;
}

char* rfgw_get_stream_zone ( rfgw_video_server_group *video_sg)
{
    ermi_r_neighbor_t *errp_mynode;
    ermi_protocol_info_t *ermi_info;

    ermi_info = video_sg->proto_specific_info;
    if ((!ermi_info) || (!ermi_info->errp_mynode))
        return NULL;

    errp_mynode = ermi_info->errp_mynode;
    if (strlen(errp_mynode->streaming_zone))
        return (errp_mynode->streaming_zone);
    else
        return NULL;    
}

char* rfgw_get_component_name ( rfgw_video_server_group *video_sg)
{
    ermi_r_neighbor_t *errp_mynode;
    ermi_protocol_info_t *ermi_info;

    ermi_info = video_sg->proto_specific_info;
    if ((!ermi_info) || (!ermi_info->errp_mynode))
        return NULL;

    errp_mynode = ermi_info->errp_mynode;
    if (strlen(errp_mynode->component_name))
        return (errp_mynode->component_name);
    else
        return NULL;    
}

