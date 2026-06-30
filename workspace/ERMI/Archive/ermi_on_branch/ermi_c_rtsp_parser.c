/*
 *-------------------------------------------------------------------
 * ermi_c_rtsp_parser.c  RTSP Parser ERMI-Video Control Plane
 *
 * January 2004, Vasmi Abidi: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by Cisco Systems, Inc.
 * All rights reserved.
 *-------------------------------------------------------------------
 */
#include COMP_INC(network, address.h)
#include <assert.h>
#include <logger.h>
#include COMP_INC(cisco, ciscolib.h)
#include <ctype.h>
#include COMP_INC(kernel, clock.h)
#include "name.h"
#include COMP_INC(ip, ip.h)

#include "ermi_c_global.h"
#include "ermi_c_rtsp_def.h"
#include "msg_ermi_c_vcp.c"
#include "ermi_c_rtsp_parser.h"

#include "ermi_c_master.h"
#include "ermi_video_debug.h"
#include "ermi_c_rtsp_session.h"


static char rtsp_version_str[]      = "RTSP/1.0";
static char rtsp_teardown_str[]     = "TEARDOWN";
static char rtsp_setup_str[]        = "SETUP";
static char rtsp_setparam_str[]     = "SET_PARAMETER";
static char rtsp_getparam_str[]     = "GET_PARAMETER";
static char rtsp_response_str[]     = "RESPONSE";
static char rtsp_unrecognized_str[] = "UNRECOGNIZED";
static char rtsp_announce_str[]     = "ANNOUNCE";

static char ermi_require_hdr_str[]  = "com.cablelabs.ermi";

/* Convert from numerical codepoints to textual description
 */
char *ermi_c_rtsp_respcode2text (ushort resp_code)
{
    char *str;

    switch(resp_code) {
    case RTSP_RET_OK:
	str = "OK";
	break;
    case RTSP_RET_NOT_FOUND:
	str = "Not Found";
	break;
    case RTSP_RET_METHOD_NOT_ALLOWED:
	str = "Method Not Allowed";
	break;
    case RTSP_RET_REQUEST_TIMEOUT:
	str = "Request Time Out";
	break;
    case RTSP_RET_INVALID_PARAMETER:
	str = "Invalid Parameter";
	break;
    case RTSP_RET_NOT_ENOUGH_BANDWIDTH:
	str = "Not Enough Bandwidth";
	break;
    case RTSP_RET_SESSION_NOT_FOUND:
	str = "Session Not Found";
	break;
    case RTSP_RET_INVALID_RANGE:
	str = "Invalid Range";
	break;
    case RTSP_RET_UNSUPPORTED_TRANSPORT:
	str = "Unsupported Transport";
	break;
    case RTSP_RET_DESTINATION_UNREACHABLE:
	str = "Destination Unreachable";
	break;
    case RTSP_RET_INTERNAL_SERVER_ERROR:
	str = "Internal Server Error";
	break;
    case RTSP_RET_SERVICE_UNAVAILABLE:
	str = "Service Unavailable";
	break;
    case RTSP_RET_VERSION_NOT_SUPPORTED:
	str = "Version Not Supported";
	break;
    case RTSP_RET_INVALID:
    default:
	str = "Unknown";
	break;
    }
    return str;
}

char *ermi_c_rtsp_announcecode2text (ushort announce_code)
{
    char * str;

    switch (announce_code) {
    case RTSP_NOTICE_CODE_DOWNSTREAM_FAILURE: 
	str = "Downstream Failure";
	break;
    case RTSP_NOTICE_CODE_INTERNAL_SERVER_ERROR:
	str = "Internal Server Error";
	break;
    case RTSP_NOTICE_CODE_STREAM_MARKER_MISMATCH: 
	str = "Inband Stream Marker Mismatch";
	break;
    case RTSP_NOTICE_CODE_DELIVERY_SUCCEEDED:
        str = "Delivery succeeded";
	break;
    case RTSP_NOTICE_CODE_SERVER_RESOURCE_UNAVAILABLE:
        str = "Server Resource Unavailable";
	break;
    case RTSP_NOTICE_CODE_UNABLE_TO_JOIN:
        str = "Unable to Join";
	break;
    case RTSP_NOTICE_CODE_INPUT_PORT_FAILURE:
        str = "Input Port Failure";
	break;
    case RTSP_NOTICE_CODE_FAILOVER_TO_RED_SOURCE:
        str = "Failover to Redundant Source";
	break;
    case RTSP_NOTICE_CODE_ERROR_CONTENT_PID_CONFLICT:
        str = "Error Reading Content Data - PID Conflict";
	break;
    case RTSP_NOTICE_CODE_BANDWIDTH_EXCEEDED_LIMIT:
        str = "Bandwidth Exceeded Limit";
	break;
    case RTSP_NOTICE_CODE_SESSION_IN_PROGRESS:
        str = "Session in Progress";
	break;
    case RTSP_NOTICE_CODE_RECLAIM_SESSION:
        str = "Reclaim Session";
	break;
    default:
	str = "";
    }
    return str;
}


/*
 * Parse patterns of one of these forms:
 *  (1) <keyword>;
 *  (2) <keyword>\0
 *  (3) <keyword>=<value>;
 *  (4) <keyword>=<value>\0
 *
 *  INPUTS:
 *		ps: pointer to string
 *	       
 *  OUTPUTS:
 *	        kv: the kv structure is filled in
 *		kv.value is set to NULL if there is no '<value>' field, i.e.,
 *              cases (1) and (2) above.
 *  RETURN: 
 *   	NULL if string ended after parsing kv, i.e., cases (2) and (4) above.
 *	otherwise, return pointer to the next character in the input string.
 *  NOTES: 
 *    also strips leading-white-space  from kv.keyword and kv.value 
 */
static char *get_kvpair (char *ps, kvpair_t *kv)
{
    char *s = ps;
    char *vs = NULL;

    assert(ps);

    kv->keyword = ps;
    kv->value = NULL;

    while (*s) {
	if (*s == '=') {
	    *s = NUL;
	    vs = s+1;
	}

	if (*s == ';') {
	    *s = NUL;
	    kv->value = vs;
	    /* Skip Leading White Spaces */
	    SKIP_LWS(kv->keyword);
	    if (kv->value) 
		SKIP_LWS(kv->value);
	    return (s+1);
	}

	++s;
    }/*while*/
    
    kv->value = vs;
    if (kv->value) 
	SKIP_LWS(kv->value);
    SKIP_LWS(kv->keyword);
    return NULL;
}



/*
 * custom string tokenizer function
 *
 * INPUTS
 *	s1: start of string 
 *	delim: delimiter character
 *             delim can be any character EXCEPT '\0'
 * OUTPUTS
 *	next: pointer to next byte after delim	
 *      when the function returns NULL, because delim was not seen, 
 *      then 'next' points to the byte after eos or eol. 
 * RETURNS
 *	pointer to start of (null-terminated) token string.
 *      NULL if s1 is NULL
 *	NULL if end-of-string seen, and delim was not seen
 *      NULL if end-of-line seen,   and delim was not seen
 */
static char *strtoken (char *s1, char delim, char **next)
{
    char *s = s1;
    char *tok;
    
    if (!s1)
	return NULL;

    /* skip initial delimiter characters */
    while (*s==delim) s++;

    /* skip initial whitespace */
    SKIP_LWS(s);

    tok = s; /* start of token */

    /* find end of token */
    while (*s) {
	if (*s == delim) {
	    *s = NUL;
	    /* Special Case: if delim is LF, remove previous CR */
	    if (delim == LF && *(s-1) == CR)
		*(s-1) = NUL;
	    *next = s+1;
	    return tok;
	} else if (*s == LF) {
	    /* if we encounter end-of-line, stop the search */
	    break;
	}

	++s;
    }

    *next = s+1;
    return NULL;
}

/* 
 * Given a string like "rtsp://192.15.12.3/movies/topgun", extract the IP
 * address.
 * Or in, "rtsp://192.15.12.3:8234/movies/topgun", extract the IP address
 * and the port.
 */
static boolean ip_from_url (char *url_str, ipaddrtype *ipa, ushort *port)
{
    char *pa, *pb;
    char *token, *tp;
    uchar a1, a2, a3, a4;

    token = strtoken(url_str, ':', &pa);
    if (!token)
	return FALSE;
    if (*pa != '/' || *(pa+1) != '/')
	return FALSE;
    pa += 2;

    pb = pa; /* remember start of URL following rtsp:// */
    token = strtoken(pa, '/', &pa);
    if (!token) {
	/* the URL could contain just the hostname part */
	token = pb;
    }

    tp = strtoken(token, ':', &pa);
    if (tp) {
	/* a port number is present */
	*port = atoi(pa);
    }
    
    if (!isdigit(*token)) {
	/* can't be an IP address. Let's see if it is a hostname that 
	 * we can look up.  
	 */
	nametype *ptr;
	hostaddr *haddr;

	ptr = name_cache_lookup(token, NULL, NAM_IP);
	if (ptr == NULL)
	    return FALSE;
	haddr = name_first_addr_type(ptr, ADDR_IP);
	if (haddr == NULL)
	    return FALSE;
	*ipa = haddr->address.addr.ip_address;
	return TRUE;
    }

    token = strtoken(token, '.', &pa);
    if (!token)
	return FALSE;
    a1 = atoi(token);
    token = strtoken(pa, '.', &pa);
    if (!token)
	return FALSE;
    a2 = atoi(token);

    token = strtoken(pa, '.', &pa);
    if (!token)
	return FALSE;
    a3 = atoi(token);

    a4 = atoi(pa);

    *ipa = (a1<<24) | (a2<<16) | (a3<<8) | a4; 

    return TRUE;
}

