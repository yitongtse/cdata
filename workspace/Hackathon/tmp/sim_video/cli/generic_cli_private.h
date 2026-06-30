/*
 * Copyright (c) 2006-2014 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __GENERIC_CLI_PRIVATE_H__
#define __GENERIC_CLI_PRIVATE_H__

typedef struct generic_cli_control_block_ {
    /*
     * Currently there is no history stored.
     */
    struct generic_cli_control_block_ *prev;
    struct generic_cli_control_block_ *next;

    /*
     * These elements are for inputting 
     * text via the command line supporting <- and -> arrow keys
     */
    int input_str_length;
    int input_location;
    int esc_mode;
    char last_char;
    char input_string_shadow[MAX_INPUT_STRING_SIZE];
    char input_string[MAX_INPUT_STRING_SIZE];

    int num_raw_elems;
    int num_elems;
    char *tokens[MAX_INPUT_ELEMS];
    cli_elem_token_type *cli_elems[MAX_INPUT_ELEMS];

    /*
     * these are filled in right before calling the 
     * final function.
     */
    cli_data_t data[MAX_INPUT_ELEMS];

    char * var_string[MAX_INPUT_ELEMS];

    cli_control_block ccb;
} generic_cli_control_block;

typedef struct cli_ipc_information_type_ {
    char *port_name;
} cli_ipc_information_type;

typedef generic_cli_control_block gccb;


#endif // __GENERIC_CLI_PRIVATE_H__
