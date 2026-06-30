/*
 * Copyright (c) 2014-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *
 */

#ifndef __CLI_COMMON_H__
#define __CLI_COMMON_H__

#include "common.h"
#include "arpa/inet.h"

#define MAX_INPUT_STRING_SIZE   256
#define MAX_INPUT_ELEMS         24

#define GEN_CLI_FLAG_REDIRECTED                   0x0001

typedef union {
    int32 i32;
    uint32 u32;
    uint32 ip_addr[4];
} cli_data_t;


typedef struct cli_control_block_ {
    int type;                                  /* Type = 1 */
    int fooid;                                 /* Function id */
    int num_tokens;                            /* Number of tokens */
    cli_data_t data[MAX_INPUT_ELEMS];

    int offset[MAX_INPUT_ELEMS];               /* offset of tokens in
                                                  input_tokens */
    char *tokens[MAX_INPUT_ELEMS];             /* Pointer to tokens relative
                                                * to input_tokens as base
                                                */
    char input_tokens[MAX_INPUT_STRING_SIZE];  /* Input tokens separated by
                                                * NULLs
                                                */
    char cli_tty[MAX_INPUT_STRING_SIZE];       /* cli_tty file */
    FILE *fp;                                  // file pointer for tty
} cli_control_block;


/* For the peer CLI element processes */
#if CLI
int cli_peer_init(void);

static inline
void cli_peer_open_console (cli_control_block *cc)
{
    cc->fp = fopen(cc->cli_tty, "w");
}

static inline
void cli_peer_close_console (cli_control_block *cc)
{
    fclose(cc->fp);
}

int cli_printf(cli_control_block *cc, char *fmt, ...)
                __attribute__ ((format (printf, 2, 3)));

#else

#define cli_peer_init(...)              (1)
#define cli_peer_open_console(...)      (1)
#define cli_peer_close_console(...)     (1)
#define cli_printf(...)                 (1)

#endif

int cli_prepare_cc(cli_control_block *cc);

// CLI parser helper functions
int cli_parse_range_int(char* range, int* start, int* stop, int* inc);
int cli_parse_ip_addr(char* token, int* af, uint32 ip_addr[]);
int cli_parse_range_ip_addr(char* range, int* af, uint32 start_ip[],
                            uint32 stop_ip[], int* inc);
int cli_parse_list_int(char* list_string, uint16* list, int max_list_size);


#endif // __CLI_COMMON_H__
