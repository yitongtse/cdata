/*
 * Copyright (c) 2007-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef ___SCS_MESSAGES_H__
#define ___SCS_MESSAGES_H__

#include "ipc.h"
#include "video_def.h"
//#include "../../../../drivers/userspace/ds_jib_encrypt/include/dsjib_encrypt_common.h"  //necessary for shim enums.

//#include "binos/include/common.h"

#ifndef CBR
// Predefined constants to specify descriptor location (for SID scrambling)
// Ugly workaround to temporarily prevent duplicate definition errors
#ifndef __DO_NOT_DEFINE_FRAGMENT_HEADER__
const unsigned long FRAGMENT_HEADER = 0xFFFFFFFE;
const unsigned long FRAGMENT_ALL = 0xFFFFFFFD;
#endif //  __DO_NOT_DEFINE_FRAGMENT_HEADER__
#endif //ifndef CBR

#define	MAX_PIDS_PER_PROGRAM    32
#define	PID_NOT_AVAILABLE       0xFFFF

#ifndef CBR // Defined in the TDL file in CBR platform
/* IPC Message Types */
typedef enum{
    MSG_TYPE_SET_CW_INDEX,          //Sets the CW index to each PID that will be scrambled using the same Control Word
    MSG_TYPE_START_ECM_PLAYOUT,     //Passes the ecm_info_ptr index to initiate playout
    MSG_TYPE_REMOVE_ECM_PLAYOUT,
    MSG_TYPE_ADD_CA_DESCRIPTOR,     //Passes the CA descriptor buffer and PMT placement info
    MSG_TYPE_REMOVE_CA_DESCRIPTOR,  //Passes the CA descriptor buffer and PMT placement info in order to find an remove CA descriptor.
    MSG_TYPE_NEW_PID,               //Notifies the scrambler of the presence of a PID on a carrier (as read from the PMT)
    MSG_TYPE_PID_GONE,              //Notifies the scrambler that the PID was removed on a carrier (as read from the PMT)
    MSG_TYPE_SERVICE_CHANGE,        //Notifies the scrambler of the list of PIDs and SID, added or removed on a carrier (as read from the PMT)
    MSG_TYPE_GET_SID,               //Request from SCS for the availability of a service on a QAM.!!!!SHOULD THIS BE A NON-BLOCKING/NON-RELIABLE MESSAGE!!!!!
    MSG_TYPE_SID_EXISTS,            //Response from VIOP to SCS that the program is available
    MSG_TYPE_NO_SID,                //Response from VIOP to SCS that the program is unavailable
    MSG_TYPE_GET_PID,               //Request from SCS to VIOP as to the availability of a PID on a QAM
    MSG_TYPE_PID_EXISTS,            //Response from VIOP to SCS that the pid is available
    MSG_TYPE_NO_PID,                //Response from VIOP to SCS that the pid is unavailable
    MSG_TYPE_NEW_TS,                //Notifies the SCS of a newly available TSID for scrambling
    MSG_TYPE_REMOVE_TS,             //Notifies the SCS that a ONID/TSID is no longer available for scrabling (ex. data qam)
    MSG_TYPE_ONID_TSID_CHANGE,      //Notifies the SCS of a ONID/TSID value change
    MSG_TYPE_UPDATE_SERVICE,        //Notifies the SCS that a service has changed (see msg_update_service below)
    MSG_TYPE_PID_PROVISIONED,       //Notifies the SCS to the status of an ECM pid request (see msg_pid_provisioned below)
    MSG_TYPE_SET_PID_SCRAMBLED,     //Pid scrambling status from SCS to VIOP necessary for user display.
    MSG_TYPE_RESERVE_PID,           //Request from SCS to VIOP reserving a specific ECM pid (see msg_reserve_pid below)
    MSG_TYPE_REQUEST_PID,           //Request from SCS to VIOP for any available ECM pid on the carrier (see msg_request_pid below)
    MSG_TYPE_SID_ECM_PID,           //Request from SCS to VIOP for an ECM pid from the pid block for service SID (see msg_sid_ecm_pid below)
    MSG_TYPE_RELEASE_PID,           //Request from SCS to VIOP to free the Requested/Reserved ecm pid (see msg_release_pid below)
    MSG_TYPE_SCS_INIT_DONE,         //Notifies the VMH that the SCS Process is ready for configuration parameters
    MSG_TYPE_SCS_READY,             //Notifies the VMH that the SCS Process has been configured and is ready to scramble.
    MSG_TYPE_EIS_ALL_IPC_UP,        //Notifies the root process all EIS LC IPC and TCP servers are ready.
    MSG_TYPE_SCS_ALL_IPC_UP,
    MSG_TYPE_EIS_PROXY_ALL_IPC_UP,
    CLI_MSG_EIS_PROXY,
    CLI_MSG_EIS_PROXY_CONFIG,
    CLI_MSG_ECMG_PROXY,
    CLI_MSG_ECMG_PROXY_CONFIG,
    CLI_MSG_ECMG_CONNECTION,
    CLI_MSG_ECMG_CONNECTION_CONFIG,
    CLI_MSG_ECMG_OVERRULE_DELAYS,
    CLI_MSG_ECMG_OVERRULES,
    CLI_MSG_ECMG_PK_HINT_BIT,
    CLI_MSG_ECMG_DESCR_RULE,
    CLI_MSG_ECMG_DESCR_RULE_CONFIG,
    //TEST MESSAGES BELOW HERE
    TEST_MSG_TYPE_GETCWINDEX,
    TEST_MSG_TYPE_GETRANDOMNUMBER,
    TEST_MSG_TYPE_SETSCRAMBLEROFF, 
    TEST_MSG_TYPE_SETSCRAMBLEREVEN,
    TEST_MSG_TYPE_SETSCRAMBLERODD,
    TEST_MSG_TYPE_DMA_CW_PKTS, 
    TEST_MSG_TYPE_REQUESTECMINFOPTR,
    TEST_MSG_TYPE_SETCWINDEX,
    TEST_MSG_TYPE_STARTECMPLAYOUT,
    TEST_MSG_TYPE_SETHINTBIT,
    TEST_MSG_TYPE_STOPSCRAMBLING,
    TEST_MSG_TYPE_STOPECMPLAYOUT,
    TEST_MSG_TYPE_DUMPECM,
    TEST_MSG_TYPE_ADDCADESCR,
    TEST_MSG_TYPE_REMOVECADESCR,
    TEST_MSG_TYPE_EIS_SHOW_LOG_SETTINGS,
    TEST_MSG_TYPE_EIS_SET_LOGGING,
    TEST_MSG_TYPE_SHOW_SCG_CONCISE,
    TEST_MSG_TYPE_RET_SCG_ON_QAM,
    TEST_MSG_TYPE_SHOW_SCG_DETAILS,
    TEST_MSG_TYPE_EIS_TO_SCS_MSG,
    TEST_MSG_TYPE_XML_OBJ_DUMP,
    TEST_MSG_TYPE_SHOWSCSLOGFILTER,
    TEST_MSG_TYPE_SETSCSLOGFILTER,
    TEST_MSG_TYPE_SHOWEISTRACEFILTER,
    TEST_MSG_TYPE_SETEISTRACEFILTER,
    TEST_MSG_TYPE_GET_FREE_PROXY_INDEX,
    TEST_MSG_TYPE_SCG_XML_DUMP,
    TEST_MSG_TYPE_GET_SCS_HEAP
}scs_msg_type;

