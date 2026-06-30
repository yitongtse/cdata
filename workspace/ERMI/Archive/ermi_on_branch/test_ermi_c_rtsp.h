/*
 *-------------------------------------------------------------------
 * test_ermi_c_rtsp.h - test ermi-c rtsp - CLIs
 *
 * 02/09/04, Linda Hua
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

/***************************************
 * test ermi-c rtsp client cmds 
 */

EOLS (ermi_c_rtsp_client_stop_eol, ermi_c_test_rtsp_cmds, 
                               ERMI_C_RTSP_CLIENT_STOP);
KEYWORD (ermi_c_rtsp_client_stop, ermi_c_rtsp_client_stop_eol, no_alt,
        "stop", "stop client", PRIV_ROOT);



EOLS (ermi_c_rtsp_mem_eol, ermi_c_test_rtsp_cmds, ERMI_C_RTSP_MEM);

KEYWORD (ermi_c_rtsp_mem, ermi_c_rtsp_mem_eol, no_alt,
        "mem", "display mem utilization", PRIV_ROOT);

static keyword_options tcp_port_options[] = {
    {"854", "TCP_PORT_RTSP_VOD",    854},
    {"855", "TCP_PORT_RTSP_SBM",    855},
    {"856", "TCP_PORT_EDCP_EDA",    856},
    {"857", "TCP_PORT_EDCP_EEA",    857},
    {"554", "DEFAULT_RTSP_PORT",    DEFAULT_RTSP_PORT},
    { NULL, NULL, 0 }
};


EOLS (ermi_c_rtsp_port_eol, ermi_c_test_rtsp_cmds, ERMI_C_RTSP_PORT);
KEYWORD_OPTIONS (ermi_c_rtsp_port_opt, ermi_c_rtsp_port_eol, no_alt, 
                          tcp_port_options, OBJ(int, 1), 
                		 PRIV_ROOT, 0);
KEYWORD (ermi_c_rtsp_port, ermi_c_rtsp_port_opt, ermi_c_rtsp_mem,
        "port", "pick a TCP port", PRIV_ROOT);

EOLS (ermi_c_rtsp_no_eol, ermi_c_test_rtsp_cmds, 
                                ERMI_C_TEST_RTSP_NO);
KEYWORD (ermi_c_rtsp_no, ermi_c_rtsp_no_eol, ermi_c_rtsp_port,
        "no", "unset test_rtsp flag", PRIV_ROOT);

EOLS (ermi_c_rtsp_client_teardown_eol, ermi_c_test_rtsp_cmds, 
                                ERMI_C_RTSP_CLIENT_TEARDOWN);

NUMBER (ermi_c_rc_tearb_num, ermi_c_rtsp_client_teardown_eol,
        no_alt, OBJ(int, 2), 1, 100, "Number of Messages in burst");
KEYWORD (ermi_c_rc_tearb, ermi_c_rc_tearb_num, ermi_c_rtsp_client_teardown_eol,
	 "burst", "Send a burst of Messages", PRIV_ROOT);

NUMBER (ermi_c_rtsp_client_teardown_index, ermi_c_rc_tearb,
        no_alt, OBJ(int, 1), 0, 29, "session index");

KEYWORD (ermi_c_rtsp_client_teardown, ermi_c_rtsp_client_teardown_index, 
         ermi_c_rtsp_client_stop,
        "teardown", "send teardown request", PRIV_ROOT);

EOLS (ermi_c_rtsp_client_setparam_eol, ermi_c_test_rtsp_cmds, 
      ERMI_C_RTSP_CLIENT_SETPARAM);
NUMBER (ermi_c_rtsp_client_setparam_index, ermi_c_rtsp_client_setparam_eol,
        no_alt, OBJ(int, 1), 0, 29, "session index");
KEYWORD (ermi_c_rtsp_client_setparam, ermi_c_rtsp_client_setparam_index, 
                                ermi_c_rtsp_client_teardown,
        "set-parameter", "send set parameter request", PRIV_ROOT);

