/*
 *-------------------------------------------------------------------
 * ermi_c_rtsp_parser.h  RTSP Parser Video Control Plane
 *
 * January 2004, Vasmi Abidi: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by Cisco Systems, Inc.
 * All rights reserved.
 *-------------------------------------------------------------------
 */

#ifndef __ERMI_C_RTSP_PARSER_H__
#define __ERMI_C_RTSP_PARSER_H__

#define EOM_LEN 4

/* Defines names for some common characters */
#define SPACE 	' '
#define COLON	':'
#define TAB 	'\t'
#define CR	'\r'
#define LF	'\n'
#define NUL	'\0'

#define SKIP_LWS(p) while( (*(p) == SPACE) || (*(p) == TAB) ) { \
  ++(p); \
}


typedef struct {
    char *keyword;
    char *value;
} kvpair_t;


extern ermi_status ermi_c_rtsp_parse_message(char *rbuf, ushort len, rtsp_msg_t *smsg, 
					     uint *count);

ermi_status ermi_c_parser_rtsp_encode(rtsp_msg_t *rtsp_msg, char *buf, 
                                      uint32 buflen, uint32 *size);

#endif /* __ERMI_C_RTSP_PARSER_H__ */