typedef enum{
    MSG_TYPE_SET_SCRAMBLER
}scs_shim_msg_type;
#endif //#ifndef CBR

#if 0
typedef enum{
    esVideoComponent,          ///< Video
    esAudioComponent,          ///< Audio
    esDataComponent,           ///< Data
    esUnknownComponent         ///< Unknown
}en_stream_type_t;

typedef struct{
    uint16_t pid;                       //< PID of elementary stream
    en_stream_type_t m_en_stream_type;  //< Stream type of component
} es_pid_t;
#endif

/* CA descriptor type */
typedef struct{
    uint32_t    m_ulTableIdExt;         // For outer loop use SID, for inner loop use FRAGMENT_HEADER
    uint32_t    m_ulLocation;           // Use Pid for one inner loop, use FRAGMENT_HEADER for one outer, use FRAGMENT_ALL for all inner
    uint16_t    descr_size;             // CA descriptor size
    uint8_t     m_pDescriptor[0];       // CA descriptor buffer to be inserted.
}ca_descr_t;

/* RFGW IPC Control Word Index message
* Usage:    Assigns a Control word Index to each Pid on a logical Qam
* Returns:  This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_scs_cw_index_ {
    uint16_t    qam_id;     // Qam location of Pid
    uint16_t    pid;        //Pid value
    uint16_t    p_cw;       //control word index to be assigned.
} msg_scs_cw_index_t;

/* RFGW IPC ECM Insert IPC message
* Usage: Adds the ecmInfo_Index to the playout carousel of the VIOP so the
*           ECM's can be played out.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_start_ecm_playout_ {
    uint16_t    qam_id;           // Qam location of ECM insert
    uint16_t    ecm_info_index;   // Index location of pointer to ecm_info_t structure
} msg_start_ecm_playout_t;


/* RFGW IPC Add CA descriptor IPC message
* Usage:    Adds the CA descriptor to the output PMT of the scrambled service
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_add_ca_descriptor_ {
    uint16_t    qam_id;     // Qam location of ECM insert
    ca_descr_t  descriptor; // CA descriptor buffer and placement info
} msg_add_ca_descriptor_t;


/* RFGW IPC Remove CA descriptor IPC message
* Usage: Remove the CA descriptor from the output PMT of the scrambled service
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_remove_ca_descriptor_ {
uint16_t        qam_id;     // Qam location of ECM insert
ca_descr_t      descriptor; // CA descriptor buffer and placement info
} msg_remove_ca_descriptor_t;

/* RFGW IPC New Pid Notification message
* Usage: Nofify the SCS that the Pid is available on the carrier in order to
*           Scramble
* NOTE: On a PID (value) change, two notifications should be called:
*           a msg_pid_gone notification for the old PID value and a
*           msg_new_pid notification for the new PID value
* NOTE: Both service (see msg_update_service) and component
*           (see msg_new_pid and see msg_pid_gone) notifications are required
*           on dynamic changes!
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_new_pid_ {
    uint32_t    qam_id;     // unique Logical Qam location of New Pid
    uint16_t    pid;        // New Pid value
} msg_new_pid_t;

/* RFGW IPC Removed Pid Notification message
* Usage: Nofify the SCS that the Pid is no longer available on the carrier to
*           Scramble. (usually triggers an alarm)
* NOTE: Both service (see msg_update_service) and component
*           (see msg_new_pid and see msg_pid_gone) notifications are required
*           on dynamic changes!
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_pid_gone_ {
#ifndef CBR
    rfgw_msg_hdr_t  header; // version info.....
#endif
    uint32_t    qam_id;     // unique Logical Qam location of Removed Pid
    uint16_t    pid;        // Removed Pid Value
} msg_pid_gone_t;

#if 0
//#define ES_ACTION_ADD           0
//#define ES_ACTION_DELETE        1

typedef struct {
    uint16_t action      : 1;     // ES_ACTION_ADD or ES_ACTION_DELTE
    uint16_t stream_type : 2;     // same value as en_stream_type_t
    uint16_t pid         : 13;    // PID value
} es_change_t;

#define MAX_UPDATES             32

typedef struct {
    int count;
    es_change_t table[MAX_UPDATES];
} pmt_change_t;
#endif


/* RFGW IPC Pid Changes Notification message
* Usage: Nofify the SCS about Pids that are newly present and pids that are newly missing on the
*           carrier to Scramble.
* NOTE: Both service (see msg_update_service) and component
*           (see msg_pid_changes) notifications are required
*           on dynamic changes!
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_service_change_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;      // version info.....
#endif
    uint32_t        qam_id;      // unique Logical Qam location of changed PIDs
    pmt_change_t    pmt_change;  // Pid changes structure
    uint16_t        sid;         // Program number of changed service
    uint16_t        num_pids;    // number of es pids
    es_pid_t        pid_list[MAX_PIDS_PER_PROGRAM]; // list of all current ES's of the service
} msg_service_change_t;

/* RFGW IPC Request for Program availability message
* Usage: SCS to VIOP as to the availabilty of a program on the carrier.
*           This message should be followed by a msg_sid_exist or msg_no_sid
*           (This message is necessary because SCS does not store the
*           updateServiceInScrambler message or New Pid if there is no SCG )
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_get_sid_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        qam_id;     // unique Logical Qam location of Removed Pid
    uint16_t        sid;        // Program number to retrieve
    uint64_t        p_object;   // pointer to the object used on the response
} msg_get_sid_t;

/* RFGW IPC Response to msg_get_sid if program is available.
* Usage: VIOP to SCS signalling the program is available on the carrier.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_sid_exists_{
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint64_t        p_object;   // pointer to the object used on the response
    uint16_t        sid;        // Program number of retrieval request
    uint16_t        num_pids;   // Number of ES pids associated with the service
    es_pid_t        pid_list[MAX_PIDS_PER_PROGRAM];// Individual Pid Info (Pid number and Type)
}msg_sid_exists_t;

/* RFGW IPC Response to msg_get_sid if program is unavailable.
* Usage: VIOP to SCS signalling the program is not available on the carrier.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_no_sid_{
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint16_t        sid;        // Program number of retrieval request
    uint64_t        p_object;   // pointer to the object used on the response
}msg_no_sid_t;

/* RFGW IPC Request for Pid availability message
* Usage: SCS to VIOP as to the availabilty of a Pid on the carrier.
*           This message should be followed by a msg_pid_exists or msg_no_pid
*           (This message is necessary because SCS does not store the
*           msg_new_pid or msg_pid_gone message)
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*           (uses ipc_send_message_blocked)
*/
typedef struct msg_get_pid_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        qam_id;     // unique Logical Qam location of Removed Pid
    uint16_t        pid;        // Pid number to retrieve
    uint64_t        p_object;   // pointer to the object used on the response
} msg_get_pid_t;

