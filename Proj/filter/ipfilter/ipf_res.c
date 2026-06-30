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
#include <sys/neutrino.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <unix.h>
#include <devctl.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/if_vp.h>
#include <netinet/ip.h>
#include <netinet/ip_compat.h>
#include <netinet/ip_fil.h>
#include <netinet/ip_nat.h>
#include <netinet/ip_state.h>
#include <netinet/ip_auth.h>

#include "allocator.h"

/*
 * errno is undef'd in qnx.h, since stack source use errno as a function paramater,
 * we define it here for own use.
 */
#define errno (*__get_errno_ptr())

/* 
 * free is a macro define in "sys/malloc.h", let's undef it
 */
#ifdef free
#undef free
#endif

struct ipfres_ocb {
	iofunc_ocb_t       io_ocb;
	int                unit;
	mode_t             mode;
	int                rcvid;
	int                rbytes;
};

/* This maps the IPL_LOG defininition in ip_fil.h, which is:
 *      #define IPL_LOGIPF      0
 *      #define IPL_LOGNAT      1
 *      #define IPL_LOGSTATE    2
 *      #define IPL_LOGAUTH     3
 *      #define IPL_LOGMAX      3
 */

enum {
	DEV_IPL,
	DEV_IPNAT,
	DEV_IPSTATE,
	DEV_IPAUTH,
	NUM_DEVS
};
	
extern filterstats_t frstats[];
extern int ipflog_unblock(struct ipfres_ocb *ocb);
extern int fr_auth_unblock(int rcvid);

static char *ipl_name[NUM_DEVS] = {
	IPL_NAME,
	IPL_NAT,
	IPL_STATE,
	IPL_AUTH
};

static	resmgr_connect_funcs_t  ipf_connectfuncs;
static	resmgr_io_funcs_t       ipf_iofuncs;
static	iofunc_attr_t           ipf_ioattr;
static  int                     ipf_pathid[NUM_DEVS];

/*
 * routines below for saving IP headers to buffer
 */
static int ipfres_open(resmgr_context_t *ctp, io_open_t *msg, void *handle, void *extra)
{
	struct ipfres_ocb    *ocb;
	int                  devid, ret;
	struct  _client_info cinfo;
	static const mode_t  perm_mode[3] = {S_IREAD, S_IWRITE, S_IREAD | S_IWRITE};
	static const mode_t  acce_mode[3] = {FREAD, FWRITE, FREAD | FWRITE};
	mode_t               mode;

	devid = (int)handle;

	if (devid >= NUM_DEVS) {
		return _RESMGR_ERRNO(EBADF);
	}
	
	/* get client info */
	if ((ret = ConnectClientInfo_r(ctp->info.scoid, &cinfo, 0)) != EOK) {
		return _RESMGR_ERRNO(ret);
	}

	mode = perm_mode[(msg->connect.ioflag & O_ACCMODE) - 1];
	if ((ret = iofunc_check_access(ctp, &ipf_ioattr, mode , &cinfo)) != EOK) {
		return _RESMGR_ERRNO(ret);
	}
	
	if ((ocb = stdcalloc(1, sizeof(struct ipfres_ocb))) == NULL)
	{
		return _RESMGR_ERRNO(EBADF);
	}

	if ((ret = iofunc_ocb_attach(ctp, msg, (void *)ocb, &ipf_ioattr, &ipf_iofuncs)) == EOK) {
		ocb->mode = acce_mode[(msg->connect.ioflag & O_ACCMODE) - 1];
		ocb->unit = devid;
		return _RESMGR_ERRNO(EOK);
	} else {
		return _RESMGR_ERRNO(ret);
	}
}

static int ipfres_close(resmgr_context_t *ctp, void *reserved, void *ocb)
{
	iofunc_ocb_detach(ctp, ocb);
	stdfree(ocb);
	return _RESMGR_ERRNO(EOK);
}

