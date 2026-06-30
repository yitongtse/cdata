/*
 *------------------------------------------------------------------
 * ermi_r_main.c
 *
 * Main IOS process handling an ERMI-1 connection
 *
 * Dec 2008, Dean Chen
 *
 * Copyright (c) 2008 by cisco Systems, Inc.
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
#include IOS_INC(tcp/tcp.h)
#include IOS_INC(socket/socket.h)
#include IOS_INC(socket/sock_internal.h)
#include IOS_INC(h/logger.h)
#include "ermi_r_protocol_def.h"
#include "ermi_r_errp_fsm.h"
#include "ermi_r_main.h"
#include "ermi_r_io.h"

ermi_r_master_t ermi_r_master;
pid_t          ermi_r_pid = NO_PROCESS;

void ermi_r_main_timer_start (ermi_r_neighbor_t *nbr, 
                             ushort timer_type, ulong timeout)
{
    switch (timer_type) {
        case ERRP_KEEPALIVE_TIMER_TYPE:
            mgd_timer_start(&nbr->keepalive_timer, timeout*ONESEC);
            break;
        case ERRP_HOLD_TIMER_TYPE:
            mgd_timer_start(&nbr->hold_timer, timeout*ONESEC);
            break;
        case ERRP_CONNECT_TIMER_TYPE:
            mgd_timer_start(&nbr->connect_timer, timeout*ONESEC);
            break;
        default:
            break;
    }
}

void ermi_r_main_timer_stop (ermi_r_neighbor_t *nbr, 
                            ushort timer_type)
{
    switch (timer_type) {
        case ERRP_KEEPALIVE_TIMER_TYPE:
            mgd_timer_stop(&nbr->keepalive_timer);
            break;
        case ERRP_HOLD_TIMER_TYPE:
            mgd_timer_stop(&nbr->hold_timer);
            break;
        case ERRP_CONNECT_TIMER_TYPE:
            mgd_timer_stop(&nbr->connect_timer);
            break;
        default:
            break;
    }
}

ermi_r_neighbor_t * ermi_r_nbr_create (ipaddrtype addr, 
                                     boolean server) {
    ermi_r_neighbor_t *nbr;

    if (!ermi_r_master.initialized)
        return NULL;

    nbr = (ermi_r_neighbor_t *)malloc(sizeof(ermi_r_neighbor_t));
    if (nbr) {
        nbr->master = &ermi_r_master;
        nbr->address = addr;
        ermi_r_fsm_init(nbr, server?SERVER_FSM:SENDER_FSM);

        /* assign function vectors */
        nbr->io_connect_func = ermi_r_io_connect;
        nbr->io_disconnect_func = ermi_r_io_disconnect;
        nbr->io_send_func = ermi_r_io_send;
        nbr->io_recv_func = ermi_r_io_recv;
        nbr->timer_start_func = ermi_r_main_timer_start;
        nbr->timer_stop_func = ermi_r_main_timer_stop;

        if (!mgd_timer_initialized(&nbr->keepalive_timer)) {
            mgd_timer_init_parent(&nbr->keepalive_timer,
                                  &nbr->master->master_timer);
            mgd_timer_init_leaf(&nbr->keepalive_timer,
                                &nbr->master->master_timer,
                                ERRP_KEEPALIVE_TIMER_TYPE, 
                                nbr, FALSE);
        }
        if (!mgd_timer_initialized(&nbr->hold_timer)) {
            mgd_timer_init_parent(&nbr->hold_timer,
                                  &nbr->master->master_timer); 
            mgd_timer_init_leaf(&nbr->hold_timer,
                                &nbr->master->master_timer,
                                ERRP_HOLD_TIMER_TYPE, 
                                nbr, FALSE);
        }
        if (!mgd_timer_initialized(&nbr->connect_timer)) {
            mgd_timer_init_parent(&nbr->connect_timer,
                                  &nbr->master->master_timer);
            mgd_timer_init_leaf(&nbr->connect_timer,
                            &nbr->master->master_timer,
                                ERRP_CONNECT_TIMER_TYPE,
                                nbr, FALSE);
        }
        nbr->connect_time = ERRP_DFL_CONN_TIME;
        nbr->hold_time = ERRP_DFL_HOLD_TIME;
        nbr->keep_alive_time = ERRP_DFL_KEEPALIVE_TIME;
        (void)list_enqueue(&ermi_r_master.nbr_list, NULL, nbr);
        return(nbr);
    }

    return NULL;
}

