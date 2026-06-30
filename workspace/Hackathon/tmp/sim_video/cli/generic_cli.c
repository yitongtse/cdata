/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>                /* timespec{} for pselect() */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>               /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>    /* for S_xxx file mode constants */
#include <sys/uio.h>             /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>              /* for Unix domain sockets */
#include <sys/select.h>  /* for convenience */
#include <termios.h>
#include <strings.h>
#include <stdarg.h>
//#include "ipc.h"
#include "common.h"
#include "generic_cli.h"
#include "generic_cli_private.h"


cli_elem_type *root_elem;
int end_cli;

gccb *cycle_ccb;
gccb *cur_ccb;
char const *prompt = "prompt> ";
int cycle_flag = 0;
int neg_flag = 0;
extern int gen_peer_flag;
cli_elem_type *cur_elem;

int cli_sock_fd[MAX_CLI_SOCKS] = {-1, -1, -1, -1};
int cli_sock_port[MAX_CLI_SOCKS] = {-1, -1, -1, -1};
char cli_server[INET6_ADDRSTRLEN];

#define DEBUG_BLANKET(x) \
   printf("DEBUG -- Fun %25s %15s %03d 0x%x\r\n", \
       __FUNCTION__, __FILE__, __LINE__, \
       (uintptr_t) x); \
   fflush(stdout); usleep(10);

int debug_level = 0;
#define DEBUG_LEVEL1 (debug_level & 1)
#define DEBUG_LEVEL2 (debug_level & 2)
#define DEBUG_LEVEL3 (debug_level & 4)

static void cli_print_tokens(const char *function);
static void cli_print_ccb_internal(const char *function);
static int cli_token_match(cli_elem_type *cli_elem, char *token);
static int cli_token_cmp(cli_elem_token_type *cli_data);
static int cli_print_prompt(void);
static int cli_print_current(void);
static int cli_tokenize(void);
static int cli_help_print_all(cli_elem_type *cli_elem);
static int cli_help(void);
static int cli_tab(void);
static int cli_backspace(void);
static int cli_retkey(void);
static int cli_handle_normal_char(char c);
static int cli_space(void);
static int cli_up_arrow(void);
static int cli_down_arrow(void);
static int cli_right_arrow(void);
static int cli_left_arrow(void);
static int cli_char(char c);
static int cli_insert_elem(cli_elem_type **ptr, cli_elem_type *elem);

typedef struct gen_cli_cmds_ {
    cli_elem_type *ce;
    cli_elem_type *parent;
    int depth;
} gen_cli_cmds;

static gccb *cli_get_new_ccb (void)
{
    gccb * ccb;
    int i;

    ccb = (gccb *) malloc(sizeof(gccb));
    if (!ccb) {
        DEBUG_BLANKET(ccb);
        exit(0);
    }

    bzero(ccb, sizeof(gccb));
    for (i = 0; i < MAX_INPUT_ELEMS; i++) {
        ccb->cli_elems[i] = (cli_elem_token_type *) 
            malloc(sizeof(cli_elem_token_type));
        if (!ccb->cli_elems[i]) {
            DEBUG_BLANKET(ccb->cli_elems[i]);
            exit(0);
        }
    }
    return(ccb);
}

static gccb *cli_clear_ccb (gccb * ccb)
{
    int i;

    ccb->input_str_length = 0;
    ccb->input_location = 0;
    ccb->esc_mode = 0;
    ccb->num_elems = 0;
    bzero(ccb->input_string, MAX_INPUT_STRING_SIZE);
    bzero(ccb->input_string_shadow, MAX_INPUT_STRING_SIZE);

    for (i = 0; i < MAX_INPUT_ELEMS; i++) {
        ccb->cli_elems[i]->base_elem = NULL;
        ccb->cli_elems[i]->last_match_elem = NULL;
        ccb->cli_elems[i]->data.ip_addr[0] = 0;
        ccb->cli_elems[i]->data.ip_addr[1] = 0;
        ccb->cli_elems[i]->data.ip_addr[2] = 0;
        ccb->cli_elems[i]->data.ip_addr[3] = 0;
        ccb->cli_elems[i]->num_match = 0;
        ccb->data[i].ip_addr[0] = 0;
        ccb->data[i].ip_addr[1] = 0;
        ccb->data[i].ip_addr[2] = 0;
        ccb->data[i].ip_addr[3] = 0;
        ccb->var_string[i] = NULL;
    }
    return(ccb);
}