/* Parse a string of the form "n1.n2", where n1 and n2 are numbers.
 * Return FALSE if parse failed; otherwise TRUE with values of n1, n2.
 * The "qam_destination" parameter in a transport spec has this format.
 */
static boolean parse_n1dotn2 (char *str, ulong *n1, ulong *n2) 
{   
    char *token, *pa;
    
    *n1 = *n2 = 0; 
    token = strtoken(str, '.', &pa);
    if (!token)	
	return FALSE;
    *n1 = atoi(token);
    *n2 = atoi(pa);
    return TRUE;
}


/******  Functions for parsing Header lines in RTSP messages *****/
/* All these header parsing functions have similar function prototypes. */
 
/*
 * parse a "<HeaderName>: <parameters>\r\n" header line
 * 
 *  INPUTS:
 *	pbuf: pointer to the byte following the ":"
 *  OUTPUTS:
 *		smsg: filled in after parsing
 *  RETURN: 
 *	pointer to first unparsed byte
 *  NOTES: 
 *
 */


static char *parse_cseq_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    smsg->cseq = atoi(pa);
    RPM_SETBIT(smsg->valid, RP_BIT_CSEQ);

    ERMI_C_PARSER_BUGINF("\nCSeq Header:  CSeq: %u", smsg->cseq);

    return next;
}


static char *parse_connection_timeout_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    smsg->conn_timeout = atoi(pa);
    RPM_SETBIT(smsg->valid, RP_BIT_CONN_TIMEOUT);
    return next;
}


static char *parse_contenttype_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    return next;
}


static char *parse_contentlength_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    smsg->content.len = atoi(pa);
    RPM_SETBIT(smsg->valid, RP_BIT_CONTENT);
    return next;
}

static char *parse_sessionlist_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    sstrncpy(smsg->session_group, pa, MAX_SESSGRP_LEN);
    while (FALSE) {
        // NK_TBD
    }
    RPM_SETBIT(smsg->valid, RP_BIT_SESSIONLIST);
    return next;
}

static char *parse_sessiongrouplist_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;
    char *token;
    uchar indx;

    pa = strtoken(pbuf, LF, &next);
    sstrncpy(smsg->session_group, pa, MAX_SESSGRP_LEN);

    ERMI_C_PARSER_BUGINF("\nsession_group_list string = %s", pa); // Temp
    for (indx = 0; (indx < MAX_SESSGRP_LIST_SIZE); indx++) {
        token = strtoken(pa, ' ', &pa);
        if ((strlen(token) == 0) || (token >= pa)) {
            ERMI_C_PARSER_BUGINF("\nFound %d SessionGroups in list", indx);//Tmp
            break;
        }
        sstrncpy(&(smsg->session_group_list[indx]), pa, MAX_SESSGRP_LEN);
    }
    RPM_SETBIT(smsg->valid, RP_BIT_SESSGRP_LIST);
    return next;
}

static char *parse_sessiongroup_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    sstrncpy(smsg->session_group, pa, MAX_SESSGRP_LEN);
    RPM_SETBIT(smsg->valid, RP_BIT_SESSGRP);
    return next;
}

static char *parse_require_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    if (strcmp(pa, ermi_require_hdr_str) == 0) {
	smsg->ftag = FTAG_ERMI;
    } else {
	ERMI_C_PRSERROR_BUGINF("\nUnrecognized Feature-tag <%s>", pa);
	smsg->ftag = FTAG_UNRECOGNIZED;
    }

    RPM_SETBIT(smsg->valid, RP_BIT_FTAG);

    ERMI_C_PARSER_BUGINF("\nRequire Header:  Tag:  %s",
           ((smsg->ftag == FTAG_ERMI) ? ermi_require_hdr_str : "Unrecognized"));

    return next;
}

static char *parse_reason_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;
    // char *str;

    pa = strtoken(pbuf, LF, &next);
    smsg->reason = atoi(pa);
    RPM_SETBIT(smsg->valid, RP_BIT_REASON);

    // str = strtoken(pbuf, "\"", &str);

    ERMI_C_PARSER_BUGINF("\nReason Header:  Reason:  %u", smsg->reason);

    return next;
}

static char *parse_session_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;
    kvpair_t kv;

    pa = strtoken(pbuf, LF, &next);

    smsg->sessionID = atoi(pa);
    RPM_SETBIT(smsg->valid, RP_BIT_SESSIONID);

    /* may contain optional substring-  ";timeout=<val>" */
    pa = get_kvpair(pa, &kv); /* ensure that sessionID value is 
			       * null-terminated 
			       */
    if (pa) {
	pa = get_kvpair(pa, &kv);

        ERMI_C_PARSER_BUGINF("\n%s=%s", kv.keyword, kv.value);

	if (strcmp(kv.keyword, "timeout")!=0) {
	    ERMI_C_PRSERROR_BUGINF("\nUnrecognized keyword <%s>. Expected timeout",
			      kv.keyword);
	} else {
	    smsg->timeout = atoi(kv.value);
	    RPM_SETBIT(smsg->valid, RP_BIT_TIMEOUT);
	}
	
	if (pa != NULL) {
	    ERMI_C_PRSERROR_BUGINF("\nSyntax error in Session hdr - %c", *pa);
	}
    }

    ERMI_C_PARSER_BUGINF("\nSession Header:  SessionID: %08X\ntimeout: %u",
                         smsg->sessionID, smsg->timeout);

    return next;
}


static char * parse_transport_header (char *pbuf, rtsp_msg_t *smsg, int i)
{
    char *pa;
    kvpair_t kv;
    uint hdr_count = 0;

    transhdr_t *tspec;

    tspec = &(smsg->transhdr[i]);
    tspec->tvalid = 0x0;

    pa = pbuf;

/* Headers Not YET parsed/supported:
        } else if (strcmp(kv.keyword, "fiber_node") == 0) {
        } else if (strcmp(kv.keyword, "depi_mode") == 0) {
        } else if (strcmp(kv.keyword, "frequency_range") == 0) {
        } else if (strcmp(kv.keyword, "modulation") == 0) {
        } else if (strcmp(kv.keyword, "j83_annex") == 0) {
        } else if (strcmp(kv.keyword, "taps") == 0) == 0) {
        } else if (strcmp(kv.keyword, "channel_width") == 0) {
        } else if (strcmp(kv.keyword, "symbol_rate") == 0) {
*/

    while (pa) {
	pa = get_kvpair(pa, &kv);

        ERMI_C_PARSER_BUGINF("\n%s %s", kv.keyword, kv.value);
        hdr_count++;

	if (strcmp(kv.keyword, "clab-MP2T/DVBC/QAM") == 0) {
	    tspec->type = MPEG_QAM;

	} else if (strcmp(kv.keyword, "clab-MP2T/DVBC/UDP") == 0) {
	    tspec->type = MPEG_UDP;

	} else if (strcmp(kv.keyword, "unicast") == 0) {
	    tspec->unicast = TRUE;
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_STREAM_TYPE);

	} else if (strcmp(kv.keyword, "multicast") == 0) {
	    tspec->unicast = FALSE;
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_STREAM_TYPE);

	} else if (strcmp(kv.keyword, "destination_port") == 0) {
	    tspec->dest_port = atoi(kv.value);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_DEST_PORT);

	} else if (strcmp(kv.keyword, "source_port") == 0) {
	    tspec->src_port = atoi(kv.value);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_SRC_PORT);

	} else if (strcmp(kv.keyword, "destination") == 0) {
	    ipaddrtype ipa;
	    
	    if (parse_ip_address(kv.value, &ipa)) {
		tspec->dest = ipa;
		RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_DEST);
	    } else {
		ERMI_C_PRSERROR_BUGINF("\nBad destination IP address"
				       "in Transport header");
            }

	} else if (strcmp(kv.keyword, "source") == 0) {
	    ipaddrtype ipa;
	    
	    if (parse_ip_address(kv.value, &ipa)) {
		tspec->src = ipa;
		RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_SRC);
	    } else
		ERMI_C_PRSERROR_BUGINF("\nBad Src IP Address in Transport Hdr");

        } else if (strcmp(kv.keyword, "multicast_address") == 0) {
            ipaddrtype ipa;

            if (parse_ip_address(kv.value, &ipa)) {
	        tspec->mcast_address = ipa;
                RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_MCAST_ADDR);

            } else
                ERMI_C_PRSERROR_BUGINF("\nBad Multicast IP address"
                                    "in Transport header");

        } else if (strcmp(kv.keyword, "rank") == 0) {
	    tspec->rank = atoi(kv.value);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_RANK);

        } else if (strcmp(kv.keyword, "mpts-program") == 0) {
	    tspec->mpts_program = atoi(kv.value);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_MPTS_PROGRAM);

	} else if (strcmp(kv.keyword, "qam_tsid") == 0) {
	    tspec->tsid = atoi(kv.value);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_QAM_TSID);

	} else if (strcmp(kv.keyword, "qam_name") == 0) {
            char *tsid_ptr, *tmp;

            sstrncpy(tspec->qam_name, kv.value, MAX_QAM_NAME_LEN);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_QAM_NAME);

            tmp = strtoken(kv.value, '.', &tsid_ptr);

            tspec->tsid = atoi(tsid_ptr);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_QAM_TSID);

	} else if (strcmp(kv.keyword, "bit_rate") == 0) {
	    tspec->bit_rate = atoi(kv.value);
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_BIT_RATE);

	} else if (strcmp(kv.keyword, "qam_destination") == 0) {
	    ulong freq, prognum;
	    parse_n1dotn2(kv.value, &freq, &prognum);
	    tspec->qam_frequency = freq;
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_FREQUENCY);
	    tspec->qam_prognum   = prognum;
	    RPM_SETBIT(tspec->tvalid, RP_BIT_TRANS_PROGNUM);

	} else {

	    /* Unrecognized keyword */
            ERMI_C_PRSERROR_BUGINF("\nUnknown keyword <%s>: Transport header %d"
                             "(%d hdrs parsed ok).", kv.keyword, i, hdr_count);
	    ERMI_C_PRSERROR_BUGINF("\nPending XportHdr (%d) (ParsedBits %#X) "
                                   "Buf: %d byte parsed.",
                                   i, tspec->tvalid, ((uint)pa-(uint)pbuf) );
	}
	
    } /*while*/

    ERMI_C_PARSER_BUGINF("\nParsed Transport Header # %d"
               "\nTvalid Bits: %#X"
               "\n  Type:       %u  Unicast:    %u"
               "\n  Bit_Rate:   %u"
               "\n  Dest_port:  %u  Src_port:   %u"
               "\n  Qam_name:   %s  Qam_tsid:   %d"
               "\n  Qam_freq:   %u  Qam_ProgNum %u"
               "\n  Dest:       %i  Source: %i  Mcast_Address: %i"
               "\n  Qam_Prognum:%u"
               "\n  Rank:       %u",
               i+1, tspec->tvalid,
               tspec->type, /* UDP or QAM */
               tspec->unicast,
               tspec->bit_rate,
               tspec->dest_port,
               tspec->src_port,
               tspec->qam_name,
               tspec->tsid,
               tspec->qam_frequency,
               tspec->qam_prognum,
               tspec->dest,
               tspec->src,
               tspec->mcast_address,
               tspec->mpts_program,
               tspec->rank);

    return pbuf;
}