void ermi_r_nbr_destroy (ermi_r_neighbor_t *nbr) {
    (void *)list_remove(&ermi_r_master.nbr_list, NULL, nbr);
    if (nbr->fd) {
        socket_close(nbr->fd);
    }
    free(nbr);
    return;
}

ermi_r_neighbor_t * ermi_r_find_nbr_thru_fd (int fd)
{
    list_element *elem;
    ermi_r_neighbor_t *nbr;

    if (fd < 0)
        return NULL;

    FOR_ALL_DATA_IN_LIST(&ermi_r_master.nbr_list, elem, nbr) {
        if (nbr->fd == fd) {
             return(nbr);
        } 
    }
  
    return NULL;
}

ermi_r_neighbor_t * ermi_r_find_nbr_thru_ip (ipaddrtype addr)
{
    list_element *elem;
    ermi_r_neighbor_t *nbr;

    FOR_ALL_DATA_IN_LIST(&ermi_r_master.nbr_list, elem, nbr) {
        if (nbr->address == addr) {
             return(nbr);
        } 
    }
  
    return NULL;
}

void ermi_r_main_init (void)
{
    sockaddr_in sinserver;
    int optval = 1;

    /* initialize main data structure */
    ermi_r_master.server_fd = -1;
    (void *)list_create(&ermi_r_master.nbr_list, 
                        ERRP_MAX_NBRS, "ERMI-1 ERRP neighbor list",
                        LIST_FLAGS_AUTOMATIC | 
                        LIST_FLAGS_INTERRUPT_SAFE);    
    mgd_timer_init_parent(&ermi_r_master.master_timer, NULL);

    /* initialize server socket */
    ermi_r_master.server_fd = socket_open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ermi_r_master.server_fd == -1) {
        return;
    }
    sinserver.sin_family = AF_INET;
    sinserver.sin_addr.s_addr = INADDR_ANY;
    sinserver.sin_port = ERRP_PORT;
    if (socket_bind(ermi_r_master.server_fd, (sockaddr *) &sinserver, 
                    sizeof(sockaddr_in)) == -1) {
        socket_close(ermi_r_master.server_fd);
        return;
    }
    socket_set_option(ermi_r_master.server_fd, SOL_SOCKET, SO_NBIO, 
                      (void *)&optval, sizeof(optval));
    socket_set_option(ermi_r_master.server_fd, SOL_SOCKET, 
                      SO_REUSEADDR,
                      (char *)&optval, sizeof(optval));
    if (socket_listen(ermi_r_master.server_fd, 
                      ERRP_MAX_ACCEPT_ATTEMPTS) < 0) {
        socket_close(ermi_r_master.server_fd);
        return;
    }
    process_watch_mgd_timer(&ermi_r_master.master_timer, ENABLE);
    process_watch_socket_event(0, ENABLE, RECURRING );

    (void)process_set_socket_interest(ermi_r_master.server_fd, 
                                      SOCKET_READ_EVENT, ENABLE);

    ermi_r_master.initialized = TRUE;
    ermi_r_master.running = TRUE;
    // TBD
    ermi_r_master.errp_id = ip_best_local_address(IPROUTING_DEF_TABLEID, 
                                                 0x7F000001,
                                                 TRUE);
    ermi_r_master.version = ERRP_VERSION;
    ermi_r_master.addr_domain = ERRP_ADDR_DOMAIN_ANY; // TBD, may need to be configured
    ermi_r_master.addr_family = ERRP_ADDR_FAMILY;
    ermi_r_master.app_protocol = ERRP_APP_PROTOCOL;
    ermi_r_master.send_receive_cap = ERRP_SEND_RCV_CAP_SEND_ONLY; // TBD
    ermi_r_master.version_cap = ERRP_VERSION_CAP;
    // TBD, get from CLI 
    strncpy(ermi_r_master.streaming_zone, "DFLT", strlen("DFLT"));
    strncpy(ermi_r_master.component_name, "DFLT", strlen("DFLT"));
    strncpy(ermi_r_master.vendor_str, "DFLT", strlen("DFLT"));
    ermi_r_master.connect_time = ERRP_DFL_CONN_TIME;
    ermi_r_master.hold_time = ERRP_DFL_HOLD_TIME;
    ermi_r_master.keep_alive_time = ERRP_DFL_KEEPALIVE_TIME;
    // TBD
    // ermi_r_master.signalling_ip = ip_best_local_address(IPROUTING_DEF_TABLEID, 
    //                                                    0x7F000001,
    //                                                    TRUE);
    // ermi_r_master.alt_signalling_ip = ip_best_local_address(IPROUTING_DEF_TABLEID, 
    //                                                    0x7F000001,
    //                                                    TRUE);
    return;
}

