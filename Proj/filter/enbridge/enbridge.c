/*
 * $QNXLicenseC:  
 * Copyright 2005, QNX Software Systems. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software 
 * Systems (QSS) and its licensors.  Any use, reproduction, modification, 
 * disclosure, distribution or transfer of this software, or any software 
 * that includes or is based upon any of this code, is prohibited unless 
 * expressly authorized by QSS by written agreement.  For more information 
 * (including whether this source code file has been published) please
 * email licensing@qnx.com. $
*/


/*
 * An example ethernet bridge.
 * example invocation:
 *   mount -T io-net -o"en0,en1" nfm-en_bridge.so
 *
 * The above would forward all packets received on en0 out en1
 * and vice versa.
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/io-net.h>
#include <sys/slogcodes.h>
#include <arpa/inet.h>
#include <net/ethertypes.h>

typedef struct {
	int                reg_hdl;
	void               *dll_hdl;
	io_net_self_t      *ion;
	dispatch_t         *dpp;
	int                movers[2];
#ifdef SWITCH_VLAN
	uint16_t           vlantag[2];
#endif
} brg_ctrl_t;

/* global control structure */
brg_ctrl_t brg_ctrl;


static int brg_entry(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options);

io_net_dll_entry_t io_net_dll_entry = {
	2,
	brg_entry,
	NULL
};

static int brg_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface);
static int brg_rx_down(npkt_t *npkt, void *rx_down_hdl);
static int brg_shutdown1(int registrant_hdl, void *func_hdl);
static int brg_shutdown2(int registrant_hdl, void *func_hdl);

static io_net_registrant_funcs_t brg_funcs = {
	5,
	brg_rx_up,
	brg_rx_down,
	NULL,
	brg_shutdown1,
	brg_shutdown2
};

io_net_registrant_t brg_reg = {
	_REG_FILTER_ABOVE | _REG_NO_REMOUNT,
	"nfm-enbridge.so",
	"en", 
	"en", 
	&brg_ctrl, 
	&brg_funcs,
	0
};

static int
brg_entry(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options)
{
	brg_ctrl_t *bc = &brg_ctrl;
	char *value;
	int opt, i = 0;
	static char *brg_opts[] = {
		"vlan",
		NULL
	};
	
	bc->dll_hdl = dll_hdl;
	bc->dpp     = dpp;
	bc->ion     = n;

#ifdef SWITCH_VLAN
	bc->vlantag[0] = htons(0x100);
	bc->vlantag[1] = htons(0x200);
#endif
	bc->movers[0] = 0;
	bc->movers[1] = 1;
	while(options && *options != '\0' && i < 2)
	{
		opt = getsubopt(&options, brg_opts, &value);
		switch (opt)
		{
#ifdef SWITCH_VLAN
		  case 0:
			/* vlan=<tag> */
			bc->vlantag[i] = htons(strtol(value, NULL, 0));
			break;
#endif
		  case -1:
			if(value && value[0] == 'e' && value[1] == 'n' && isdigit(value[2]))
			{
				bc->movers[i] = strtol(&value[2], NULL, 0);
				i++;
			}
			break;
		  default:
			/* Should never happen */
			errno = EINVAL;
			return -1;
			break;
		}
	}

#if 0
	if(i != 2)
	{
		slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s: Not enough ethernet interfaces specified\n", our_name);
		errno = EINVAL;
		return -1;
	}
#endif					

	if(bc->ion->reg(bc->dll_hdl, &brg_reg, &bc->reg_hdl, NULL, NULL) == -1)
	{
		return -1;
	}

	if(bc->ion->reg_byte_pat(bc->reg_hdl, 0, 0, NULL, _BYTE_PAT_ALL) == -1)
	{
		int err = errno;

		bc->ion->dereg(bc->reg_hdl);
		errno = err;
		return -1;
	}

	return 0;
}

static int
brg_shutdown1(int registrant_hdl, void *func_hdl)
{
	return EOK;
}

static int
brg_shutdown2(int registrant_hdl, void *func_hdl)
{
	return EOK;
}

static int
brg_rx_down(npkt_t *npkt, void *func_hdl)
{
	brg_ctrl_t  *bc = func_hdl;

	return  bc->ion->tx_down(bc->reg_hdl, npkt);
}

static int
brg_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface)
{
	brg_ctrl_t *bc = func_hdl;
#ifdef SWITCH_VLAN
	uint16_t *wp;
#endif
	if(npkt->flags & _NPKT_MSG)
	{
		if(bc->ion->tx_up(bc->reg_hdl, npkt, off, len_sub, cell, endpoint, iface) == 0)
		{
			/* noone above you took it */
			bc->ion->tx_done(bc->reg_hdl, npkt);
		}
		return 0;
	}
	
#ifdef SWITCH_VLAN
	wp = (uint16_t*)(npkt->buffers.tqh_first->net_iov->iov_base + 12);
	if(endpoint == bc->movers[0] && (wp[0] == htons(ETHERTYPE_VLAN)) && (wp[1] == bc->vlantag[0]))
#else
	if(endpoint == bc->movers[0])
#endif
	{
		/*
		 * npkt->ref_cnt == 1, therefor doesn't need to be atomic.  Ethernet is the
		 * link layer and they all filter into us (as opposed to branching out to
		 * multiple protocols).
		 */
		npkt->flags &= ~_NPKT_UP;
		npkt->cell = cell;
		npkt->endpoint = bc->movers[1];
		npkt->iface = 0;
#ifdef SWITCH_VLAN
		wp[1] = bc->vlantag[1];
#endif
		/* If it fails, io-net calls tx_done */
		bc->ion->tx_down(bc->reg_hdl, npkt);
	}
#ifdef SWITCH_VLAN
	else if(endpoint == bc->movers[1] && (wp[0] == htons(ETHERTYPE_VLAN)) && (wp[1] == bc->vlantag[1]))
#else
	else if(endpoint == bc->movers[1])
#endif
	{
		npkt->flags &= ~_NPKT_UP;
		npkt->cell = cell;
		npkt->endpoint = bc->movers[0];
		npkt->iface = 0;
#ifdef SWITCH_VLAN
		wp[1] = bc->vlantag[0];
#endif
		bc->ion->tx_down(bc->reg_hdl, npkt);
	}
	else
	{
		if(bc->ion->tx_up(bc->reg_hdl, npkt, off, len_sub, cell, endpoint, iface) == 0)
		{
			/* noone above us took it */
			bc->ion->tx_done(bc->reg_hdl, npkt);
		}
	}

	return 0;
   
}