static char * parse_multitransport_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *tok;
    char *next;
    char *last;
    int i = 0;
    
    pa = strtoken(pbuf, LF, &last);

    /*
     * The string 'pa' could contain multiple comma-separated transport headers
     */
    while (pa) {
	tok = pa;

        /* Separating multiple Transport headers with ',' into pa and next */
	pa = strtoken(tok, ',', &next);
	parse_transport_header(tok, smsg, i);
	if (pa) {
	    pa = next;
	}
	/* if pa==NULL, that means there are no more transport header in the 
	 * string buffer - pbuf.
	 */

	i++;
    }

    smsg->num_trans_hdrs = i;

    ERMI_C_PARSER_BUGINF("\n\nParsed %d Transport Headers", i);

    return last;
}


static char * parse_clab_clientsessionid_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    SKIP_LWS(pbuf);

    pa = strtoken(pbuf, LF, &next);
    sstrncpy(smsg->clientsessionID, pa, MAX_SESSID_LEN);
    RPM_SETBIT(smsg->valid, RP_BIT_CLIENTSESSIONID);

    ERMI_C_PARSER_BUGINF("\nParsed ClientSessionId Hdr: ClientSessionID: %s",
                         smsg->clientsessionID);

    return next;
}

static char *parse_clab_priority_header (char *pbuf, rtsp_msg_t *smsg)
{

    return NULL;
}

static char *parse_clab_setuptype_header (char *pbuf, rtsp_msg_t *smsg)
{

    return NULL;
}

static char *parse_clab_existing_session_header (char *pbuf, rtsp_msg_t *smsg)
{

    return NULL;
}

static char *parse_clab_pidremap_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);
    smsg->pid_remap = atoi(pa);
    RPM_SETBIT(smsg->valid, RP_BIT_PIDREMAP);

    ERMI_C_PARSER_BUGINF("\nclab-PidRemap Header:  PidRemap: %d",
                         smsg->pid_remap);

    return next;
}

static char *parse_clab_mptsmode_header (char *pbuf, rtsp_msg_t *smsg)
{
    char *pa;
    char *next;

    pa = strtoken(pbuf, LF, &next);

    if (strncmp(pa, "passthrough", strlen("passthrough"))) {

        smsg->mpts_mode = ERMI_MPTS_MODE_PASSTHROUGH;

    } else if (strncmp(pa, "multiplex", strlen("multiplex"))) {

        smsg->mpts_mode = ERMI_MPTS_MODE_MULTIPLEX;

    } else {
        smsg->mpts_mode = ERMI_MPTS_MODE_UNKNOWN;
    }

    RPM_SETBIT(smsg->valid, RP_BIT_MPTS_MODE);

    ERMI_C_PARSER_BUGINF("\nclab-MPTSMode Header:  MPTS Mode: %d",
                         smsg->mpts_mode);

    return next;
}

static char *parse_clab_statmuxgroup_header (char *pbuf, rtsp_msg_t *smsg)
{

    return NULL;
}


/******* Functions for parsing RTSP Methods *****************/

/*
 * parse a RTSP RESPONSE message
 * 
 *
 *  INPUTS:
 *		buf: pointer to rtsp msg. null-terminated.
 *  OUTPUTS:
 *		smsg: filled in after parsing
 *	  	count: number of bytes consumed.
 *  RETURN: 
 *
 *  NOTES: after this function returns, some bytes in the input buffer, upto
 *  'count' bytes, may be modified (e.g. overwritten with \0).
 *  The buffer beyond the 'count' bytes is guaranteed to be untouched.
 */
static ermi_status
parse_rtsp_resp (char *buf, rtsp_msg_t *smsg, int *count)
{
    char *token;
    char *next;
    char *phdr;

    /* parse the first line */

    token = strtoken(buf, SPACE, &next);
    if (strcmp(token, rtsp_version_str) != 0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. RTSP Response header is bad");
	return ERMI_PARSE_ERROR;
    }

    smsg->msg_type = RTSP_RESPONSE;
	
    token = strtoken(next, SPACE, &next);
    if (token == NULL) {
	ERMI_C_PRSERROR_BUGINF("\n Could  not parse retcode in Response");
	return ERMI_PARSE_ERROR;
    }

    smsg->resp_retcode = atoi(token);
    
    token = strtoken(next, LF, &next);
    /* Don't  do anything with the retphrase */
    /* sstrncpy(smsg->resp_retphrase, token, RTSP_RETPHRASE_LEN); */

    /* parse other lines in the response message */
    while (1) {

	if (*next == CR && *(next+1) == LF) {
	    *count = next + 2 - buf;
	    return ERMI_OK;
	}
	
	phdr = next;
	token = strtoken(next, COLON, &next);
	
	if (!token) {
	    ERMI_C_PRSERROR_BUGINF("\n strtoken returned NULL when looking for"
			      "Colon in <%s>."
			      " Skipping to the next line...", phdr);
	    if (*(next-1) == LF)
		continue;
	    return ERMI_PARSE_ERROR;
	}

	if (strcmp(token, "CSeq")==0) {
	    next = parse_cseq_header(next, smsg);

	} else if (strcmp(token, "Session")==0) {
	    next = parse_session_header(next, smsg);

	} else if (strcmp(token, "clab-ClientSessionId")==0) {
	    next = parse_clab_clientsessionid_header(next, smsg);

	} else if (strcmp(token, "Transport")==0) {
	    next = parse_multitransport_header(next, smsg);
	    if (smsg->num_trans_hdrs > 1) {
		ERMI_C_PRSERROR_BUGINF("\n Response contains %d transport specs",
				    smsg->num_trans_hdrs);
	    }

 	} else if (strcmp(token, "Content-Length")==0) {
	    next = parse_contentlength_header(next, smsg);

	} else if (strcmp(token, "Content-Type")==0) {
	    next = parse_contenttype_header(next, smsg);

 	} else if (strcmp(token, "clab-connection-timeout")==0) {
	    next = parse_connection_timeout_header(next, smsg);

	} else {
	    ERMI_C_PRSERROR_BUGINF("\n Unrecognized header in response - <%s>."
			      "Skipped...", 
			      token);
	    next = strstr(next, "\r\n");
	    if (next == NULL) {
		ERMI_C_PRSERROR_BUGINF("\nCan't locate end-of-line after"
				" - <%s>", token); 
		return ERMI_PARSE_ERROR;
	    }

	    next += 2;
	    continue;
	}

    }/*while*/

    return ERMI_OK;
}

/*
 * parse a RTSP SETUP Request message
 *
 *  INPUTS:
 *		buf: pointer to rtsp msg. null-terminated.
 *  OUTPUTS:
 *		smsg: filled in after parsing
 *	  	count: number of bytes consumed.
 *  RETURN: 
 *
 *  NOTES: after this function returns, some bytes in the input buffer, upto
 *  'count' bytes, may be modified (e.g. overwritten with \0).
 *  The buffer beyond the 'count' bytes is guaranteed to be untouched.
 */