/* RFGW IPC Response to msg_get_pid if pid is available
* Usage: VIOP to SCS signalling the pid is available on the carrier.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_pid_exists_{
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint16_t        pid;        // pid number of retrieval request
    uint64_t        p_object;   // pointer to the object used on the response
}msg_pid_exists_t;

/* RFGW IPC Response to msg_get_pid if pid is unavailable.
* Usage: VIOP to SCS signalling the pid is not available on the carrier.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_no_pid_{
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint16_t        pid;        // pid number of retrieval request
    uint64_t        p_object;   // pointer to the object used on the response
}msg_no_pid_t;

/* RFGW IPC Reserve Pid Request to VIOP
* Usage: SCS to VIOP Requesting for a specific ECM Pid Allocation.  This
*           message should be followed by a msg_pid_provisioned.
*           NOTE: If Reserved pid is not available then the VIOP should
*           Return any available pid on the carrier!!!!
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
* NOTE: p_object is not modified by VIOP but rather just returned as is so
*           so SCS can find the correct object by typecasting back.
*/
typedef struct msg_reserve_pid_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        qam_id;     // unique Logical Qam location for ECM pid
    uint16_t        pid;        // pid number to reserve
    uint64_t        p_object;   // pointer to SCS object requesting pid
}msg_reserve_pid_t;