static gccb *cli_copy_ccb (gccb * src, gccb * dst)
{
    int i;

    dst->input_str_length = src->input_str_length;
    dst->input_location = src->input_location;
    bcopy(src->input_string, dst->input_string, MAX_INPUT_STRING_SIZE);
    dst->last_char = src->last_char;

    dst->esc_mode = 0;
    dst->num_elems = 0;
    for (i = 0; i < MAX_INPUT_ELEMS; i++) {
        dst->cli_elems[i]->base_elem = NULL;
        dst->cli_elems[i]->last_match_elem = NULL;
        dst->cli_elems[i]->data.ip_addr[0] = 0;
        dst->cli_elems[i]->data.ip_addr[1] = 0;
        dst->cli_elems[i]->data.ip_addr[2] = 0;
        dst->cli_elems[i]->data.ip_addr[3] = 0;
        dst->cli_elems[i]->num_match = 0;
        dst->data[i].ip_addr[0] = 0;
        dst->data[i].ip_addr[1] = 0;
        dst->data[i].ip_addr[2] = 0;
        dst->data[i].ip_addr[3] = 0;
        dst->var_string[i] = NULL;
    }
    bzero(dst->input_string_shadow, MAX_INPUT_STRING_SIZE);
    bzero(&dst->ccb, sizeof(cli_control_block));
    return(dst);
}

static void cli_print_cmds_rec (gen_cli_cmds gcc)
{
    gen_cli_cmds gcc2;
    int i;

    if (gcc.ce == NULL) {
        return;
    }
    for (i = 0; i < gcc.depth; i++) {
        printf("  ");
    }
    switch(gcc.ce->elem_type) {
    case CLI_ELEM_TYPE_NUMBER:
        printf("%-5s %-4d %-4d", gcc.ce->token, gcc.ce->min, gcc.ce->max);
        break;
    case CLI_ELEM_TYPE_STRING:
        printf("%-15s", gcc.ce->token);
        break;
    case CLI_ELEM_TYPE_GEN_STRING:
        printf("%-15s", "gen_string");
        break;
    default:
        printf("%-15s", "ERROR");
        break;
    }
    if (gcc.ce->cli_function) {
        printf("  <cr>  ");
    }
    printf("\n");

    if (gcc.ce->elem_child) {
        gcc2.ce = gcc.ce->elem_child;
        gcc2.depth = gcc.depth + 1;
        cli_print_cmds_rec(gcc2);
    }
    if (gcc.ce->elem_peer) {
        gcc2.ce = gcc.ce->elem_peer;
        gcc2.depth = gcc.depth;
        cli_print_cmds_rec(gcc2);
    }
}

static void cli_print_tokens (const char *function)
{
    int i;

    if (!DEBUG_LEVEL1) {
        return;
    }
    printf("\r\n");
    printf("function %s\r\n input_string %s\r\n", function, cur_ccb->input_string);
    for (i = 0; i < MAX_INPUT_ELEMS; i++) {
        if (cur_ccb->tokens[i]) {
            printf("%i  tok (0x%x) %20s\r\n", i, 
                        (uintptr_t) cur_ccb->tokens[i],
                        cur_ccb->tokens[i]);
        } else {
            printf("%i  tok (0x%x) %20s\r\n", i, 
                        (uintptr_t) cur_ccb->tokens[i],
                        "(null)");
        }
    }
}

static void cli_print_ccb_internal (const char *function)
{
    int cur_debug;
    if (!DEBUG_LEVEL2) {
        return;
    }

    printf("\r\n---------------------------- cur_ccb ---------------------\r\n");
    cur_debug = cli_set_debug_level(debug_level | 1);
    cli_print_tokens((const char *)__FUNCTION__);
    cli_set_debug_level(cur_debug);
    printf("function            %s\r\n", function);
    printf("num_raw_elems       %d\r\n", cur_ccb->num_raw_elems);
    printf("num_elems           %d\r\n", cur_ccb->num_elems);
    printf("input_str_length    %d\r\n", cur_ccb->input_str_length);
    printf("input_location      %d\r\n", cur_ccb->input_location);
    printf("esc_mode            %d\r\n", cur_ccb->esc_mode);
    printf("---------------------------- end cur_ccb -----------------\r\n");
}