static ermi_status
parse_rtsp_setup (char *buf, rtsp_msg_t *smsg, int *count)
{
    char *token;
    char *next;
    int sl;
    char *phdr;
    ipaddrtype ipa;
    ushort port = DEFAULT_RTSP_PORT;
    boolean ret;

    /* parse the first line */

    token = strtoken(buf, SPACE, &next);
    if (strcmp(token, "SETUP") !=0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. RTSP SETUP header is bad");
	return ERMI_PARSE_ERROR;
    }

    smsg->msg_type = RTSP_SETUP;
	
    token = strtoken(next, SPACE, &next);
    sl = strlen(token);
    if (sl > (MAX_URL_LEN-1)) {
        ERMI_C_PRSERROR_BUGINF("\n%s: Too Long Url - %d bytes. <%s>", 
			    __FUNCTION__, sl, token);
    } else if (sl == 0) {
	ERMI_C_PRSERROR_BUGINF("\n%s: URL not found", __FUNCTION__);
	return ERMI_PARSE_ERROR;
    } else {
	sstrncpy(smsg->url, token, MAX_URL_LEN);
	RPM_SETBIT(smsg->valid, RP_BIT_URL);
	smsg->port_in_url = DEFAULT_RTSP_PORT;
	ret = ip_from_url(token, &ipa, &port);
	if (ret) {
	    smsg->ip_in_url = ipa;
	    smsg->port_in_url = port;
	    RPM_SETBIT(smsg->valid, RP_BIT_IP_IN_URL);
	}
    }
    
    token = strtoken(next, LF, &next);
    if (strcmp(token, rtsp_version_str) != 0) {
        ERMI_C_PRSERROR_BUGINF("\nError. Expected SETUP header to end with"
                               " RTSP/1.0");
	return ERMI_PARSE_ERROR;
    }
    
    /* parse other lines in the setup message */
    while (1) {

	if (*next == CR && *(next+1) == LF) {
	    *count = next + 2 - buf;
	    return ERMI_OK;
	}

	phdr = next;
	token = strtoken(next, COLON, &next);
	
	if (!token) {
	    ERMI_C_PRSERROR_BUGINF("\nstrtoken returned NULL when looking for"
				   " Colon in <%s>."
			           " Skipping to the next line...", phdr);
	    if (*(next-1) == LF)
		continue;
	    *count = next - buf;
	    return ERMI_PARSE_ERROR;
	}

        /* RTSP Style Headers in ERMI-2 (ERMI-C) */
	if (strcmp(token, "CSeq")==0) {
	    next = parse_cseq_header(next, smsg);

	} else if (strcmp(token, "Require")==0) {
	    next = parse_require_header(next, smsg);

	} else if (strcmp(token, "Transport")==0) {
	    next = parse_multitransport_header(next, smsg);
  
        /* RTSP Header Extensions for ERMI-2 (ERMI-C) */
	} else if (strcmp(token, "clab-ClientSessionId")==0) {
	    next = parse_clab_clientsessionid_header(next, smsg);
  
        } else if (strcmp(token, "clab-SessionGroup")==0) {
            next = parse_sessiongroup_header(next, smsg);

        } else if (strcmp(token, "clab-Priority")==0) {
            next = parse_clab_priority_header(next, smsg);

        } else if (strcmp(token, "clab-SetupType")==0) {
            next = parse_clab_setuptype_header(next, smsg);

        } else if (strcmp(token, "clab-Existing-Session")==0) {
            next = parse_clab_existing_session_header(next, smsg);

        } else if (strcmp(token, "clab-PidRemap")==0) {
            next = parse_clab_pidremap_header(next, smsg);

        } else if (strcmp(token, "clab-MPTSMode")==0) {
            next = parse_clab_mptsmode_header(next, smsg);

        } else if (strcmp(token, "clab-StatmuxGroup")==0) {
            next = parse_clab_statmuxgroup_header(next, smsg);

	} else {
	    ERMI_C_PRSERROR_BUGINF("\n Unrecognized header in SETUP - <%s>."
			      "Skipped...", 
			      token);
	    next = strstr(next, "\r\n");
	    if (next == NULL) {
		ERMI_C_PRSERROR_BUGINF("\nCan't locate end-of-line after"
				" - <%s>", token); 
		return ERMI_PARSE_ERROR;
	    }

	    next += 2;
	    continue;
	}

    }/*while*/
    
    return ERMI_OK;
}


/*
 * parse a RTSP TEARDOWN Request message
 * 
 *
 *  INPUTS:
 *		buf: pointer to rtsp msg. null-terminated.
 *  OUTPUTS:
 *		smsg: filled in after parsing
 *	  	count: number of bytes consumed.
 *  RETURN: 
 *
 *  NOTES: after this function returns, some bytes in the input buffer, upto
 *  'count' bytes, may be modified (e.g. overwritten with \0).
 *  The buffer beyond the 'count' bytes is guaranteed to be untouched.
 */
static ermi_status
parse_rtsp_teardown (char *buf, rtsp_msg_t *smsg, int *count)
{
    char *token;
    char *next;
    int sl;
    char *phdr;
    ipaddrtype ipa;
    ushort port = DEFAULT_RTSP_PORT;
    boolean ret;

    /* parse the first line */

    token = strtoken(buf, SPACE, &next);
    if (strcmp(token, rtsp_teardown_str) !=0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. RTSP TEARDOWN header is bad");
	return ERMI_PARSE_ERROR;
    }

    smsg->msg_type = RTSP_TEARDOWN;
	
    token = strtoken(next, SPACE, &next);
    sl = strlen(token);
    if (sl > (MAX_URL_LEN-1)) {
	ERMI_C_PRSERROR_BUGINF("\n%s: Too Long Url - %d bytes. <%s>", 
			    __FUNCTION__, sl, token);
	sstrncpy(smsg->url, token, MAX_URL_LEN);
    } else if (sl == 0) {
	ERMI_C_PRSERROR_BUGINF("\n%s: URL not found", __FUNCTION__);
	return ERMI_PARSE_ERROR;
    } else {
	strcpy(smsg->url, token); 
	smsg->port_in_url = DEFAULT_RTSP_PORT;
	ret = ip_from_url(token, &ipa, &port);
	if (ret) {
	    smsg->ip_in_url = ipa;
	    smsg->port_in_url = port;
	    RPM_SETBIT(smsg->valid, RP_BIT_IP_IN_URL);
	}
    }

    token = strtoken(next, LF, &next);
    if (strcmp(token, rtsp_version_str) !=0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. Expected TEARDOWN header to end with"
			  " RTSP/1.0");
	return ERMI_PARSE_ERROR;
    }
    
    /* parse other lines in the message */
    while (1) {

	if (*next == CR && *(next+1) == LF) {
	    *count = next + 2 - buf;
	    return ERMI_OK;
	}

	phdr = next;
	token = strtoken(next, COLON, &next);
	
	if (!token) {
	    ERMI_C_PRSERROR_BUGINF("\n strtoken returned NULL when looking for"
			      " Colon in <%s>"
			      " Skipping to the next line", phdr);
	    if (*(next-1) == LF)
		continue;
	    *count = next - buf;
	    return ERMI_PARSE_ERROR;
	}

	if (strcmp(token, "CSeq")==0) {
	    next = parse_cseq_header(next, smsg);

	} else if (strcmp(token, "Session")==0) {
	    next = parse_session_header(next, smsg);

	} else if (strcmp(token, "Require")==0) {
	    next = parse_require_header(next, smsg);

	} else if (strcmp(token, "clab-Reason")==0) {
	    next = parse_reason_header(next, smsg);

        } else if (strcmp(token, "clab-ClientSessionId")==0) {
            next = parse_clab_clientsessionid_header(next, smsg);

	} else {
	    ERMI_C_PRSERROR_BUGINF("\n Unrecognized header in TEARDOWN - <%s>."
			      "Skipped...",
			      token);
	    next = strstr(next, "\r\n");
	    if (next == NULL) {
		ERMI_C_PRSERROR_BUGINF("\nCan't locate end-of-line after"
				" - <%s>", token); 
		return ERMI_PARSE_ERROR;
	    }

	    next += 2;
	    continue;
	}

    }/*while*/

    return ERMI_OK;
}

/*
 * parse a RTSP SET_PARAMETER Request message
 * 
 *
 *  INPUTS:
 *		buf: pointer to rtsp msg. null-terminated.
 *  OUTPUTS:
 *		smsg: filled in after parsing
 *	  	count: number of bytes consumed.
 *  RETURN: 
 *
 *  NOTES: after this function returns, some bytes in the input buffer, upto
 *  'count' bytes, may be modified (e.g. overwritten with \0).
 *  The buffer beyond the 'count' bytes is guaranteed to be untouched.
 */