/* RFGW IPC Request to VIOP for ANY available ECM pid
* Usage: SCS to VIOP Requesting for any ECM pid to use.  This
*           message should be followed by a msg_pid_provisioned
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
* NOTE: p_object is not modified by VIOP but rather just returned as is so
*           so SCS can find the correct object by typecasting back.
*/
typedef struct msg_request_pid_ {
    uint32_t        qam_id;     // unique Logical Qam location for ECM pid
    uint64_t        p_object;   // pointer to SCS object requesting pid
}msg_request_pid_t;


/* RFGW IPC Request ECM Pid Request to VIOP from service SID pid block
* Usage: SCS to VIOP Requesting for an ECM Pid Allocation.  This
*           message should be followed by a msg_pid_provisioned.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
* NOTE: p_object is not modified by VIOP but rather just returned as is so
*           so SCS can find the correct object by typecasting back.
*/
typedef struct msg_sid_ecm_pid {
    uint32_t        qam_id;     // unique Logical Qam location for ECM pid
    uint16_t        sid;        // sid number for the pid block to get the ECM pid from
    uint64_t        p_object;   // pointer to SCS object requesting pid
} msg_sid_ecm_pid_t;


/* RFGW IPC Reply to msg_reserve_pid or msg_request_pid
* Usage: VIOP to SCS providing the availability status of the pid request.
*           If the pid is available use the pid value if not PID_NOT_AVAILABLE
*           should be used.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
* NOTE: p_object is not modified by VIOP but rather just returned as is so
*           so SCS can find the correct object by typecasting back.
*/
typedef struct msg_pid_provisioned_ {
    uint32_t        qam_id;     // unique Logical Qam location for ECM pid
    uint16_t        pid;        // pid number available or PID_NOT_AVAILABLE if unavailable
    uint64_t        p_object;   // pointer to SCS object requesting pid
}msg_pid_provisioned_t;



