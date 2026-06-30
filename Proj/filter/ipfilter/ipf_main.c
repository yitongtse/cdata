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
#include <sys/io-net.h>

#include <netinet/in.h>
#include "netinet/ip_compat.h"
#include "netinet/ip_fil.h"

/*
 * errno is undef'd in qnx.h, we define it here for own use.
 */
#define errno (*__get_errno_ptr())

/* 
 * free is a macro define in "sys/malloc.h", let's undef it
 */
#ifdef free
#undef free
#endif

static int ipf_entry(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options);
static int ipf_destroy(void *dll_hdl);
static int ipf_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface);
static int ipf_rx_down(npkt_t *npkt, void *rx_down_hdl);
static int ipf_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl);
static int ipf_shutdown1(int registrant_hdl, void *func_hdl);
static int ipf_shutdown2(int registrant_hdl, void *func_hdl);
static int ipf_dl_advert(int registrant_hdl, void *func_hdl);

extern int ipfres_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options);
extern void ipfres_destroy(dispatch_t *dpp);

/* we regist to io-net, just so we could be umount */
static io_net_registrant_funcs_t ipf_funcs = {
	6,
	ipf_rx_up,
	ipf_rx_down,
	ipf_tx_done,
	ipf_shutdown1,
    ipf_shutdown2,
	ipf_dl_advert
};

static io_net_registrant_t ipf_reg = {
	_REG_PRODUCER_UP | _REG_NO_REMOUNT | _REG_DEREG_ALL,
	"nfm-ipfilter.so",
	"ipfilter", 
	"ipfilter",
	NULL,
	&ipf_funcs,
	0
};

/* io-net entries */
io_net_dll_entry_t io_net_dll_entry = {
	2,
	ipf_entry,
	ipf_destroy
};

static  dispatch_t              *ipf_dpp;
static  io_net_self_t           *ipf_npi;
static  int                     ipf_hdl;

#if 0
static  char                    *ipf_options[] = {
	"mss-clamping",
	NULL
};
int mss_clamping = 0;
#endif

static int ipf_entry(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options)
{
	/* steal the stack dpp so we are in single thread. 
	 * Another alternative is make sure the resmgr code and the filter
	 * code are thread safe.
	 */
	extern struct proc *curproc;
	
	if (!curproc || !curproc->p_nglobs || !curproc->p_nglobs->dpp) {
		errno = EINVAL;
		return -1;
	}

#if 0
	while (options && *options != NULL) {
		char *value;
		int opt;
		
		opt = getsubopt(&options, ipf_options, &value);
		
		switch (opt)
		{
		  case 0:
			mss_clamping++;
			break;
		}
	}
#endif
	
	ipf_dpp = curproc->p_nglobs->dpp;
	ipf_npi = n;
	if ((errno = ipl_enable()) != 0)
	  return -1;
	
	if (ipfres_init(dll_hdl, ipf_dpp, n, options) == -1)
	  return -1;
	
	return n->reg(dll_hdl, &ipf_reg, &ipf_hdl, NULL, NULL);
}

static int ipf_destroy(void *dll_hdl)
{
	ipfres_destroy(ipf_dpp);
	ipl_disable();
	return 0;
}

static int ipf_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface) {
	return ipf_npi->tx_done(ipf_hdl, npkt);
}

static int ipf_rx_down(npkt_t *npkt, void *rx_down_hdl) {
	return ipf_npi->tx_done(ipf_hdl, npkt);
}

static int ipf_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl) {
	return EOK;
}

static int ipf_shutdown1(int registrant_hdl, void *func_hdl) {
	return EOK;
}

static int ipf_shutdown2(int registrant_hdl, void *func_hdl) {
	return EOK;
}

static int ipf_dl_advert(int registrant_hdl, void *func_hdl) {
	return EOK;
}

#if 0
int ipf_copyin(caddr_t to, caddr_t from, int size)
{
	extern struct proc *curproc;
	int ret = EOK;

	if (MsgRead(curproc->p_ctxt->rcvid, to, size, sizeof(struct _io_devctl)) == -1) {
	  ret = errno;
	  syslog(LOG_ERR, "MsgRead (Rcvid = %d, size = %d): %m", pid, size);
	}

	return ret;
}

int ipf_copyout(caddr_t from, caddr_t to, int size, pid_t pid)
{
	int ret = EOK;

	if (pid == 0)
	  memcpy(to, from, size);
	else if (MsgWrite(pid, from, size, sizeof(struct _io_devctl_reply)) == -1) {
		ret = errno;
		syslog(LOG_ERR, "MsgWrite (Rcvid = %d, size = %d): %m", pid, size);
	}

	return ret;
}
#endif