void ermi_r_main_cleanup (void)
{
    list_element *element;
    list_element *next;
    ermi_r_neighbor_t *nbr;

    ermi_r_master.initialized = FALSE;
    ermi_r_master.running = FALSE;
    if (ermi_r_master.server_fd >= 0) {
        socket_close(ermi_r_master.server_fd);
        ermi_r_master.server_fd = -1;
    }
    mgd_timer_stop(&ermi_r_master.master_timer);

    FOR_ALL_ELEMENTS_IN_LIST_SAVE_NEXT(&ermi_r_master.nbr_list,
                                       element, next) {
        nbr = (ermi_r_neighbor_t *)element->data;
        ermi_r_nbr_destroy(nbr);
    }
    list_destroy(&ermi_r_master.nbr_list);
    memset(&ermi_r_master, 0, sizeof(ermi_r_master_t));
    ermi_r_pid = NO_PROCESS;
}

void ermi_r_main_process_message_event (void)
{
    ulong msg, value;
    void *pointer;
    ermi_r_neighbor_t *nbr;
    ipaddrtype ipaddr;
    int len;

    process_get_message(&msg, &pointer, &value);

    switch (msg) {
        case ERMI_R_MAIN_CONNECT:
            ipaddr = (ipaddrtype)value;
            ERMI_R_DEBUG("ERMI-1 start connecting to %x\n", ipaddr);
            nbr = ermi_r_nbr_create(ipaddr, FALSE);
            if (!nbr) {
                ERMI_R_DEBUG("ERMI-1 failed to connect to %d\n", ipaddr);
                break;
            }
            ermi_r_event(nbr, E_START, NULL);
            break;
        case ERMI_R_MAIN_DISCONNECT:
            nbr = (ermi_r_neighbor_t *)pointer;
            if (!nbr)
                break;
            ermi_r_event(nbr, E_STOP, NULL);
            break;
        case ERMI_R_MAIN_RESTART:
            // TBD, HA PRE s/o, recover states
            break;
        case ERMI_R_MAIN_STOP:
            ermi_r_master.running = FALSE;
            break;

        // TBD
        case ERMI_R_MAIN_SEND_KEEPALIVE:
            break;
        case ERMI_R_MAIN_SEND_UPDATE:
            nbr = (ermi_r_neighbor_t *)pointer;
            // initial ERRP connection; edge input group
            BITMASK_CLEAR_ALL(nbr->attrib_bitmask, ERRP_ATTRIB_MAX_BIT+1);             
            BITMASK_SET(nbr->attrib_bitmask, ERRP_EDGE_INPUT_BIT);
            buginf("\n=== formatting update for %x", nbr->address);
            // TBD, should also set nbr->id to the right interface
            errp_format_update(nbr, nbr->workspace, &len);

            // interface up, add route; QAM
            BITMASK_CLEAR_ALL(nbr->attrib_bitmask, ERRP_ATTRIB_MAX_BIT+1); 
            BITMASK_SET(nbr->attrib_bitmask, ERRP_REACHABLE_ROUTE_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_NEXT_HOP_SERVER_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_QAM_NAMES_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_TOTAL_BANDWIDTH_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_AVAILABLE_BANDWIDTH_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_COST_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_QAM_CHANNEL_CONFIGURATION_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_UDP_MAP_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_SERVICE_STATUS_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_MAX_MPEG_FLOWS_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_NEXT_HOP_ALTERNATE_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_PORT_ID_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_FIBER_NODE_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_QAM_CAPABILITY_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_INPUT_MAP_BIT);
            buginf("\n=== formatting update for %x", nbr->address);
            // TBD, should also set nbr->id to the right interface
            errp_format_update(nbr, nbr->workspace, &len);

            // interface down, remove route; QAM
            BITMASK_CLEAR_ALL(nbr->attrib_bitmask, ERRP_ATTRIB_MAX_BIT+1); 
            BITMASK_SET(nbr->attrib_bitmask, ERRP_WITHDRAWN_ROUTE_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_NEXT_HOP_SERVER_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_QAM_NAMES_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_COST_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_UDP_MAP_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_NEXT_HOP_ALTERNATE_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_FIBER_NODE_BIT);
            BITMASK_SET(nbr->attrib_bitmask, ERRP_QAM_CAPABILITY_BIT);
            // TBD, should also set nbr->id to the right interface
            errp_format_update(nbr, nbr->workspace, &len);

            break;
        case ERMI_R_MAIN_START:
            break;
        case ERMI_R_MAIN_SEND_OPEN:
            break;
        default:
            break;
    }
}