EOLS (ermi_c_rtsp_client_setup_eol, ermi_c_test_rtsp_cmds, 
                                ERMI_C_RTSP_CLIENT_SETUP);

NUMBER (ermi_c_rc_setb_num, ermi_c_rtsp_client_setup_eol,
        no_alt, OBJ(int, 2), 1, 100, "Number of Messages in burst");
KEYWORD (ermi_c_rc_setb, ermi_c_rc_setb_num, ermi_c_rtsp_client_setup_eol,
        "burst", "Send a burst of Messages", PRIV_ROOT);

NUMBER (ermi_c_rtsp_client_setup_index, ermi_c_rc_setb,
        no_alt, OBJ(int, 1), 0, 29, "session index");
KEYWORD (ermi_c_rtsp_client_setup, ermi_c_rtsp_client_setup_index, 
                                ermi_c_rtsp_client_setparam,
        "setup", "send setup request", PRIV_ROOT);

EOLS (ermi_c_rtsp_client_connect_eol, ermi_c_test_rtsp_cmds, 
                               ERMI_C_RTSP_CLIENT_CONNECT);
IPADDR (ermi_c_rtsp_server_ip, ermi_c_rtsp_client_connect_eol, no_alt, OBJ(paddr,1),
       "Enter RTSP server IP address");
KEYWORD (ermi_c_rtsp_client_connect, ermi_c_rtsp_server_ip, ermi_c_rtsp_client_setup,
        "connect", "connect", PRIV_ROOT);
KEYWORD (ermi_c_rtsp_client, ermi_c_rtsp_client_connect, ermi_c_rtsp_no,
        "client", "test client", PRIV_ROOT);


/***************************************
* test ermi_c rtsp server cmds
*/

EOLS (ermi_c_rtsp_server_stop_eol, ermi_c_test_rtsp_cmds, ERMI_C_RTSP_SERVER_STOP);
/*INTERFACE(ermi_c_rtsp_server_if, ermi_c_rtsp_server_eol, no_alt,
  OBJ(idb, 1), IFTYPE_LOOPBACK);*/
KEYWORD (ermi_c_rtsp_server_stop, ermi_c_rtsp_server_stop_eol, no_alt,
         "stop", "stop server", PRIV_ROOT);

EOLS (ermi_c_rtsp_server_setparam_eol, ermi_c_test_rtsp_cmds, 
      ERMI_C_RTSP_SERVER_SETPARAM);
NUMBER (ermi_c_rtsp_server_setparam_index, ermi_c_rtsp_server_setparam_eol,
        no_alt, OBJ(int, 1), 0, 29, "session index");
KEYWORD (ermi_c_rtsp_server_setparam, ermi_c_rtsp_server_setparam_index, 
         ermi_c_rtsp_server_stop,
        "set-parameter", "send set parameter request", PRIV_ROOT);

EOLS (ermi_c_rtsp_server_eol, ermi_c_test_rtsp_cmds, ERMI_C_RTSP_SERVER_INIT);
KEYWORD (ermi_c_rtsp_server_init, ermi_c_rtsp_server_eol, ermi_c_rtsp_server_setparam,
        "init", "init server", PRIV_ROOT);
KEYWORD (ermi_c_rtsp_server, ermi_c_rtsp_server_init, ermi_c_rtsp_client,
        "server", "test server", PRIV_ROOT);
KEYWORD (ermi_c_rtsp, ermi_c_rtsp_server, ALTERNATE,
        "rtsp", "rtsp protocol", PRIV_ROOT);
KEYWORD (ermi_c_test, ermi_c_rtsp, no_alt,
         "ermi-C", "video control plane test commands", PRIV_ROOT);

HELP(test_ermi_c_rtsp_top, ermi_c_test, "ermi_c test prompt:\n");

#undef ALTERNATE
#define ALTERNATE test_ermi_c_rtsp_top