// Returns the match level: higher match level has precedence over lower
//   0: no match
//   1: match generic string
//   2: match a number
//   3: match a command
//
static int cli_token_match (cli_elem_type *cli_elem, char *token)
{
    int match = 0;
    int num;

    switch (cli_elem->elem_type) {

    case CLI_ELEM_TYPE_STRING: 
        {
            int token_len = strlen(token);
            char *elem_str;

            elem_str = cli_elem->token;
            if (DEBUG_LEVEL3) {
                printf("  %d: CLI_ELEM_TYPE_STRING %s",
                       cli_elem->depth, elem_str);
            }
            if (strncmp(token, elem_str, token_len) == 0) {
                match = 3;
            }
        }
        break;

    case CLI_ELEM_TYPE_GEN_STRING:
        {
            char *elem_str;

            elem_str = cli_elem->token;
            if (DEBUG_LEVEL3) {
                printf("  %d: CLI_ELEM_TYPE_GEN_STRING %s",
                       cli_elem->depth, elem_str);
            }
            match = 1;
        }
        break;

    case CLI_ELEM_TYPE_NUMBER:
    {
        char *endptr;
        num = strtol(token, &endptr, 0);
            // Note: endptr == token means token's format is not recognized

        if (DEBUG_LEVEL3) {
            printf("  %d: CLI_ELEM_TYPE_NUMBER %d (min %d, max %d)",
                   cli_elem->depth, num, cli_elem->min, cli_elem->max);
        }
        if (endptr == token || *endptr != 0) {
            // Not matched
            break;
        }
        if (neg_flag || ((num >= cli_elem->min) && (num <= cli_elem->max))) {
            match = 2;
        }
        break;
    }

    default:
        printf("Error... %d %s\r\n", cli_elem->elem_type, token);
    }

    if (DEBUG_LEVEL3) {
        printf(": %d\r\n", match);
    }
    return(match);
}

static int cli_token_cmp (cli_elem_token_type *cli_data)
{
    int match_level = 0;
    int match_count = 0;
    cli_elem_type *last_match_elem = NULL;
    cli_elem_type *cli_elem = cli_data->base_elem;

    if (DEBUG_LEVEL3) {
        printf("Matching %s...\r\n", cli_data->token);
    }

    while(cli_elem != NULL) {
        int level = cli_token_match(cli_elem, cli_data->token);
        if (level == 0)  goto next;
        if (level > match_level) {
            match_level = level;
            match_count = 1;
            last_match_elem = cli_elem;
        } else if (level == match_level) {
            match_count++;
            last_match_elem = cli_elem;
        }
next:
        cli_elem = cli_elem->elem_peer;
    }
    cli_data->last_match_elem = last_match_elem;
    cli_data->num_match = match_count;
    return(match_count);
}

static int cli_print_prompt (void)
{
    printf("%s", prompt);
    return(0);
}

static int cli_print_current (void)
{
    cli_print_prompt();
    printf("%s", cur_ccb->input_string);
    return(0);
}

static int cli_print_cycle (void)
{
    printf("\r                                                    \r");
    cli_print_prompt();
    printf("%s", cycle_ccb->input_string);
    return(0);
}

static void cli_cycle_check (void)
{
    if (cycle_ccb != cur_ccb) {
        cli_copy_ccb(cycle_ccb, cur_ccb);
    }
}