static ermi_status
parse_rtsp_setparameter (char *buf, rtsp_msg_t *smsg, int *count)
{
    char *token;
    char *next;
    int sl;
    char *phdr;
    ipaddrtype ipa;
    ushort port = DEFAULT_RTSP_PORT;
    boolean ret;

    /* parse the first line */

    token = strtoken(buf, SPACE, &next);
    if (strcmp(token, rtsp_setparam_str) !=0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. RTSP SET_PARAMETER header is bad");
	return ERMI_PARSE_ERROR;
    }

    smsg->msg_type = RTSP_SETPARAMETER;

    token = strtoken(next, SPACE, &next);
    sl = strlen(token);
    if (sl > (MAX_URL_LEN-1)) {
	ERMI_C_PRSERROR_BUGINF("\n%s: Too Long Url - %d bytes. <%s>", 
			    __FUNCTION__, sl, token);
	sstrncpy(smsg->url, token, MAX_URL_LEN);
    } else if (sl == 0) {
	ERMI_C_PRSERROR_BUGINF("\n%s: URL not found", __FUNCTION__);
	return ERMI_PARSE_ERROR;
    } else {
	strcpy(smsg->url, token); 
	smsg->port_in_url = DEFAULT_RTSP_PORT;
	ret = ip_from_url(token, &ipa, &port);
	if (ret) {
	    smsg->ip_in_url = ipa;
	    smsg->port_in_url = port;
	    RPM_SETBIT(smsg->valid, RP_BIT_IP_IN_URL);
	}
    }
    
    token = strtoken(next, LF, &next);
    if (strcmp(token, rtsp_version_str) !=0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. Expected SET_PARAMETER header to end"
			  " with RTSP/1.0");
	return ERMI_PARSE_ERROR;
    }
    
    /* parse other lines in the message */
    while (1) {

	if (*next == CR && *(next+1) == LF) {
	    *count = next + 2 - buf;
	    return ERMI_OK;
	}

	phdr = next;
	token = strtoken(next, COLON, &next);
	
	if (!token) {
	    ERMI_C_PRSERROR_BUGINF("\n strtoken returned NULL when looking for"
			      " Colon in <%s>"
			      " Skipping to the next line", phdr);
	    if (*(next-1) == LF)
		continue;
	    *count = next - buf;
	    return ERMI_PARSE_ERROR;
	}

	if (strcmp(token, "CSeq")==0) {
	    next = parse_cseq_header(next, smsg);

        } else if (strcmp(token, "Require")==0) {
            next = parse_require_header(next, smsg);

	} else if (strcmp(token, "Session")==0) {
	    next = parse_session_header(next, smsg);

	} else if (strcmp(token, "Content-Length")==0) {
	    next = parse_contentlength_header(next, smsg);

	} else if (strcmp(token, "Content-Type")==0) {
	    next = parse_contenttype_header(next, smsg);

        } else if (strcmp(token, "clab-SessionGroup")==0) {
            next = parse_sessiongroup_header(next, smsg);

        } else if (strcmp(token, "clab-sessiongroup-list")==0) {
            smsg->session_group_list = malloc(MAX_SESSGRP_LIST_SIZE *
                                              MAX_SESSGRP_LEN * sizeof(char));
            next = parse_sessiongrouplist_header(next, smsg);

	} else {
	    ERMI_C_PRSERROR_BUGINF("\nUnrecognized header in SET_PARAMETER - <%s>."
			      " Skipped...",
			      token);
	    next = strstr(next, "\r\n");
	    if (next == NULL) {
		ERMI_C_PRSERROR_BUGINF("\nCan't locate end-of-line after"
				" - <%s>", token); 
		return ERMI_PARSE_ERROR;
	    }

	    next += 2;
	    continue;
	}

    } /*while*/

    return ERMI_OK;
}


/*
 * parse a RTSP GET_PARAMETER Request message
 * 
 *
 *  INPUTS:
 *		buf: pointer to rtsp msg. null-terminated.
 *  OUTPUTS:
 *		smsg: filled in after parsing
 *	  	count: number of bytes consumed.
 *  RETURN: 
 *
 *  NOTES: after this function returns, some bytes in the input buffer, upto
 *  'count' bytes, may be modified (e.g. overwritten with \0).
 *  The buffer beyond the 'count' bytes is guaranteed to be untouched.
 */
static ermi_status
parse_rtsp_getparameter (char *buf, rtsp_msg_t *smsg, int *count)
{
    char *token;
    char *next;
    int sl;
    char *phdr;
    ipaddrtype ipa;
    ushort port = DEFAULT_RTSP_PORT;
    boolean ret;

    /* parse the first line */

    token = strtoken(buf, SPACE, &next);
    if (strcmp(token, rtsp_getparam_str) !=0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. RTSP GET_PARAMETER header is bad");
	return ERMI_PARSE_ERROR;
    }

    smsg->msg_type = RTSP_GETPARAMETER;

    token = strtoken(next, SPACE, &next);
    sl = strlen(token);
    if (sl > (MAX_URL_LEN-1)) {
	ERMI_C_PRSERROR_BUGINF("\n%s: Truncated Too-Long Url - %d bytes. <%s>", 
			    __FUNCTION__, sl, token);
	sstrncpy(smsg->url, token, MAX_URL_LEN);
    } else if (sl == 0) {
	ERMI_C_PRSERROR_BUGINF("\n%s: URL not found", __FUNCTION__);
	return ERMI_PARSE_ERROR;
    } else {
	strcpy(smsg->url, token); 
	smsg->port_in_url = DEFAULT_RTSP_PORT;
	ret = ip_from_url(token, &ipa, &port);
	if (ret) {
	    smsg->ip_in_url = ipa;
	    smsg->port_in_url = port;
	    RPM_SETBIT(smsg->valid, RP_BIT_IP_IN_URL);
	}
    }
    
    token = strtoken(next, LF, &next);
    if (strcmp(token, rtsp_version_str) !=0) {
	ERMI_C_PRSERROR_BUGINF("\n Error. Expected GET_PARAMETER header to end"
			  " with RTSP/1.0");
	return ERMI_PARSE_ERROR;
    }
    
    /* parse other lines in the message */
    while (1) {

	if (*next == CR && *(next+1) == LF) {
	    *count = next + 2 - buf;
	    return ERMI_OK;
	}

	phdr = next;

	token = strtoken(next, COLON, &next);
	
        if ( !token && (strcmp(phdr, "clab-session-list") !=0) ) {
            /*
             * "clab-session-list" hdr is not-COLON-ed in 'ALL-session case'.
             */
	    ERMI_C_PRSERROR_BUGINF("\nstrtoken returned NULL when looking"
			           " for Colon in <%s>"
			           " Skipping to the next line.", phdr);
	    if (*(next-1) == LF) {
		continue;
	    }
            *count = next - buf;
	    return ERMI_PARSE_ERROR;
	}

	if (strcmp(token, "CSeq")==0) {
	    next = parse_cseq_header(next, smsg);

	} else if (strcmp(token, "Require")==0) {
	    next = parse_require_header(next, smsg);

	} else if (strcmp(token, "Content-Length")==0) {
	    next = parse_contentlength_header(next, smsg);

	} else if (strcmp(token, "Content-Type")==0) {
	    next = parse_contenttype_header(next, smsg);

        } else if (strcmp(token, "clab-SessionGroup")==0) {
            next = parse_sessiongroup_header(next, smsg);

	} else if (strcmp(token, "clab-session-list")==0) {
	    next = parse_sessionlist_header(next, smsg);

 	} else if (strcmp(token, "clab-connection-timeout")==0) {
	    next = parse_connection_timeout_header(next, smsg);

	} else {
	    ERMI_C_PRSERROR_BUGINF("\nUnrecognized header in GET_PARAMETER"
				   " - <%s>. Skipped...", token);
	    next = strstr(next, "\r\n");
	    if (next == NULL) {
		ERMI_C_PRSERROR_BUGINF("\nCan't locate end-of-line after"
				       " - <%s>", token); 
		return ERMI_PARSE_ERROR;
	    }

	    next += 2;
	    continue;
	}

    } /* while */

    return ERMI_OK;
}


/*
 * parse a single message received by an RTSP client or server from the input 
 * buffer.
 * 
 *
 *  INPUTS:
 *		rbuf: pointer to start of message
 *                    buffer must be null-terminated
 *
 *  OUTPUTS:
 *		smsg:  struct is filled in after parsing the RTSP message 
 *	  	count: number of bytes parsed
 *  RETURN: 
 *		ERMI_OK - parsed a message without errors
 *		ERMI_PARSE_ERROR - parsed a message with errors
 *		VP_PARSE_INCOMPLETE_MSG - did not see a complete message;
 *			                  did not parse anything.
 *
 * SIDE-EFFECTS:
 *		after this function returns, some bytes in the input buffer, 
 *              upto 'count' bytes, may be modified (e.g. overwritten with \0).
 *              The buffer beyond the 'count' bytes is guaranteed to be 
 *              untouched.
 *
 *  NOTES: 
 */