void ermi_r_main_clear_socket_event (int fd, ulong event)
{
    socktype *sock;

    sock = getsock_by_fd(fd);
    socket_clear_event(sock, event);
}

void ermi_r_main_process_socket_event (void)
{
    int the_fd = -1;
    int unsolicit_fd = -1;
    ulong mask_event = 0x0;
    sockaddr_in sinhim, sinpeer;
    uchar buf[ERRP_MAXBYTES];
    int addr_len, optval = 1;
    int count = 0;
    ermi_r_neighbor_t *nbr = NULL, *new_nbr = NULL;
    errp_fsm_event_desc_t event_desc;

    process_get_socket_event(&the_fd, &mask_event);
    if (the_fd == -1)
        return;

    if (ermi_r_master.server_fd == the_fd) {
        if (mask_event & SOCKET_READ_EVENT) {
            ermi_r_main_clear_socket_event(the_fd, mask_event);
            unsolicit_fd = socket_accept(ermi_r_master.server_fd,
                                         (sockaddr *)&sinhim, 
                                         &optval);
            if (unsolicit_fd == -1) {
                return;
            }
            addr_len = sizeof(sinpeer);
            (void)socket_get_peername(unsolicit_fd, 
                                      (sockaddr *)&sinpeer, 
                                      &addr_len);
            new_nbr = ermi_r_nbr_create(sinpeer.sin_addr.s_addr, 
                                       TRUE);
            if (new_nbr) {
                new_nbr->fd = unsolicit_fd;
                new_nbr->connected = TRUE;
                ermi_r_event(new_nbr, E_START, NULL);
            } else {
                return;
            }
            optval = 1;
            socket_set_option(unsolicit_fd, SOL_SOCKET, 
                              SO_NBIO, (void *)&optval, sizeof(optval));
            optval = ERRP_SOC_WINDOW;
            socket_set_option(unsolicit_fd, SOL_SOCKET, SO_RCVBUF,
                              &optval, 
                              sizeof(optval));
            (void)process_set_socket_interest(unsolicit_fd, 
                                              SOCKET_READ_EVENT, ENABLE);
            ermi_r_event(new_nbr, E_CONN_OPEN, NULL);
            return;
        }
    } else {
        if ((nbr = ermi_r_find_nbr_thru_fd(the_fd))) {
            if (mask_event & SOCKET_READ_EVENT) {
                ermi_r_main_clear_socket_event(the_fd, mask_event);
                if (!nbr->connected) {
                    count = socket_recv(nbr->fd, buf, 1, 0);
                    if (count == SO_FAIL) {
                        if (errno == EWOULDBLOCK) {
                            nbr->connected = TRUE;
                            ermi_r_event(nbr, E_CONN_OPEN, NULL);
                        } else {
                            ermi_r_event(nbr, E_CONN_OPEN_FAILED, NULL);
                        }
                    }
                    return;
                }
                count = ermi_r_io_recv(nbr, buf);
                ermi_r_parse_dispatch_msg(nbr, buf, count);
                event_desc.nbr = nbr;
                return;
            }
        }
    }

    if (mask_event & SOCKET_EXCEPTION_EVENT) {
        ermi_r_main_clear_socket_event(the_fd, mask_event);
        socket_close(the_fd);
        ermi_r_event(nbr, E_CONN_FATAL, NULL);
        return;
    }

    ermi_r_main_clear_socket_event(the_fd, mask_event);
}