static int cli_tokenize (void)
{
    int num, i;

    cycle_ccb = cur_ccb;
    // if was in a curser position not at the end place at end now!
    // Help is alway for last element...
    cur_ccb->input_location = cur_ccb->input_str_length;

    if (DEBUG_LEVEL3) {
        printf("\r\nTokenizing information ...\r\n");
    }
  
    if (strlen(cur_ccb->input_string) == 0) {
        return(0);
    }

    strncpy(cur_ccb->input_string_shadow, cur_ccb->input_string, 
                    MAX_INPUT_STRING_SIZE);
    i = 0;
    cur_ccb->tokens[0] = strtok(cur_ccb->input_string_shadow, " ");
    if (!cur_ccb->tokens[0]) {
        return (0);
    }

    while ((cur_ccb->tokens[i]) && (i < MAX_INPUT_ELEMS-1)) {
        cur_ccb->tokens[++i] = strtok(NULL, " ");
    }

    cur_ccb->num_elems = i;
    cur_ccb->cli_elems[0]->base_elem = root_elem->elem_child;
    cur_ccb->cli_elems[0]->last_match_elem = NULL;
    cur_ccb->cli_elems[0]->token = cur_ccb->tokens[0];
    cur_ccb->cli_elems[0]->num_match = 0;
    cur_ccb->cli_elems[0]->data.ip_addr[0] = 0;
    cur_ccb->cli_elems[0]->data.ip_addr[1] = 0;
    cur_ccb->cli_elems[0]->data.ip_addr[2] = 0;
    cur_ccb->cli_elems[0]->data.ip_addr[3] = 0;
    num = cli_token_cmp(cur_ccb->cli_elems[0]);

    // If not a unique match on every element previous to last
    // token, it is Ambiguous!
    i = 1;
    while ((num == 1) && (i < cur_ccb->num_elems)) {
        cur_ccb->cli_elems[i]->base_elem =  
            cur_ccb->cli_elems[i - 1]->last_match_elem->elem_child;
        if (cur_ccb->cli_elems[i]->base_elem == NULL) {
            break;
        }
        cur_ccb->cli_elems[i]->last_match_elem = NULL;
        cur_ccb->cli_elems[i]->token = cur_ccb->tokens[i];
        cur_ccb->cli_elems[i]->num_match = 0;
        cur_ccb->cli_elems[i]->data.ip_addr[0] = 0;
        cur_ccb->cli_elems[i]->data.ip_addr[1] = 0;
        cur_ccb->cli_elems[i]->data.ip_addr[2] = 0;
        cur_ccb->cli_elems[i]->data.ip_addr[3] = 0;
        num = cli_token_cmp(cur_ccb->cli_elems[i]);
        i++;
    }
    cur_ccb->num_elems = i;
    return(i);
}


static int cli_help_print_all (cli_elem_type *cli_elem)
{
    printf("\r\n");
    while (cli_elem != NULL) {
        if (cli_elem->elem_type == CLI_ELEM_TYPE_NUMBER) {
            printf("%-15s %s (%d - %d)\r\n", cli_elem->token, cli_elem->help,
                   cli_elem->min, cli_elem->max);
        } else {
            printf("%-15s %s\r\n", cli_elem->token, cli_elem->help);
        }
        cli_elem = cli_elem->elem_peer;
    }
    return(0);
}

static int cli_help (void)
{
    int num_elems;
    cli_elem_type *cli_elem = NULL;
    char *tok;

    num_elems = cli_tokenize();
    cli_print_tokens((const char *)__FUNCTION__);

    if (num_elems < 0) {
        // error reprint prompt and let user fix problem
        printf("\r\n");
        cli_print_current();
    }
    cli_print_ccb_internal((const char *)__FUNCTION__);
    
    if (num_elems > 0) {
        int idx = cur_ccb->num_elems - 1;

        tok = cur_ccb->tokens[idx];
        cli_elem = cur_ccb->cli_elems[idx]->last_match_elem;
        printf("\r\n");
        if ((cur_ccb->last_char == ' ') && (cur_ccb->cli_elems[idx]->num_match == 1)) {
            if (cli_elem->cli_function) {
                printf("\n<cr>");
            }
            cli_help_print_all(cli_elem->elem_child);
#if 0
        } else if (cur_ccb->last_char == ' ') { 
            printf("Ambiguous command\r\n");
#endif
        } else {
            cli_elem = cur_ccb->cli_elems[idx]->base_elem;
            while (cli_elem != NULL) {
                if (cli_token_match(cli_elem, tok)) {
                    printf("%-15s %s\r\n", cli_elem->token, cli_elem->help);
                }
                cli_elem = cli_elem->elem_peer;
            }
        }
    } else {
        // Print all commands at root level
        printf("\r\n");
        cli_help_print_all(root_elem->elem_child);
    }
    printf("\r\n");
    cli_print_current();
    return(0);
}

/*
 * This function will automatically fill in the cmd if it is a unique match.
 * In order to verify that it is functioning correctly the following conditions
 * must be met:
 * a) No match -- Print message, then prompt.
 * b) Match more then 1 -- Print message, then prompt.
 * c) Exact match -- Complete the command.
 *
 */