/* RFGW IPC Service change notification message
* Usage: This notification is from VIOP to SCS and needs to be sent whenever
*           a service changes:
*               - new service
*               - new component(s)
*               - component(s) removed
*               - PID of component changed
*               - service removed (invoke call with empty component list!)
*
* NOTE: On a SID change, this notification should be called twice:
*           - once for the old SID, as a service remove (empty component list!)
*           - once for the new SID, as a new service (containing all components)
* NOTE: Both service (see msg_update_service) and component (see msg_new_pid
*           and see msg_pid_gone) notification are required on dynamic changes!
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_update_service_ {
    uint32_t        qam_id;     // unique Logical Qam location of service change
    uint16_t        sid;        // Program number of changed service
    uint16_t        num_pids;   // number of es pids
    es_pid_t        pid_list[MAX_PIDS_PER_PROGRAM];// list of all ES's of the servie
}msg_update_service_t;

/* RFGW IPC TSID/ONID change notification message
* Usage: This notification is from VIOP to SCS and needs to be sent whenever
*           there is a change to the TSID/ONID on a carrier.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_change_on_onid_tsid_ {
    uint32_t        qam_id;     // unique Logical Qam location of ONID/TSID change
    uint16_t        old_onid;   // old ONID of TS
    uint16_t        new_onid;   // new ONID of TS
    uint16_t        old_tsid;   // old TSID of TS
    uint16_t        new_tsid;   // new TSID of TS
}msg_change_on_onid_tsid_t;

/* RFGW IPC Removed TSID/ONID notification message
* Usage: This notification is from VIOP to SCS and needs to be sent whenever
*           a carrier is no longer available to scramble (for example DATA mode)
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_remove_onid_tsid_ {
    uint32_t        qam_id;     // unique Logical Qam location of removed ONID/TSID
    uint16_t        onid;       // ONID of removed TS
    uint16_t        tsid;       // TSID of removed TS
}msg_remove_onid_tsid_t;


/* RFGW IPC New TSID/ONID notification message
* Usage: This notification is from VIOP to SCS and needs to be sent whenever
*           a carrier is available to scramble.
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_new_onid_tsid_ {
    uint32_t        qam_id;     // unique Logical Qam location of new ONID/TSID
    uint16_t        onid;       // ONID of new TS
    uint16_t        tsid;       // TSID of new TS
}msg_new_onid_tsid_t;


/* RFGW IPC Status of Pid Scrambling message
* Usage: SCS to VIOP providing the new scrambling status of a pid (newly
*           scrambled or newly unscrambled).  This function should be used to
*           notify the user that a pid (or sid) is actively being scrambled
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_set_pid_scrambled_ {
    uint32_t        qam_id;     // unique Logical Qam location of pid
    uint16_t        pid;        // pid number
    boolean         scrambled;  // new status (either newly scrambled(true) or newly unscrambled (false)
}msg_set_pid_scrambled_t;

/* RFGW IPC Release ECM pid message
* Usage: SCS to VIOP message free'ing the allocated ECM pid back to the
*           available pool
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_release_pid_ {
    uint32_t        qam_id;     // unique Logical Qam location of ecm pid
    uint16_t        pid;        // released ecm pid value
}msg_release_pid_t;

/* RFGW IPC SCS Process Initialization Done Message
* Usage: SCS to VMH message, signaling that the SCS is ready to accept config
*           parameters
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_scs_init_done_ {
    boolean         init_done;  // should always be true (unless something failed during startup, i.e. queue failed)
}msg_scs_init_done_t;

/* RFGW IPC SCS has been initialized and Parameters configured Message
* Usage: SCS to VMH message, signaling that the SCS is ready to scramble
*           parameters
* Returns: This message is sent as a blocking reliable IPC message
*           because their is not a higher level handshake mechanism in place
*/
typedef struct msg_scs_ready_ {
    boolean         ready;      // should always be true (unless invalid parameters were passed)
}msg_scs_ready_t;

////////////////////////// TEST MESSAGE STRUCTS BELOW HERE ////////////////////////////////////////////
typedef struct msg_test_getcwindex_ {
    uint32_t        qam_id;
}msg_test_getcwindex_t;

typedef struct msg_test_getrandomnumber_ {
}msg_test_getrandomnumber_t;

typedef struct msg_test_scrambleroff_ {
    uint32_t        qam_id;
    uint32_t        cw_index;
}msg_test_scrambleroff_t;

