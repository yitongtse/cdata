/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * Note: Set compilation flag CLI=0 will disable the use of CLI.
 *
 */

#ifndef __GENERIC_CLI_H__
#define __GENERIC_CLI_H__

#include "cli_common.h"

struct cli_elem_type_;
struct cli_elem_token_type_;

typedef int (*cli_function_type)(cli_control_block *cc);
typedef int (*cli_function_int_range)(int *min, int *max);
typedef int (*cli_function_uint_range)(unsigned int *min, unsigned int *max);

#define CLI_ELEM_FUNCTION       1
#define CLI_ELEM_IPC            2
#define CLI_ELEM_SOCK           3

typedef struct cli_elem_type_ {
    struct cli_elem_type_ * elem_peer;         /* Peer element */
    struct cli_elem_type_ * elem_child;        /* Child element */
    struct cli_elem_type_ * elem_parent;       /* Parent element 
                                                * Used at runtime only
                                                */
    struct cli_elem_type_ * elem_queue;        /* Queue element */

    int depth;                                 /* depth of the command */
    int elem_type;                             /* Type of element */
    unsigned int foo_type;                     /* function type */
    int fooid;                                 /* Function ID for the ccb */
    cli_function_type cli_function;            /* Function to call for this 
                                                * element 
                                                */
    const char * ipc_port;                     /* CLI IPC port */
    int  port_index;                           /* CLI IPC port information index*/
    unsigned int flags;                        /* Flags */
    char token[MAX_INPUT_ELEMS];               /* The token for this element */
    const char * help;                         /* Help string for this 
                                                * element
                                                */
    cli_function_int_range cli_int_range_function; /* Range function for integer input */
    int min;                                   /* Min for integer input */
    int max;                                   /* Max for integer input */
    cli_function_uint_range cli_uint_range_function; /* Range function for 
                                                      * unsigned integer input
                                                      */
    unsigned int umin;                     /* Min for unsigned integer input */
    unsigned int umax;                     /* Max for unsigned integer input */
} cli_elem_type;

typedef struct cli_elem_token_type_ {
    cli_elem_type *base_elem;
    cli_elem_type *last_match_elem;
    char *token;
    cli_data_t data;
    int num_match;
} cli_elem_token_type;

#define CLI_ELEM_TYPE_STRING     0x0001
#define CLI_ELEM_TYPE_NUMBER     0x0002
#define CLI_ELEM_TYPE_GEN_STRING 0x0004

int __cli_ipc_function(cli_control_block *cc);
int __cli_sock_function(cli_control_block *cc);

/*
 ***********************************************************************
 * Macros for defining the cmd array.
 */

/*
 * Start maco for the array
 */
#define CLI_ARR_START(name) \
    cli_elem_type name[] = { \

/*
 * End maco for the array
 */
#define CLI_ARR_END  \
    CLI_ARR_CMD(-1, "", CLI_FUN(NULL, 0), 0, "") \
    };

/*
 * Type macros for the tokens/numbers, etc
 * Currently support string, and number.
 */
#define CLI_ARR_CMD( depth, elem_str, function, flags, help) \
    {     \
    NULL, NULL, NULL, NULL, depth, CLI_ELEM_TYPE_STRING, function, \
    flags, #elem_str, help,  \
    NULL, 0, 0, NULL, 0, 0 \
    },

#define CLI_ARR_NUM( depth, min, max, function, flags, help) \
    {     \
    NULL, NULL, NULL, NULL, depth, CLI_ELEM_TYPE_NUMBER, function, \
    flags, "<number>", help,  \
    NULL, min, max, NULL, 0, 0 \
    },

#define CLI_ARR_STR( depth, function, flags, help) \
    {     \
    NULL, NULL, NULL, NULL, depth, CLI_ELEM_TYPE_GEN_STRING, function, \
    flags, "<string>", help,  \
    NULL, 0, 0, NULL, 0, 0 \
    },

/*
 * Function macros for the array
 */
#define CLI_NULL()              0, 0, NULL, NULL, 0
#define CLI_FUN(fun, fooid)     CLI_ELEM_FUNCTION, fooid, fun, NULL, 0
#define CLI_IPC(ipc, fooid)     CLI_ELEM_IPC, fooid, __cli_ipc_function, ipc, 0
#define CLI_SOCK(sock_id, fooid) CLI_ELEM_SOCK, fooid, __cli_sock_function, NULL, sock_id


/*
 ***********************************************************************
 */

#define PRINT_CCB_ELEM( name, elem) \
   printf("\r\n"#elem"\t=%lu [0x%08lx]", \
  (unsigned long)((##name)->(##elem)), (unsigned long)((##name)->(##elem)))

#define PRINT_CCB_ELEM_TOKEN( name, num, elem) \
   printf("\r\n"#elem"\t=%lu [0x%08lx]", \
  (unsigned long)((##name)->cli_elems[num]->(##elem)), \
  (unsigned long)((##name)->cli_elems[num]->(##elem)))

/* Functions internal to generic_cli */
int cli_print_cli_elem_type(cli_elem_type *elem);

/* Internal con_stty functions */
int con_stty_tc_setraw(void);
int con_stty_tc_restore(void);
char con_stty_getch(void);
int con_stty_print_stty_raw_info(void);

/* Initialization and required functions for functionality */
int cli_init(void);
int cli_add_elems(cli_elem_type *elem);
int cli_main_loop(void);

/* Helper functions */
int cli_set_prompt(const char *p);
void cli_print_version_banner(void);
int cli_print_cmds(void);
unsigned int cli_set_debug_level(unsigned int new_level);
int cli_print_ccb(cli_control_block *cc);
int cli_main_loop_end(void);
void cli_sock_send_ccb(cli_control_block *cc, int fd);


// CLI socket related defines
#define MAX_CLI_SOCKS    4
#define CLI_VIDEO_SOCK   0
#define CLI_SNIFFER_SOCK 1
extern int cli_sock_fd[MAX_CLI_SOCKS];
extern int cli_sock_port[MAX_CLI_SOCKS];
extern char cli_server[INET6_ADDRSTRLEN];

#endif // __GENERIC_CLI_H__