static int cli_tab (void)
{
    int num_elems;
    char *token;
    int token_len;
    int cmd_len;
    int i;

    num_elems = cli_tokenize();
    cli_print_tokens((const char *)__FUNCTION__);
    cli_print_ccb_internal((const char *)__FUNCTION__);

    if (!num_elems) {
        return (0);
    }

    token = NULL;
    token_len = 0;
    cmd_len = 0;

    if (cur_ccb->cli_elems[cur_ccb->num_elems - 1]->last_match_elem) {
        token = 
           cur_ccb->cli_elems[cur_ccb->num_elems - 1]->last_match_elem->token;
    }
    
    if ((cur_ccb->num_elems > 0) && 
        (cur_ccb->cli_elems[cur_ccb->num_elems - 1]->num_match == 1)) {

        cmd_len = strlen(token);
        token_len = strlen(cur_ccb->cli_elems[cur_ccb->num_elems - 1]->token);
//        printf("\r\n");
//        cli_print_current();
        if (cur_ccb->cli_elems[cur_ccb->num_elems - 1]->
            last_match_elem->elem_type == CLI_ELEM_TYPE_STRING) {
            for(i = token_len; i < cmd_len; i++) {
                cli_handle_normal_char(token[i]);
            }
            cli_handle_normal_char(' ');
        }
    }
    return(0);
}

static int cli_backspace (void)
{
    char local_c;
    int size;

    if (cur_ccb->input_location == 0) {
        return(0);
    }
    size = cur_ccb->input_str_length - cur_ccb->input_location + 1;

    bcopy(&cur_ccb->input_string[cur_ccb->input_str_length], 
        &cur_ccb->input_string[cur_ccb->input_str_length - 1], 
        size);
    local_c = cur_ccb->input_string[cur_ccb->input_str_length];
    cur_ccb->input_str_length--;
    cur_ccb->input_location--;

    if (cur_ccb->input_location > 0) {
        cur_ccb->last_char = cur_ccb->input_string[cur_ccb->input_location - 1];
    } else {
        cur_ccb->last_char = 0;
    }
    if (local_c == 0) {
        local_c = ' ';
    }
    printf("%c", 8); // Back space
    printf("%c", local_c);
    printf("%c", 8); // Back space
    return(0);
}

static int cli_clear_line (void)
{
    while (cur_ccb->input_location) {
        cli_backspace();
    }
    return(0);
}

static int cli_setup_ccb (void)
{
    int i;
    char *tty_nam;

    bzero(&(cur_ccb->ccb), sizeof(cli_control_block));
    if ((tty_nam = ttyname(STDOUT_FILENO)) == NULL) {
        perror(NULL);
        printf("%s Error initializing tty name\r\n", __FUNCTION__);
        return(-1);
    } 

    bcopy(tty_nam, cur_ccb->ccb.cli_tty, strlen(tty_nam) + 1);
    bcopy(cur_ccb->input_string_shadow, cur_ccb->ccb.input_tokens, 
                                MAX_INPUT_STRING_SIZE);

    cur_ccb->ccb.type = 0;
    cur_ccb->ccb.num_tokens = cur_ccb->num_elems;

    for (i=0; i<cur_ccb->num_elems; i++) {
        uintptr_t offset = cur_ccb->tokens[i] - cur_ccb->input_string_shadow;
        cur_ccb->ccb.tokens[i] = (char*)offset;

        switch (cur_ccb->cli_elems[i]->last_match_elem->elem_type) {

        case CLI_ELEM_TYPE_NUMBER:
            cur_ccb->ccb.data[i].i32 = (int)strtol(cur_ccb->tokens[i],
                                                   (char**)NULL, 0);
            break;

        default:
            break;
        }
    }

    if (DEBUG_LEVEL1) {
        cli_print_ccb(&(cur_ccb->ccb));
    }
    return(0);
}


static int cli_sock_open (int* sock_fd, int sock_port)
{
    int rc;
    struct sockaddr_in serv_addr;

    if (sock_port == -1) {
      printf("Error: No port to CLI server\n");
      return -1;
    }

    *sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_fd == -1) {
        printf("Error : Socket open\n");
        return -1;
    }
    
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(sock_port);
    
    rc = inet_pton(AF_INET, cli_server, &serv_addr.sin_addr);
    if (rc != 1) {
        printf("CLI: Failed to convert IP address");
        close(*sock_fd);
        return EINVAL;
    }
    
    rc = connect(*sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc == -1) {
        printf("CLI: Failed to open socket: %s\n", strerror(errno));
        close(*sock_fd);
        return ENOTCONN;
    }

    return EOK;
}