typedef struct msg_test_scramblereven_ {
    uint16_t        qam_id;
    uint32_t        cw_index;
}msg_test_scramblereven_t;

typedef struct msg_test_scramblerodd_ {
    uint32_t        qam_id;
    uint32_t        cw_index;
}msg_test_scramblerodd_t;

typedef struct msg_test_dmacwpkts_ {
}msg_test_dmacwpkts_t;

typedef struct msg_test_requestecminfoptr_ {
    uint16_t        logicalQam; // 
}msg_test_requestecminfoptr_t;

typedef struct msg_test_setcwindex_ {
    uint32_t        qam_id;
    uint32_t        cw_index;
    uint16_t        pid;
}msg_test_setcwindex_t;

typedef struct msg_test_startecm_ {
    uint32_t        ecm_info_index;
    uint32_t        ecm_pid;
}msg_test_startecm_t;

typedef struct msg_test_sethintbit_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        ecm_info_index;
}msg_test_sethintbit_t;

typedef struct msg_test_stopscrambling_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        qam_id;
    uint32_t        cw_index;
}msg_test_stopscrambling_t;

typedef struct msg_test_stopecm_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        ecm_info_index;
}msg_test_stopecm_t;

typedef struct msg_test_dumpecm_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        ecm_info_index;
}msg_test_dumpecm_t;

typedef struct msg_test_xml_obj_dump_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean         thread_safe;
}msg_test_xml_obj_dump_t;

typedef struct msg_test_addcadescr_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        qam_id;
    uint32_t        ecm_pid;
    uint32_t        sid;
}msg_test_addcadescr_t;

typedef struct msg_test_removecadescr_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        qam_id;
    uint32_t        ecm_pid;
    uint32_t        sid;
}msg_test_removecadescr_t;

typedef struct msg_test_showlogfilter_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
}msg_test_showlogfilter_t;

typedef struct msg_test_setlogfilter_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    uint32_t        log_sev;		
    uint32_t        filter_value;
}msg_test_setlogfilter_t;

typedef enum {
    EIS_DEBUG_LEVEL_ERROR,
    EIS_DEBUG_LEVEL_TXTRACE,
    EIS_DEBUG_LEVEL_TRACE,
    EIS_DEBUG_LEVEL_DETAIL_TRACE,
    EIS_DEBUG_LEVEL_ALL
}eis_debug_levels_t;

typedef struct {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    int level;
    unsigned char enable;
} eis_enable_debug_t;

// type used to send a generic message from the EIS to the SCS
typedef struct {
    uint16_t  len;        // CA descriptor size
    uint8_t   val[0];     // CA descriptor buffer to be inserted.
}msg_eis_to_scs_msg_t;

#define GET          (FALSE)
#define SET          (TRUE)
#define GETSTR       "show"
#define GETSTRLEN    2        //only need this long to distinguish show from set
#define CREATE       (FALSE)
#define DELETE       (TRUE)
#define CREATESTR    "add"
#define CREATESTRLEN 1        //only need this long to distinguish add from delete

enum enEisType {
    eSaEis,
    eCaVendorEis
};

enum enEcmgType_V2 {
    eEcmgType_Standard = 0x00,
    eEcmgType_Irdeto =   0x01,
    eEcmgType_Nagra =    0x02,
    eEcmgType_Hitachi =  0x03,
    eEcmgType_PowerKey = 0x05
};

enum enEcmPidSource {
    eEcmPidSource_EcmId,
    eEcmPidSource_Auto,
    eEcmPidSource_Sid
};

enum enEcmgChannelIdMode {
    eEcmgChannelIdManual,
    eEcmgChannelIdAuto
};

enum enEcmgDescRuleType {
    eAddPrivateData,
    eDoNotInsertDescriptor
};

enum enDescInsertLvl {
    eFollowScg,
    eInsertAtESLevel,
    eInsertAtServiceLevel
};

typedef struct msg_test_get_free_proxy_index_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
}msg_test_get_free_proxy_index_t;

typedef struct msg_proxy_id_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   createDelete;
    uint32_t                  proxy_id;
}msg_proxy_id_t;

typedef struct msg_eis_config_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   getSet;
    uint32_t                  proxy_id;
    char                      name[20+1];
    enum enEisType            type;
    uint16_t                  port;
    boolean                   overrule;
    uint32_t                  cp;
}msg_eis_config_t;