ermi_status
ermi_c_rtsp_parse_message (char *rbuf, ushort len, rtsp_msg_t *smsg, uint *count)
{
    int ret;
    char *eomp;
    ushort mlen;
    char *msg_type_str = NULL;

    assert(rbuf);
    assert(smsg);

    /* ignore NULs */
    if (*rbuf == NUL) {
	*count = 1;
	return ERMI_PARSE_ERROR;
    }

    /*
     * Verify - end-of-message, before parsing.
     *
     * Note: If there's a EOM at the end of the msg (last 12 bytes). Only last 12
     * bytes being scanned, just to optimize CPU usage for this check.
     */
    eomp = strstr( rbuf /* &rbuf[len - (EOM_LEN*3)] */, "\r\n\r\n" );
    if (!eomp) {
	ERMI_C_PRSERROR_BUGINF("\n%s: Can't find eom in message", __FUNCTION__);
	*count = 0;
	return ERMI_PARSE_INCOMPLETE_MSG;
    }

    mlen = eomp + EOM_LEN - rbuf;

    smsg->valid = 0;
    smsg->num_trans_hdrs = 0;
    
    if (strncmp(rbuf, rtsp_setup_str, strlen(rtsp_setup_str)) == 0) {

	/* SETUP Request */
	ret = parse_rtsp_setup(rbuf, smsg, count);
	msg_type_str = rtsp_setup_str;

    } else if (strncmp(rbuf, rtsp_teardown_str, strlen(rtsp_teardown_str)) ==0) {

	/* TEARDOWN Request */
	ret = parse_rtsp_teardown(rbuf, smsg, count);
	msg_type_str = rtsp_teardown_str;

    } else if (strncmp(rbuf, rtsp_setparam_str, 
		       strlen(rtsp_setparam_str)) == 0) {

	/* SET_PARAMETER Request */
	/* or Keepalive Request */
	ret = parse_rtsp_setparameter(rbuf, smsg, count);
	msg_type_str = rtsp_setparam_str;

    }  else if (strncmp(rbuf, rtsp_getparam_str, 
		        strlen(rtsp_getparam_str)) == 0) {

	/* GET_PARAMETER Request */
	ret = parse_rtsp_getparameter(rbuf, smsg, count);
	msg_type_str = rtsp_getparam_str;

    } else if (strncmp(rbuf, rtsp_version_str, strlen(rtsp_version_str)) == 0) {

	/* RTSP Version Header - means - RTSP Response (for ANNOUNCE Request) */
	ret = parse_rtsp_resp(rbuf, smsg, count);
	msg_type_str = rtsp_response_str;

    } else {

	/* Unrecognized message-header. Skip this message */
	ERMI_C_PRSERROR_BUGINF("\n Unrecognized message header %s", rbuf);
	ret = ERMI_PARSE_ERROR;
	msg_type_str = rtsp_unrecognized_str;

    }

    if (ret == ERMI_PARSE_ERROR) {
	ERMI_C_PRSERROR_BUGINF("\n%s: Skipping to end of bad message",
			       __FUNCTION__); 
    } else {
	if (*count != mlen) /* Unexpected */
	    ERMI_C_PRSERROR_BUGINF("\n%s: Mismatched counts. count=%d  mlen=%d",
			           __FUNCTION__, *count, mlen);
    }

    ERMI_C_PRSEBRIEF_BUGINF("\nIN RTSP\n%s Msg CSeq %d",msg_type_str,smsg->cseq);

    if (RPM_TESTBIT(smsg->valid, RP_BIT_CONTENT)) {
	ret = ERMI_PARSE_CONTENT_AFTER_MSG;
    }

    /*
     * Update Message Receive/Parser Success/Error Stats
     */
    ermi_c_stats_incr_rcvd(ERMI_C_GET_METHOD(smsg->msg_type));
    if (ret != ERMI_OK) {
        ermi_c_stats_incr_rcvd_err(ERMI_C_GET_METHOD(smsg->msg_type));
    }

    *count = mlen;
    return ret;
}

/**********  Binary-to-Text Conversion (i.e. Encoding) code Begins ******/

#define BASE10 10

static     char tbuf[40]; /* temporary buffer */

static char CSeq_str[] = "CSeq: ";
static char eventdate_str[] = "event-date=";
static char npt_str[] = "npt=";
static char Contenttype_str[] = "Content-Type: text/parameters";
static char Contentlength_str[] = "Content-Length: ";
static char Session_str[] = "Session: ";
static char clab_clientsessionid_str[] = "clab-ClientSessionId:";
static char Timeout_str[] = ";Timeout: ";
static char Transport_str[] = "Transport: ";

static char MPEG_QAM_str[] = "clab-MP2T/DVBC/QAM";
static char MPEG_UDP_str[] = "clab-MP2T/DVBC/UDP";
static char unicast_str[] = ";unicast";
static char multicast_str[] = ";multicast";
static char destination_addr_str[] = ";destination=";
static char source_addr_str[] = ";source=";
static char dest_port_str[] = ";destination_port=";
static char src_port_str[] = ";source_port=";
static char bit_rate_str[] = ";bit_rate=";
static char tsid_str[] = ";tsid=";
static char qam_name_str[] = ";qam_name=";
static char qamdestination_str[] = ";qam_destination=";
static char mcast_addr_str[] = ";multicast_address=";
static char rank_str[] = ";rank=";
static char mpts_program_str[] = ";mpts_program=";
static char Require_str[] = "Require: ";

static char clab_notice_str[]       = "clab-Notice: ";

#ifdef NK_NOT_USED
static char clab_seslistreq_str[] = "clab-session-list: ";
static char clab_Reason_str[] = "clab-Reason: ";
static char clab_sessiongroup_str[] = "clab-SessionGroup: ";
#endif // NK_NOT_USED

#ifdef NK_NOT_USED
static char clab_priority_str[] = "clab-Priority: ";
static char clab_setuptype_str[] = "clab-SetupType: ";
static char clab_existingsession_str[] = "clab-Existing-Session: ";
static char clab_pidremap_str[] = "clab-PidRemap: ";
static char clab_mptsmode_str[] = "clab-MPTSMode: ";
static char clab_statmuxgroup_str[] = "clab-StatmuxGroup: ";
#endif // NK_NOT_USED

static void iptoascii (ipaddrtype addr, char *str)
{
    sprintf(str,"%d.%d.%d.%d",
            (addr >> 24) & 0xFF,
            (addr >> 16) & 0xFF,
            (addr >> 8)  & 0xFF,
            addr & 0xFF);
}


/* 
 * copied from ccsip_sip_utils.c
 *
 * va - removed leading space.
 *    - removed code to deal with negative base
 */ 
static void inttoascii (unsigned val, char *str, int base)
{
    char tmp[11];
    char *right;

    tmp[10] = '\0';
    right = &tmp[9];

    do {
	*right-- = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" [val % base];
    } while ((val /= base) != 0);

    right++;
    while (*right)
    {
	*str++ = *right++;
    }
    *str = '\0';
}

static char timestring[24];
static char *get_datetime_string (void)
{
    clock_epoch curtime;
    ios_timeval tv;
    
    clock_get_time(&curtime);
    clock_epoch_to_timeval(&curtime, &tv, 0);
    format_time(timestring, sizeof(timestring), "%Y%m%dT%H%M%S.%kZ", &tv, NULL);
    return timestring;
}

static inline void add_space (char *buf_)
{
    *(buf_) = SPACE;
}

static inline void add_quote (char *buf_)
{
    *(buf_) = '"';
}

static inline void add_eol (char *buf_)
{
    *(buf_) = CR;
    *(buf_ + 1) = LF;
}


static void add_resp_header (rtsp_msg_t *smsg, char *buf, char **next)
{
    int sl;
    char *pb = buf;
    char *retphrase;

    memcpy(pb, rtsp_version_str, (sl=strlen(rtsp_version_str)));
    pb += sl;
    add_space(pb); pb++;
    inttoascii(smsg->resp_retcode, tbuf, BASE10);
    memcpy(pb, tbuf, (sl=strlen(tbuf)));
    pb += sl;
    add_space(pb); pb++;
    retphrase = ermi_c_rtsp_respcode2text(smsg->resp_retcode);
    memcpy(pb, retphrase, (sl=strlen(retphrase)));
    pb += sl;
    add_eol(pb);
    pb += 2;
    
    *next = pb;
}

static void add_message_header (rtsp_msg_t *smsg, char *buf, char **next)
{
    int sl;
    char *pb = buf;
    char *rtsp_method_str;

    switch (smsg->msg_type) {
    case RTSP_RESPONSE:
	rtsp_method_str = rtsp_response_str;
	break;
    case RTSP_SETUP:
	rtsp_method_str = rtsp_setup_str;
	break;
    case RTSP_TEARDOWN:
	rtsp_method_str = rtsp_teardown_str;
	break;
    case RTSP_GETPARAMETER:
	rtsp_method_str = rtsp_getparam_str;
	break;
    case RTSP_ANNOUNCE:
	rtsp_method_str = rtsp_announce_str;
	break;
    default:
	rtsp_method_str = rtsp_unrecognized_str;
	break;
    }

    memcpy(pb, rtsp_method_str, (sl=strlen(rtsp_method_str)));
    pb += sl;
    add_space(pb); pb++;
    memcpy(pb, smsg->url, (sl=strlen(smsg->url)));
    pb += sl;
    add_space(pb); pb++;
    memcpy(pb, rtsp_version_str, (sl=strlen(rtsp_version_str)));
    pb += sl;

    add_eol(pb);
    pb += 2;
    
    *next = pb;
}


static void add_cseq (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, CSeq_str, (sl=strlen(CSeq_str)));
    pb += sl;
    inttoascii(smsg->cseq, tbuf, BASE10);
    memcpy(pb, tbuf, (sl=strlen(tbuf)));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}

static void add_contentlength (int content_length, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, Contentlength_str, (sl=strlen(Contentlength_str)));
    pb += sl;
    inttoascii(content_length, tbuf, BASE10);
    memcpy(pb, tbuf, (sl=strlen(tbuf)));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}


static void add_contenttype (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, Contenttype_str, (sl=strlen(Contenttype_str)));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}

#ifdef NK_NOT_USED
static void add_reason (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, clab_Reason_str, (sl=strlen(clab_Reason_str)));
    pb += sl;
    inttoascii(smsg->reason, tbuf, BASE10);
    memcpy(pb, tbuf, (sl=strlen(tbuf)));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}
#endif // NK_NOT_USED

static void add_require (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, Require_str, (sl=strlen(Require_str)));
    pb += sl;
    memcpy(pb, ermi_require_hdr_str, sl=strlen(ermi_require_hdr_str));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}


