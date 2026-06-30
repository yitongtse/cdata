/*
* $Id: $
* $Source: $
*-------------------------------------------------------------------
* ermi_video_chain.c - Parse chains for EQAM VCP support
*
* Feb 2004, Xiaomei Liu; Jun 2008, Neeraj Khurana
*
* Copyright (c) 2004-2009 by cisco Systems, Inc.
* All rights reserved.
*------------------------------------------------------------------
*
*/

#include COMP_INC(ui, common_strings.h)
#include IOS_INC(h/interface_private.h)
#include <config.h>
#include COMP_INC(kernel/debug, debug.h)
#include <parser.h>
#include IOS_INC(parser/parser_actions.h)
#include IOS_INC(parser/macros.h)
#include IOS_INC(parser/parser_defs_parser.h)
#include COMP_INC(posix, string.h)
#include COMP_INC(kernel/memory, chunk.h)

#include "parser_defs_ermi_c.h"
#include "ermi_c_global.h"
#include "ermi_video_debug.h"
#include "ermi_c_master.h"
#include "ermi_c_rtsp_def.h"

/*
 * Parse chains for VCP debug commands
 */
#define	ALTERNATE NONE
#include "exec_debug_ermi_video.h"
LINK_POINT(ermi_c_debug_commands, ALTERNATE);
#undef	ALTERNATE

/***********************************************************************
 * Exec commands
 ***********************************************************************
 */
#define ALTERNATE NONE
LINK_POINT(exec_ermi_c_cmds, ALTERNATE);
#undef ALTERNATE

/***********************************************************************
 * Config commands
 ***********************************************************************
 */

/***********************************************************************
 * Show  command
 ***********************************************************************
 */
#define ALTERNATE NONE
#include "exec_show_ermi_c.h"
LINK_POINT(ermi_c_show_cmds, ALTERNATE);
#undef  ALTERNATE

/***********************************************************************
 * Test commands
 ***********************************************************************
 */
#define ALTERNATE NONE
#include "test_ermi_c_rtsp.h"
LINK_POINT(test_ermi_c_rtsp_cmds, ALTERNATE);
#undef  ALTERNATE

/***********************************************************************
 * Parser submodes
 ***********************************************************************
 */

/*
 * Parse chain registration array 
 */
static parser_extension_request ermi_video_chain_init_table[] = {
    { PARSE_ADD_DEBUG_CMD, &pname(ermi_c_debug_commands) },
    { PARSE_ADD_EXEC_CMD, &pname(exec_ermi_c_cmds) },
    { PARSE_ADD_SHOW_CMD, &pname(ermi_c_show_cmds) },
    { PARSE_ADD_TEST_CMD, &pname(test_ermi_c_rtsp_cmds) },
    { PARSE_LIST_END, NULL }
};

/*
 * ermi_video_parser_init - Initialize video parser support
 */
void ermi_video_parser_init (void)
{
    parser_add_command_list(ermi_video_chain_init_table, "ermi_video cp");
}