typedef struct msg_ecmg_config_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   getSet;
    uint32_t                  proxy_id;
    char                      name[20+1];
    enum enEcmgType_V2        type;
    uint16_t                  cas_id;
    uint16_t                  sub_cas_id;
    enum enEcmPidSource       ecm_pid_source;
    uint16_t                  ecm_lower_limit;
    uint16_t                  ecm_upper_limit;
    enum enEcmgChannelIdMode  channel_id_mode;
}msg_ecmg_config_t;

typedef struct msg_ecmg_connection_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   createDelete;
    uint32_t                  proxy_id;
    uint32_t                  conn_id;
}msg_ecmg_connection_t;

typedef struct msg_ecmg_conn_config_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   getSet;
    uint32_t                  proxy_id;
    uint32_t                  conn_id;
    uint32_t                  ip_address;
    uint16_t                  port;
    uint16_t                  channel_id;
    int32_t                   priority;
}msg_ecmg_conn_config_t;

typedef struct msg_ecmg_overrule_delays_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   getSet;
    uint32_t                  proxy_id;
    boolean                   overrule_transition_start_delay;
    short                     transition_start_delay;
    boolean                   overrule_transition_stop_delay;
    short                     transition_stop_delay;
    boolean                   overrule_start_delay;
    short                     start_delay;
    boolean                   overrule_stop_delay;
    short                     stop_delay;
    boolean                   overrule_AC_start_delay;
    short                     AC_start_delay;
    boolean                   overrule_AC_stop_delay;
    short                     AC_stop_delay;
}msg_ecmg_overrule_delays_t;

typedef struct msg_ecmg_overrules_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   getSet;
    uint32_t                  proxy_id;
    boolean                   overrule_max_comp_time;
    short                     max_comp_time;
    boolean                   overrule_min_CP_duration;
    uint32_t                  min_CP_duration;
    boolean                   overrule_max_streams;
    unsigned short            max_streams;
    boolean                   overrule_ecm_repetition;
    unsigned short            ecm_repetition;
}msg_ecmg_overrules_t;

typedef struct msg_ecmg_PK_hint_bit_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   getSet;
    uint32_t                  proxy_id;
    boolean                   overrule_skip_CP;                 //clear to scrambling delay
    short                     skip_CP;
    boolean                   overrule_HB_initial_stop_delay;   //delay before turning off initial ECM HB (the new ECM)
    short                     HB_initial_stop_delay;
    boolean                   overrule_HB_stop_delay;           //delay before turning off HB (the new ECM)
    short                     HB_stop_delay;
    boolean                   overrule_HB_start_delay;          //delay before turning on HB (before the new ECM)
    short                     HB_start_delay;
}msg_ecmg_PK_hint_bit_t;

typedef struct msg_ecmg_descriptor_rule_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   createDelete;
    uint32_t                  proxy_id;
    uint32_t                  rule_id;
}msg_ecmg_descriptor_rule_t;

typedef struct msg_ecmg_desc_rule_config_ {
#ifndef CBR
    rfgw_msg_hdr_t  header;     // version info.....
#endif
    boolean                   getSet;
    uint32_t                  proxy_id;
    uint32_t                  rule_id;
    char                      rule_name[30+1];
    enum enEcmgDescRuleType   type;
    enum enDescInsertLvl      level;
    char                      data[100+1];
    uint32_t                  ecm_id;                //TODO make a TLV!
}msg_ecmg_desc_rule_config_t;

typedef struct msg_test_get_generic_ {
#ifndef CBR
    rfgw_msg_hdr_header;        // version info.....
#endif
}msg_test_get_generic_t;

#if 0
/*********************************************************************
 * Shim Layer messages, when VEMAN is converted to 64-bit and the shim
 * layer is no longer needed these can be removed
 */

/* The "bits" and "encrypt_type" enums are defined in the dsjib_encrypt_common.h which is now included by this file.
 */
typedef struct msg_set_scrambler_ {
    uint32_t                cw_index;     /* CW index to write the scrambling info */
    scr_bits_t              bits;         /* Scrambler bits applied to outgoing pkts */
    core_enc_t              encrypt_type; /* core encryption type */
    uint64_t                keyword[2];   /* Array of keys, not all used by all core encryption types */
    uint64_t                key_iv[2];    /* Extra array of keys used by AES encryption */
}msg_set_scrambler;
#endif

#endif  // ___SCS_MESSAGES_H__