// Forwad ccb to server, wait for reply before printing prompt
//
void cli_sock_send_ccb (cli_control_block *cc, int fd)
{
    write(fd, cc, sizeof(*cc));
    int n = read(fd, cc, sizeof(*cc));     // prompt control
    if (n == 0) {
        printf("Error: Peer connection down\n");
    }
}


int __cli_sock_function (cli_control_block *cc)
{
    int sock_id = cur_elem->port_index;     // reused for socket index
    int sock_fd = cli_sock_fd[sock_id];

    if (sock_fd == -1) {
        // Lazy open
        int rc = cli_sock_open(&sock_fd, cli_sock_port[sock_id]);
        if (rc != EOK) {
            printf("Error: Command not executed.\n"); 
            return 0;
        }

        cli_sock_fd[sock_id] = sock_fd;
    }

    cli_sock_send_ccb(cc, sock_fd);
    return 0;
}


static int cli_retkey (void)
{
    int num_elems;
    cli_elem_type *elem;
    int idx;

    num_elems = cli_tokenize();
    cli_print_tokens((char const *)__FUNCTION__);

    printf("\n");
    if (num_elems > 0) {
        idx = num_elems - 1;
        if (cur_ccb->cli_elems[idx]->num_match == 1) {
            if (DEBUG_LEVEL3) {
                printf("Match!!!!\n");
            }
            elem = cur_ccb->cli_elems[idx]->last_match_elem;
            cur_elem = elem;
            if (elem->foo_type) {
                if (DEBUG_LEVEL3) {
                    printf("Calling function...!!!\n");
                }
                if (cli_setup_ccb()) {
                    return(-1);
                }
                cur_ccb->ccb.fooid = elem->fooid;
                cli_print_ccb_internal((const char *)__FUNCTION__);
                if (DEBUG_LEVEL1) {
                    cli_print_cli_elem_type(elem);
                }
                if (elem->cli_function) {
                    elem->cli_function(&(cur_ccb->ccb));
                } else {
                    printf("Not IPC, or function...\n");
                }
            } else {
                printf("Incomplete command...\n");
            }
        } else if (cur_ccb->cli_elems[idx]->num_match == 0) {
            printf("Unknown command\n");
        } else {
            printf("Ambiguous command\n");
        }

        cur_ccb = cur_ccb->next;
        cycle_ccb = cur_ccb;
        cli_clear_ccb(cur_ccb);
    }
    cli_print_prompt();
    return(0);
}

static int cli_handle_normal_char (char c)
{
    if (cur_ccb->input_location == cur_ccb->input_str_length) {
        cur_ccb->input_string[cur_ccb->input_str_length] = c;
        cur_ccb->input_string[cur_ccb->input_str_length + 1] = 0;
        cur_ccb->input_location++;
        cur_ccb->input_str_length++;
        cur_ccb->last_char = c;
        printf("%c", c);
    } else { // (input_location < input_str_length)
        int i, len, offset;
        len = cur_ccb->input_str_length - cur_ccb->input_location + 1;
        offset = cur_ccb->input_location;

        for(i = len; i >= 0; i--) {
            cur_ccb->input_string[offset + i + 1] = 
                cur_ccb->input_string[offset + i];
        }
        cur_ccb->input_string[cur_ccb->input_location] = c;
        cur_ccb->input_location++;
        cur_ccb->input_str_length++;
        cli_print_current();
        for(i = 0; i < len - 1; i++) {
            printf("%c", 8);
        }
    }
    return(0);
}

static int cli_space (void)
{
    cli_handle_normal_char(' ');
    return(0);
}

static int cli_up_arrow (void)
{
    cycle_ccb = cycle_ccb->prev;
    cur_ccb->esc_mode = 0;
    return(0);
}

static int cli_down_arrow (void)
{
    cycle_ccb = cycle_ccb->next;
    cur_ccb->esc_mode = 0;
    return(0);
}

static int cli_right_arrow (void)
{
    cur_ccb->esc_mode = 0;
    printf("->\r\n");
    cli_print_current();
    /**
    if (cur_ccb->input_location < cur_ccb->input_str_length) {
        printf("%c", 
            cur_ccb->input_string[cur_ccb->input_location]);
        cur_ccb->input_location++;
    }
    **/
    return(0);
}


static int cli_left_arrow (void)
{
    cur_ccb->esc_mode = 0;
    printf("<-\r\n");
    cli_print_current();
    /**
    if (cur_ccb->input_location > 0) {
        cur_ccb->input_location--;
        printf("%c", 8);
    }
    **/
    return(0);
}