static int ipfres_devctl(resmgr_context_t *ctp, io_devctl_t *msg, void *o)
{
	struct ipfres_ocb *ocb = o;
	void *data = _DEVCTL_DATA(msg->i);
	int32_t dcmd = msg->i.dcmd;

	if (ocb->unit < 0 || ocb->unit > NUM_DEVS) {
		return _RESMGR_ERRNO(msg->o.ret_val = EINVAL);
	}

	msg->o.ret_val = iplioctl(ocb->unit, dcmd, data, ocb->mode, curproc);

	switch (dcmd) {
#ifdef IPFILTER_AUTH
	  case SIOCATHST:
		MsgWrite(curproc->p_ctp.rcvid, data, sizeof(fr_authstat_t), sizeof(msg->o));
		break;
#endif
	  case SIOCGNATS:
		MsgWrite(curproc->p_ctp.rcvid, data, sizeof(natstat_t), sizeof(msg->o));
		break;
	  case SIOCGNATL:
		MsgWrite(curproc->p_ctp.rcvid, data, sizeof(natlookup_t), sizeof(msg->o));
		break;
	  case SIOCSTPUT:
	  case SIOCSTGET:
		MsgWrite(curproc->p_ctp.rcvid, data, sizeof(nat_save_t), sizeof(msg->o));
		break;
	  case SIOCSTGSZ:
		MsgWrite(curproc->p_ctp.rcvid, data, sizeof(natget_t), sizeof(msg->o));
		break;
	  case SIOCGETFS:
		if (ocb->unit == DEV_IPL) 
		  MsgWrite(curproc->p_ctp.rcvid, data, sizeof(friostat_t), sizeof(msg->o));
		else if (ocb->unit == DEV_IPSTATE)
		  MsgWrite(curproc->p_ctp.rcvid, data, sizeof(ips_stat_t), sizeof(msg->o));
		break;
	  case SIOCFRZST:
	  case SIOCGFRST:
		MsgWrite(curproc->p_ctp.rcvid, data, sizeof(friostat_t), sizeof(msg->o));
		break;
	  default:
		if (((dcmd & IOC_DIRMASK) & IOC_OUT) != 0) 
		  MsgWrite(curproc->p_ctp.rcvid, data, IOCPARM_LEN(dcmd), sizeof(msg->o));
		break;
	}
	MsgReply(curproc->p_ctp.rcvid, msg->o.ret_val, &msg->o, sizeof(msg->o));
	return _RESMGR_NOREPLY;
}

static int ipfres_read(resmgr_context_t *ctp, io_read_t *msg, void *o)
{
	extern  int ipflog_read __P((minor_t, struct uio *));
	struct ipfres_ocb    *ocb = o;
	int status;
	struct iovec iov;
	struct uio uio;

	iov.iov_base = 0;
	iov.iov_len = uio.uio_resid = msg->i.nbytes;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = UIO_USERSPACE;
	uio.uio_rw = UIO_READ;
	uio.uio_procp = curproc;
	curproc->p_offset         = 0;
	
	if ((status =  ipflog_read(ocb->unit, &uio)) != 0)
	{
		if (uio.uio_resid != msg->i.nbytes && 
			(status == ERESTART || status == EINTR || status == EWOULDBLOCK))
		{
			status = EOK;
		}
	}
	
	if (status)
	  MsgError(ctp->rcvid, status);
	else
	  MsgReplyv(ctp->rcvid, msg->i.nbytes - uio.uio_resid, 0, 0);
	
	return _RESMGR_NOREPLY;
}

static int ipfres_unblock(resmgr_context_t *ctp, io_pulse_t *pulse, void *ocb)
{
#if 0
	switch (((struct ipfres_ocb *)ocb)->unit) {
	  case DEV_IPL:
		return ipflog_unblock(ocb);
	  case DEV_IPAUTH:
		return fr_auth_unblock(ctp->rcvid);
	}
#endif
	return _RESMGR_ERRNO(EINVAL);
}

void ipfres_destroy(dispatch_t *dpp)
{
	int i;
	
	for (i = 0; i < NUM_DEVS; i++) {
		if (ipf_pathid[i] != -1)
		  resmgr_detach(dpp, ipf_pathid[i], _RESMGR_DETACH_ALL);
	}
	return;
}

int ipfres_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options)
{
	int i;

	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &ipf_connectfuncs, _RESMGR_IO_NFUNCS, &ipf_iofuncs);
	iofunc_attr_init(&ipf_ioattr, 0600 | S_IFCHR, 0, 0);

	ipf_connectfuncs.open = ipfres_open;
	ipf_iofuncs.read      = ipfres_read;
	ipf_iofuncs.close_ocb = ipfres_close;
	ipf_iofuncs.devctl    = ipfres_devctl;
	ipf_iofuncs.unblock   = ipfres_unblock;
	
	memset(ipf_pathid, -1, sizeof(ipf_pathid));

	/* Attach device numbers to our I/O manager */
	for (i = 0; i < NUM_DEVS; i++) {
		if ((ipf_pathid[i] = resmgr_attach(dpp, NULL, ipl_name[i], _FTYPE_ANY, 0, &ipf_connectfuncs, &ipf_iofuncs, (void *)i)) == -1)
		{
			int err = errno;
			ipfres_destroy(dpp);
			errno = err;
			return -1;
		}
	}

	return 0;
}