static void add_session (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, Session_str, (sl=strlen(Session_str)));
    pb += sl;
    pb += snprintf(pb, SESSID_LEN8, "%08X", smsg->sessionID);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_TIMEOUT)) {
	memcpy(pb, Timeout_str, (sl=strlen(Timeout_str)));
	pb += sl;
	inttoascii(smsg->timeout, tbuf, BASE10);
	memcpy(pb, tbuf, (sl=strlen(tbuf)));
	pb += sl;
    }
	
    add_eol(pb);
    pb += 2;

    *next = pb;
}

static void add_clientsessionid (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, clab_clientsessionid_str, (sl=strlen(clab_clientsessionid_str)));
    pb += sl;
    memcpy(pb, smsg->clientsessionID, (sl=strlen(smsg->clientsessionID)));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}

#ifdef NK_NOT_USED
static void add_session_group (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    
    memcpy(pb, clab_sessiongroup_str, (sl=strlen(clab_sessiongroup_str)));
    pb += sl;
    memcpy(pb, smsg->session_group, (sl=strlen(smsg->session_group)));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}
#endif // NK_NOT_USED

static void add_notice (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    char *texts;
    char *timestr;
    
    memcpy(pb, clab_notice_str, (sl=strlen(clab_notice_str)));
    pb += sl;
    inttoascii(smsg->notice.code, tbuf, BASE10);
    memcpy(pb, tbuf, (sl=strlen(tbuf)));
    pb += sl;
    add_space(pb); pb++;
    add_quote(pb); pb++;
    texts = ermi_c_rtsp_announcecode2text(smsg->notice.code);
    memcpy(pb, texts, (sl=strlen(texts)));
    pb += sl;
    add_quote(pb); pb++;
    add_space(pb); pb++;
    memcpy(pb, eventdate_str, (sl=strlen(eventdate_str)));
    pb += sl;
    timestr = get_datetime_string();
    memcpy(pb, timestr, (sl=strlen(timestr)));
    pb += sl;
    add_space(pb); pb++;
    memcpy(pb, npt_str, (sl=strlen(npt_str)));
    pb += sl;
    inttoascii(smsg->notice.npt, tbuf, BASE10);
    memcpy(pb, tbuf, (sl=strlen(tbuf)));
    pb += sl;

    add_eol(pb);
    pb += 2;

    *next = pb;
}


#ifdef NK_NOT_USED
static void add_sessionlistreq (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    int sl;
    char *mp;

    /* First add the Content-Type and Content-Length headers */
    add_contenttype(smsg, pb, &mp);
    pb = mp;
    add_contentlength(strlen(clab_seslistreq_str), pb, &mp);
    pb = mp;
      
    memcpy(pb, clab_seslistreq_str, (sl=strlen(clab_seslistreq_str)));
    pb += sl;
    add_eol(pb);
    pb += 2;

    *next = pb;
}
#endif // NK_NOT_USED


static void add_transport (rtsp_msg_t *smsg, char *buf, char **next)
{
    char *pb = buf;
    char *sp;
    int sl;
    int i;
    transhdr_t *tspec;

    memcpy(pb, Transport_str, (sl=strlen(Transport_str)));
    pb += sl;

    for (i = 0; i < smsg->num_trans_hdrs; i++) {

	if (i > 0) {
	    /* add a comma between transport specs */
	    *pb = ',';
	    pb++;
	}

	tspec = &(smsg->transhdr[i]);

	if (tspec->type == MPEG_QAM) {
	    sp = MPEG_QAM_str;
	} else if (tspec->type == MPEG_UDP) {
	    sp = MPEG_UDP_str;
	} else {
	    ERMI_C_PRSERROR_BUGINF("\nBad Transport type <%d>", tspec->type);
            continue;
	}
	memcpy(pb, sp, (sl=strlen(sp)));
	pb += sl;

        /* Specify Stream type, for UDP Transport Headers */
	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_STREAM_TYPE)) {
            if (tspec->unicast == TRUE) {
	        sp = unicast_str;
	    } else {
	        sp = multicast_str;
	    }
	    memcpy(pb, sp, (sl=strlen(sp)));
	    pb += sl;
        }

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_BIT_RATE)) {
	    memcpy(pb, bit_rate_str, (sl=strlen(bit_rate_str)));
	    pb += sl;
	    inttoascii(tspec->bit_rate, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_SRC)) {
	    memcpy(pb, source_addr_str, (sl=strlen(source_addr_str)));
	    pb += sl;
	    iptoascii(tspec->src, tbuf);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_DEST)) {
	    memcpy(pb, destination_addr_str, (sl=strlen(destination_addr_str)));
	    pb += sl;
	    iptoascii(tspec->dest, tbuf);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_DEST_PORT)) {
	    memcpy(pb, dest_port_str, (sl=strlen(dest_port_str)));
	    pb += sl;
	    inttoascii(tspec->dest_port, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_SRC_PORT)) {
	    memcpy(pb, src_port_str, (sl=strlen(src_port_str)));
	    pb += sl;
	    inttoascii(tspec->src_port, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_MCAST_ADDR)) {
	    memcpy(pb, mcast_addr_str, (sl=strlen(mcast_addr_str)));
	    pb += sl;
	    iptoascii(tspec->mcast_address, tbuf);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_RANK)) {
	    memcpy(pb, rank_str, (sl=strlen(rank_str)));
	    pb += sl;
	    inttoascii(tspec->rank, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_MPTS_PROGRAM)) {
	    memcpy(pb, mpts_program_str, (sl=strlen(mpts_program_str)));
	    pb += sl;
	    inttoascii(tspec->mpts_program, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_FREQUENCY)) {
	    memcpy(pb, qamdestination_str, (sl=strlen(qamdestination_str)));
	    pb += sl;
	    inttoascii(tspec->qam_frequency, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	    *pb++ = '.';
	    inttoascii(tspec->qam_prognum, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

	if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_QAM_NAME)) {
	    memcpy(pb, qam_name_str, (sl=strlen(qam_name_str)));
	    pb += sl;
	    memcpy(pb, tspec->qam_name, (sl=strlen(tspec->qam_name)));
	    pb += sl;
	} else if (RPM_TESTBIT(tspec->tvalid, RP_BIT_TRANS_QAM_TSID)) {
	    memcpy(pb, tsid_str, (sl=strlen(tsid_str)));
	    pb += sl;
	    inttoascii(tspec->tsid, tbuf, BASE10);
	    memcpy(pb, tbuf, (sl=strlen(tbuf)));
	    pb += sl;
	}

    }/*for*/
	
    add_eol(pb);
    pb += 2;

    *next = pb;
}

/*
 * Functions to encode DOCSIS QAM related attributes and headers.
 * DOCSIS specific ERMI headers encode and parser functions.
 */
// TBD-Added


/*
 * Functions to encode RTSP Messages.
 */
static ermi_status
ermi_encode_rtsp_resp (rtsp_msg_t *smsg, char *buf, char **next)
{
    add_resp_header(smsg, buf, next);
    add_cseq(smsg, (*next), next);

    if (RPM_TESTBIT(smsg->valid, RP_BIT_SESSIONID)) {
        add_session(smsg, (*next), next);
    }

    if (smsg->num_trans_hdrs > 0) {
        add_transport(smsg, (*next), next);
    }

    if (RPM_TESTBIT(smsg->valid, RP_BIT_CLIENTSESSIONID)) {
	add_clientsessionid(smsg, (*next), next);
    }

    if (smsg->content.ptr) {
	/* Add Content-Type and Content-Length headers */
	add_contenttype(smsg, (*next), next);
	add_contentlength(smsg->content.len, (*next), next);
    }

    add_eol(*next);
    *next += 2;

    /* If there's content in the message, it goes in at the end */
    if (smsg->content.ptr) {
	memcpy(*next, smsg->content.ptr, smsg->content.len);
	*next += smsg->content.len;
    }
    
    return ERMI_OK;
}

#ifdef NK_NOT_USED
static ermi_status ermi_encode_rtsp_setup (rtsp_msg_t *smsg, char *buf,
                                           char **next)
{
    add_message_header(smsg, buf, next);
    add_cseq(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_FTAG))
	add_require(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_SESSIONID))
	add_session(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_CLIENTSESSIONID))
	add_clientsessionid(smsg, (*next), next);
    if (smsg->num_trans_hdrs > 0)
	add_transport(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_SESSGRP))
	add_session_group(smsg, (*next), next);

    add_eol(*next);
    *next += 2;

    return ERMI_OK;
}

static ermi_status
ermi_encode_rtsp_teardown (rtsp_msg_t *smsg, char *buf, char **next)
{
    add_message_header(smsg, buf, next);
    add_cseq(smsg, (*next), next);
    add_session(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_FTAG))
	add_require(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_CLIENTSESSIONID))
	add_clientsessionid(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_REASON))
	add_reason(smsg, (*next), next);

    add_eol(*next);
    *next += 2;

    return ERMI_OK;
}


static ermi_status
ermi_encode_rtsp_getparameter (rtsp_msg_t *smsg, char *buf, char **next)
{
    add_message_header(smsg, buf, next);
    add_cseq(smsg, (*next), next);

    if (RPM_TESTBIT(smsg->valid, RP_BIT_SESSGRP))
	add_session_group(smsg, (*next), next);

    add_sessionlistreq(smsg, (*next), next);

    add_eol(*next);
    *next += 2;

    return ERMI_OK;
}