static int cli_esc_char_handle (char c)
{
    int esc_char = 0;
    switch (c) {
    case 27:   cur_ccb->esc_mode = 1;
               esc_char = 1;
               break;

    case '[':  // Extra char in ESC mode, don't know why...
               if (cur_ccb->esc_mode) {
                   esc_char = 1;
                   break;
               }
    case 'A':  if (cur_ccb->esc_mode) {
                   esc_char = 1;
                   cycle_flag = 1;
                   cli_up_arrow();
                   break;
               }
    case 'B':  if (cur_ccb->esc_mode) {
                   esc_char = 1;
                   cycle_flag = 1;
                   cli_down_arrow();
                   break;
               }
    case 'C':  if (cur_ccb->esc_mode) {
                   esc_char = 1;
                   cli_cycle_check();
                   cli_right_arrow();
                   break;
               }
    case 'D':  if (cur_ccb->esc_mode) {
                   esc_char = 1;
                   cli_cycle_check();
                   cli_left_arrow();
                   break;
               }
    default:
               break;
    }
    if (cycle_flag & esc_char) {
        cli_print_cycle();
        return(1);
    }
    if (esc_char) {
        return(1);
    }
    if (cycle_flag) {
        cycle_flag = 0;
        cli_cycle_check();
    }
    return(0);
}

static int cli_char (char c)
{
    if (cli_esc_char_handle(c)) {
        return(0);
    }
    switch(c) {
    case '?':  cli_help();
               break;
    case 3: // ctrl-c
               printf("\r\nctrl-c\r\n");
               cli_main_loop_end();
               break;
    case 4: // ctrl-d
               cli_print_cmds();
               cli_print_current();
               break;
    case 21: // ctrl-u
               cli_clear_line();
               break;
    case 127:
    case 8:    cli_backspace();
               break;
    case '\r': cli_retkey();
               break;
    case '\t': cli_tab();
               break;
    case ' ':  cli_space();
               break;
    default:   
               cli_handle_normal_char(c);
    };
    return(0);
}

static int cli_insert_elem (cli_elem_type **ptr, cli_elem_type *elem)
{
    cli_elem_type *tmp;

    while(*ptr != NULL) {
        if (strncmp((*ptr)->token, elem->token, 20) > 0) {
            break;
        }
        ptr = &((*ptr)->elem_peer);
    }

    tmp = *ptr;
    *ptr = elem;
    elem->elem_peer = tmp;
    return(0);
}

/****
 **** Public functions after this point!
 ****/

int cli_set_prompt (const char *p)
{
    prompt = p;
    return(0);
}

int cli_print_cli_elem_type(cli_elem_type *elem)
{
    printf("elem = 0x%8x\r\n", (uintptr_t) elem);
    return(0);
}

int cli_add_elems (cli_elem_type *elem)
{
    cli_elem_type **head;
    cli_elem_type *parent;
    cli_elem_type *last_elem;
    cli_elem_type *new_elem;
    int prev_depth;
    int depth;

    last_elem = 0;
    parent = root_elem;
    head = &(root_elem->elem_child);
    prev_depth = elem->depth;
    while (elem->depth != -1) {
        root_elem->fooid++;
        new_elem = malloc(sizeof(cli_elem_type));
        if (!new_elem) {
            DEBUG_BLANKET(new_elem);
            exit(0);
        }
        bcopy(elem, new_elem, sizeof(cli_elem_type));
        depth = new_elem->depth;
        if (prev_depth == depth) {
            // Nothing to do if they are the same!
        } else if (prev_depth < depth) {
            if (depth != (prev_depth + 1)) {
                printf("Error with data\n");
            }
            parent = last_elem;
            head = &(parent->elem_child);
        } else if (prev_depth > depth) {
            parent = last_elem->elem_parent;
            while ((prev_depth != depth) && (parent->elem_parent != NULL)) {
                parent = parent->elem_parent; 
                prev_depth--;
            }
            head = &(parent->elem_child);
        }
        new_elem->elem_parent = parent;
        cli_insert_elem(head, new_elem);
        prev_depth = depth;
        last_elem = new_elem;
        elem++;
    }

    fflush(stdout);
    return(0);
}

