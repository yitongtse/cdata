/*
 * Copyright 2001, QNX Software Systems Ltd. All Rights Reserved
 *
 * This source code has been published by QNX Software Systems Ltd. (QSSL).
 * However, any use, reproduction, modification, distribution or transfer of
 * this software, or any software which includes or is based upon any of this
 * code, is only permitted under the terms of the QNX Open Community License
 * version 1.0 (see licensing.qnx.com for details) or as otherwise expressly
 * authorized by a written license agreement from QSSL. For more information,
 * please email licensing@qnx.com.
 */
#include <string.h>
#include <errno.h>
#include <devctl.h>
#include <syslog.h>
#include <stddef.h>
#include <stdarg.h>
#include <unix.h>
#include <sys/ioctl.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>
#include <net/if.h>
#include <netinet/in.h>
#include "ip_compat.h"
#include "ip_fil.h"
#include "ip_state.h"
#include "ip_nat.h"
#include "ip_auth.h"

/* this overwrite the orignal ioctl() (by -Dioctl=devctl in Makefile).
 * so that we have to match the ioctl() protocal define in sys/ioctl.h
 */
int ipf_devctl(int fd, int cmd, ...)
{
	int sbytes, rbytes;
	void *data, *dp;
	va_list v;
	union {
		struct _io_devctl       s;
		struct _io_devctl_reply r;
	} msg;
	iov_t   sx[2], rx[2];
	int sparts, rparts;

	sbytes = rbytes = 0;
	va_start(v, cmd);
	data = va_arg(v, void *);
	va_end(v);

	switch(cmd) {
#ifdef IPFILTER_AUTH		
	  case SIOCATHST:
		rbytes = sizeof(fr_authstat_t);
		break;
#endif

	  case SIOCINAFR:
	  case SIOCZRLST:
	  case SIOCINIFR:
	  case SIOCRMIFR:
	  case SIOCADIFR:
		sbytes = sizeof(struct frentry);
		break;

	  case SIOCGETFS:
		rbytes = max(sizeof(ips_stat_t), sizeof(friostat_t));
		break;

	  case SIOCFRZST:
		rbytes = sizeof(friostat_t);
		break;

	  case SIOCGFRST:
		rbytes = sizeof(friostat_t);
		break;

		/* NAT and AFR share the same stupid id !!!
		 case SIOCADNAT:
		 case SIOCRMNAT:
		 */

	  case SIOCRMAFR:
	  case SIOCADAFR:
		sbytes = max(sizeof(ipnat_t), sizeof(struct frentry));
		break;

	  case SIOCGNATS:
		rbytes = sizeof(natstat_t);
		break;

	  case SIOCGNATL:
		sbytes = rbytes = sizeof(natlookup_t);
		break;

	  case SIOCSTPUT:
	  case SIOCSTGET:
		sbytes = rbytes = sizeof(nat_save_t);
		break;

	  case SIOCSTGSZ:
		sbytes = rbytes = sizeof(natget_t);
		dp = data;
		data = &dp;
		break;

#if 0
		/* these should be handled by normal ioctl(). Note the data
		 * pointer here is not a (void **)
		 */
	  case SIOCGETFF:
	  case SIOCSWAPA:
	  case SIOCSTLCK:
	  case SIOCIPFFB:
		rbytes = sizeof(u_int);
		break;

	  case SIOCFRENB:
	  case SIOCSETFF:
	  case SIOCFRSYN:
		sbytes = sizeof(u_int);
		break;

	  case SIOCIPFFL:
		sbytes = rbytes = sizeof(u_int);
		break;
#endif
	  default:
		{
			uint32_t inout = cmd & IOC_DIRMASK;
			
			if (inout & IOC_IN)
			  sbytes = IOCPARM_LEN(cmd);
			if (inout & IOC_OUT)
			  rbytes = IOCPARM_LEN(cmd);
			dp = data;
			data = &dp;
		}
		break;
	}

	msg.s.type        = _IO_DEVCTL;
	msg.s.combine_len = 0;
	msg.s.dcmd        = cmd;
	msg.s.nbytes      = sbytes;

	sparts = rparts = 1;
	SETIOV(&sx[0], &msg, sizeof(msg.s));
	if (sbytes) {
		SETIOV(&sx[1], *(void **)data, sbytes);
		sparts++;
	}
	SETIOV(&rx[0], &msg, sizeof(msg.r));
	if (rbytes) {
		SETIOV(&rx[1], *(void **)data, rbytes);
		rparts++;
	}

	if (MsgSendv(fd, sx, sparts, rx, rparts) == -1)
	  return errno;

	return EOK;
}