void ermi_r_main_process_timer_event (void)
{
    mgd_timer *expired_timer;
    ushort timer_type;
    ermi_r_neighbor_t *nbr;

    while (mgd_timer_expired(&ermi_r_master.master_timer)) {
        expired_timer = mgd_timer_first_expired(&ermi_r_master.master_timer);
        if (!expired_timer)
            continue;
        nbr = (ermi_r_neighbor_t *)mgd_timer_context(expired_timer);
        if (!nbr)
            continue;
        timer_type = mgd_timer_type(expired_timer);
        switch (timer_type) {
            case ERRP_KEEPALIVE_TIMER_TYPE:
                ERMI_R_DEBUG("ERMI-1 KEEPALIVE TIMER TIMED OUT\n");
                mgd_timer_stop(&nbr->keepalive_timer);
                ermi_r_event(nbr, E_KEEPALIVE_TIMER_EXP, NULL);
                break;
            case ERRP_HOLD_TIMER_TYPE:
                ERMI_R_DEBUG("ERMI-1 HOLD TIMER TIMED OUT\n");
                mgd_timer_stop(&nbr->hold_timer);
                ermi_r_event(nbr, E_HOLD_TIMER_EXP, NULL);
                break;                
            case ERRP_CONNECT_TIMER_TYPE:
                ERMI_R_DEBUG("ERMI-1 CONNECT TIMER TIMED OUT\n");
                mgd_timer_stop(&nbr->connect_timer);
                ermi_r_event(nbr, E_CONN_RETRY_TIMER_EXP, NULL);
                break; 
            default:
                mgd_timer_stop(expired_timer);
                break;
        }
    }    
}

void ermi_r_main_process_queue_event (void)
{
    // TBD, may need it if recv queue is used
}

void ermi_r_main_process_boolean_event (void)
{
    // TBD, may need it for process control
}

process ermi_r_main_process (void)
{
    unsigned long minor, major;

    process_wait_on_system_init();
    ermi_r_main_init();

    while(ermi_r_master.running) {
        process_wait_for_event();
        while (process_get_wakeup(&major, &minor)) {
            switch (major) {
            case MESSAGE_EVENT:
                ERMI_R_DEBUG("ERMI-1 MESSAGE EVENT\n");
                ermi_r_main_process_message_event();
                break;
            case TIMER_EVENT : 
                ERMI_R_DEBUG("ERMI-1 TIMER EVENT\n");
                ermi_r_main_process_timer_event();
                break;
            case QUEUE_EVENT: 
                ermi_r_main_process_queue_event();
                break;
            case BOOLEAN_EVENT :
                ermi_r_main_process_boolean_event();
                break;
            case SOCKET_EVENT :
                ERMI_R_DEBUG("ERMI-1 SOCKET EVENT\n");
                ermi_r_main_process_socket_event();
                break;
            default :
                break;
            } 
            process_may_suspend();
        } 
    } 

    ermi_r_main_cleanup();
}