int cli_print_cmds (void)
{
    gen_cli_cmds gcc;

    gcc.ce = root_elem->elem_child;
    gcc.depth = 0;

    printf("Num commands = %d\r\n", root_elem->fooid);
    printf("\r\n**** **** **** **** **** **** **** ****\r\n");
    if (root_elem->elem_child) {
        cli_print_cmds_rec(gcc);
    }
    printf("**** **** **** **** **** **** **** ****\r\n");
    return(0);
}

int cli_init (void)
{
    int i;
    gccb *my_ccb;
    gccb *my_ccb2;

    cur_elem = NULL;
    cur_ccb = cli_get_new_ccb();
    root_elem = malloc(sizeof(cli_elem_type));
    if (!root_elem) {
        printf("Unable to alloc memory\r\n");
        DEBUG_BLANKET(root_elem);
        exit(0);
    }
    bzero(root_elem, sizeof(cli_elem_type));
    strncpy(root_elem->token, "root_elem", 20);
    my_ccb = cur_ccb;
    for(i = 0; i < MAX_INPUT_ELEMS; i++) {
        cli_clear_ccb(my_ccb);
        my_ccb2 = cli_get_new_ccb();
        my_ccb->next = my_ccb2;
        my_ccb2->prev = my_ccb;
        my_ccb = my_ccb2;
    }
    cur_ccb->prev = my_ccb;
    my_ccb->next = cur_ccb;
    cycle_ccb = cur_ccb;
    return(0);
}
int cli_main_loop_end (void)
{
    end_cli = 0;
    return(0);
}

int cli_main_loop (void)
{
    char c = 0;
    int retval;
    

    retval = con_stty_tc_setraw();
    if (retval != 0) {
        printf("Error %s line %d code %d\r\n", __FUNCTION__, __LINE__, retval);
    }
    end_cli = 1;
    cli_print_version_banner();
    cli_print_prompt();
    fflush(stdout);

    while (end_cli) {
        c = con_stty_getch();
        retval = cli_char(c);
        if (retval != 0) {
            printf("Error %s line %d code %d\r\n", __FILE__, __LINE__, retval);
            fflush(stdout);
        }
        retval = fflush(stdout);
        if (retval != 0) {
            printf("Error %s line %d code %d\r\n", __FILE__, __LINE__, retval);
            fflush(stdout);
        }
    }
    retval = con_stty_tc_restore();
    if (retval != 0) {
         printf("Error %s line %d code %d\r\n", __FILE__, __LINE__, retval);
               fflush(stdout);
    }
    return(0);
}

void cli_print_version_banner (void)
{
printf("\r\n"
"        **********************************************************\r\n"
"        *                                                        *\r\n"
"        *                      |       |                         *\r\n"
"        *                    | | |   | | |                       *\r\n"
"        *                  | | | | | | | | |                     *\r\n"
"        *                    C  I  S  C  O                       *\r\n"
"        *                                                        *\r\n"
"        * Welcome to the Cylone VDMAN Command Line Parser        *\r\n"
"        * Copyright (c) 2013-2015 by Cisco Systems, Inc.         *\r\n"
"        * All rights reserved.                                   *\r\n"
"        *                                                        *\r\n"
"        * Version 1 build 1                                      *\r\n"
"        **********************************************************\r\n"
"\r\n");
}

unsigned int cli_set_debug_level (unsigned int new_level)
{
    unsigned int old_level = debug_level;
//    printf("%s old %d new %d\r\n", __FUNCTION__, debug_level, new_level);
    debug_level = new_level;
    return(old_level);
}

int cli_print_ccb (cli_control_block *cc)
{
    int i;
    printf("\r\n---------------------------- cli_control_block ---------------------\r\n");
    printf("tty                  %s\r\n", cc->cli_tty);
    printf("fooid                %d\r\n", cc->fooid);
    printf("num_tokens           %d\r\n", cc->num_tokens);
    printf("input_tokens  ptr  0x%x\r\n", (uintptr_t) cc->input_tokens);
    printf("    [%s]\r\n", cc->input_tokens);
    for (i = 0; i < MAX_INPUT_ELEMS; i++) {
        printf("tokens[%d]  ptr  0x%x\r\n", i, (uintptr_t) cc->tokens[i]);
        if ((uintptr_t) cc->tokens[i] > MAX_INPUT_STRING_SIZE) {
            printf("    [%s]\r\n", cc->tokens[i]);
        } else {
            printf("tokens[%d]            %s\r\n", i, "(null)");
        }
    }
    printf("\r\n---------------------------- cli_control_block ---------------------\r\n");
    return(0);
}