static ermi_status
ermi_encode_rtsp_setparameter (rtsp_msg_t *smsg, char *buf, 
		               char **next, boolean add_body)
{
    add_message_header(smsg, buf, next);
    add_cseq(smsg, (*next), next);
    add_contenttype(smsg, (*next), next);
    add_session(smsg, (*next), next);

    if (add_body) {
        add_contentlength(30, (*next), next); /* TODO compute correct length */
    }

    add_eol(*next);
    *next += 2;

    return ERMI_OK;
}
#endif // NK_NOT_USED


static ermi_status
ermi_encode_rtsp_announce (rtsp_msg_t *smsg, char *buf, char **next)
{
    add_message_header(smsg, buf, next);
    add_cseq(smsg, (*next), next);

    if (RPM_TESTBIT(smsg->valid, RP_BIT_FTAG))
	add_require(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_SESSIONID))
	add_session(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_NOTICE))
	add_notice(smsg, (*next), next);
    if (RPM_TESTBIT(smsg->valid, RP_BIT_CLIENTSESSIONID))
	add_clientsessionid(smsg, (*next), next);

    add_eol(*next);
    *next += 2;

    return ERMI_OK;
}


/*
 * Build a text-form RTSP Message from the binary.
 * 
 *  INPUTS:
 *		rtsp_msg : binary  message
 *              buf      : pointer to buffer in which to build text message
 *		buflen   : maximum size of 'buf'
 *
 *  OUTPUTS:
 *		nbytes: number of bytes written into buffer

 *  RETURN: 
 *		ERMI_OK - encoded the  message without errors
 *		ERMI_PARSE_ERROR - encoded the message with errors
 *		ERMI_PARAM_ERROR - did not recognize message; 
 *			          did not encode anything.
 *		ERMI_MEM_ERROR - text message is too long to fit in 'buf'.
 *				buf may contain incomplete message.
 *  NOTES: 
 *      Caller must allocate  a buffer that is large enough to hold the 
 *      text msg.
 *	Caller, please be conservative. 
 *	Yes, this function does some error-checking, and prints out an error 
 *	message if buffer is too small, but note that this happens AFTER the
 *	buffer overflows. 
 *      Why doesn't this function do better error-checking and 
 *	error-prevention? Because (i) that will cost performance, and (ii) it
 *      isn't necessary if the caller gives us a properly-sized buffer.
 */
ermi_status ermi_c_parser_rtsp_encode (rtsp_msg_t *rtsp_msg, char *buf, 
				       uint32 buflen, uint32 *nbytes)
{
    char *next;
    ermi_status ret;
    char *msg_type_str;

    switch (rtsp_msg->msg_type) {

    case RTSP_RESPONSE:
	ret = ermi_encode_rtsp_resp(rtsp_msg, buf, &next);
	msg_type_str = rtsp_response_str;
	break;

    case RTSP_ANNOUNCE:
	ret = ermi_encode_rtsp_announce(rtsp_msg, buf, &next);
	msg_type_str = rtsp_announce_str;
	break;

    default:
	ERMI_C_PRSERROR_BUGINF("\n %s: Unrecognized message type %d",
			       __FUNCTION__, rtsp_msg->msg_type);
	*nbytes = 0;
	return ERMI_PARAM_ERROR;
    }

    *nbytes = (next - buf);

    if ((*nbytes) > buflen) {
	errmsg(&msgsym(SWERR, RTSP), 
	       "Encoder ERROR. text msg can't fit in %d bytes", buflen);
	ret = ERMI_MEM_ERROR;
    }

    if (ermi_c_debug_parser) {

	*next = NUL; /* Add NUL-terminator to print the string for debug */

	buginf("\nOUT RTSP Message (%d Bytes):\n%s", *nbytes, buf);

    } else {

        ERMI_C_PRSEBRIEF_BUGINF("\n OUT RTSP %s CSeq %d", 
			        msg_type_str, rtsp_msg->cseq); 
    }

    return ret;
}

/***************   Test Code ******************/


#define STATIC // static

STATIC char test_buf_setup_uni[] = "SETUP rtsp://192.168.12.164 RTSP/1.0\r\nCSeq: 4988\r\nRequire: com.cablelabs.ermi\r\nTransport: clab-MP2T/DVBC/QAM;qam_name=NK_QAM_HFC.2;qam_destination=906000000.1013,clab-MP2T/DVBC/UDP;unicast;bit_rate=15000000;destination=192.168.12.164;destination_port=50000\r\nclab-ClientSessionId: 00123f2a754300000050\r\n\r\n";

#ifdef NK_NOT_USED
STATIC char test_buf_getparam[] = "GET_PARAMETER rtsp://192.168.12.164 RTSP/1.0\r\nCSeq: 4973\r\nRequire: com.cablelabs.ermi\r\nContent-Type: text/parameters\r\nContent-Length: 21\r\nclab-session-list\r\n\r\n";

STATIC char test_buf_teardown[] = "TEARDOWN rtsp://192.168.12.164 RTSP/1.0\r\nCSeq: 4990\r\nRequire: com.cablelabs.ermi\r\nSession: 33\r\nclab-Reason: 200 \"User stop\"\r\nclab-ClientSessionId: 00123f2a75430000004e\r\n\r\n";

STATIC char test_buf_setparam[] = "SET_PARAMETER rtsp://192.168.12.164 RTSP/1.0\r\nCSeq: 4989\r\nRequire: com.cablelabs.ermi\r\n\r\n";

/* For ANNOUNCE Response only */
STATIC char test_buf_resp[] = "RTSP/1.0 200 OK\r\nCSeq: 4991\r\n\r\n";

STATIC char test_buf_setup_multi[] = "SETUP rtsp://192.168.12.164 RTSP/1.0\r\nCSeq: 4990\r\nRequire: com.cablelabs.ermi\r\nclab-PidRemap:0\r\nTransport: clab-MP2T/DVBC/QAM;qam_name=NK_QAM_HFC.2;qam_destination=906000000.62351,clab-MP2T/DVBC/UDP;multicast;bit_rate=200000;destination=192.168.12.164;multicast_address=232.1.1.1;destination_port=55000;mpts-program=62351;source=192.168.12.164;rank=1,clab-MP2T/DVBC/UDP;multicast;bit_rate=200000;destination=192.168.12.164;multicast_address=232.1.1.1;destination_port=55000;mpts-program=62351;source=10.90.146.164;rank=2\r\nclab-ClientSessionId: 00123f2a754300000051\r\n\r\n";
#endif // NK_NOT_USED

#ifdef NK_NOT_USED
STATIC char test_rbuf[] = "RTSP/1.0 200 OK\r\nCSeq: 315\r\n\r\nRTSP/1.0 300 NotSoGood\r\nCSeq: 316\r\nDate: 23 Oct 2008 15:35:06 GMT\r\n\r\nSETUP RTSP://erm_ip RTSP/1.0\r\nCSeq: 319\r\nRequire: com.cablelabs.ermi\r\nTransport: clab-MP2T/DVBC/QAM;unicast;bit_rate=2700000;tsid=5,clab-MP2T/DVBC/QAM;unicast;bit_rate=2700000;tsid=10,clab-MP2T/DVBC/QAM;unicast;bit_rate=2700000;tsid=20,clab-MP2T/DVBC/QAM;unicast;qam_destination=550000000.15; bit_rate=2700000;tsid=1234\r\nclab-ClientSessionId: 123456\r\n\r\nTEARDOWN\r\n\r\n";
#endif // NK_NOT_USED

STATIC char test_buf_end = '\0';

static char test_sbuf[1000];
static int testen = 0;

static int test_parser = 0; /* for DEBUG only. TODO remove from code */

void video_rtsp_parser_test (void)
{
    int count=1; /* to start up the while loop */
    ermi_status ret;
    char *recv_buf;
    rtsp_msg_t smsg;
    uint32 size;
    ermi_c_conn_t *conn;
    ermi_c_peer_t *peer;

    if (!test_parser) {
        return;
    }

    peer = ermi_c_get_peer_erm_info(NULL);

    conn = &peer->conn;
    if (!conn) {
        ERMI_C_PARSER_BUGINF("\n %s(): conn = NULL", __FUNCTION__);
    }

    // recv_buf = test_buf_teardown;
    // recv_buf = test_buf_getparam;
    // recv_buf = test_buf_setparam;
    // recv_buf = test_buf_resp;
    // recv_buf = test_rbuf;
    recv_buf = test_buf_setup_uni;

    while (count > 0) {
        ret = ermi_c_rtsp_parse_message(recv_buf, strlen(recv_buf),
                                        &smsg, &count);

	if (ret == ERMI_PARSE_INCOMPLETE_MSG) {
	    ERMI_C_PARSER_BUGINF("\nIncomplete message %s", recv_buf);
	    return;
	} 

	if (ret == ERMI_OK) {
	    ERMI_C_PARSER_BUGINF("\n Parsed one message successfully."
				 " count = %d", count);
	}

        // rtsp_svr_sock_accept(conn, RTSP_SVR_CONN);
        // ermi_c_rtsp_svr_process_msg(conn, &smsg);

	if (testen) {
	    ermi_c_parser_rtsp_encode(&smsg, test_sbuf, 1000, &size);
	}

	recv_buf += count;
    } /* while */

    ERMI_C_PARSER_BUGINF("\n\n *** Parser buffer completely processed ***");
}

