#include <sys/io-net.h>
#include <net/if_ether.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <atomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include "dinky.h"


int dinky_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface);
int dinky_rx_down(npkt_t *npkt, void *rx_down_hdl);
int dinky_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl);
int dinky_shutdown1(int registrant_hdl, void *func_hdl);
int dinky_shutdown2(int registrant_hdl, void *func_hdl);
int dinky_devctl(void *hdl, int dcmd, void *data, size_t size, int *ret);

io_net_registrant_funcs_t dinky_funcs = {
	8,
	dinky_rx_up,
	dinky_rx_down,
	dinky_tx_done,
	dinky_shutdown1,
	dinky_shutdown2,
	NULL,                    //advertise_ifaces
	dinky_devctl,
	NULL,                    //flush
	NULL                     //raw_open
};

io_net_registrant_t dinky_reg = {_REG_FILTER_ABOVE, "nfm-dinky.so", "en", "en", NULL, &dinky_funcs, 0};



int dinky_entry(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options);

/* io-net does a dlsym on "io_net_dll_entry" so don't change this name */
io_net_dll_entry_t io_net_dll_entry = {
	2,
	dinky_entry,
	NULL
};

int
dinky_entry(void *dll_hdl, dispatch_t *dpp, io_net_self_t *n, char *options)
{
	struct dinky_control *dc;

	if(!(dc = calloc(1, sizeof *dc)))
	{
		errno = ENOMEM;
		return -1;
	}

	dinky_reg.func_hdl = dc;
	
	dc->dropflag |= DROP_FLAG_ONWIRE;
	dc->ion = n;
	dc->dpp = dpp;
	if (pthread_mutex_init(&dc->irate.mutex, 0) != EOK ||
		pthread_mutex_init(&dc->orate.mutex, 0) != EOK)
	{
		return -1;
	}

	if (dc->ion->reg(dll_hdl, &dinky_reg, &dc->reg_hdl, NULL, &dc->endpoint) == -1)
	{
		return -1;
	}
	
	/* To receive all up headed packets */
	if(dc->ion->reg_byte_pat(dc->reg_hdl, 0, 0, NULL, _BYTE_PAT_ALL) == -1)
	{
		return -1;
	}
	
	return 0;
}



int
dinky_shutdown1(int registrant_hdl, void *func_hdl)
{
	/* Our last chance for _this_ thread to tx (this thread has exclusive access) */
	return 0;
}

int
dinky_shutdown2(int registrant_hdl, void *func_hdl)
{
	struct dinky_control *dc = func_hdl;
	
	/* We now have shared access.  This is where you would do most of your cleanup work */
	if (pthread_mutex_lock(&dc->irate.mutex) == EOK) {
		pthread_mutex_destroy(&dc->irate.mutex);
	}
	if (pthread_mutex_lock(&dc->orate.mutex) == EOK) {
		pthread_mutex_destroy(&dc->orate.mutex);
	}
	return 0;
}

static void dinky_setdrop(droprate_t *dr)
{
	unsigned int seed = time(NULL);
	unsigned *temp = alloca(dr->drop * sizeof(unsigned));
	int i,j;
	
	for (i = 0; i < dr->drop; i++) {
		temp[i] = (rand_r(&seed) * dr->total / RAND_MAX);
	}

	for (i = 0; i < dr->drop; i++) {
		int idx = 0;
		
		for (j = 0, dr->dropid[i] = dr->total; j < dr->drop; j++)
		{
			if (temp[j] < dr->dropid[i]) {
				dr->dropid[i] = temp[j];
				idx = j;
			}
		}
		temp[idx] = dr->total;
	}

	dr->dcount = dr->pcount = 0;
	return;
}

static int
dinky_drop(droprate_t *dr)
{	
	if (pthread_mutex_lock(&dr->mutex) != EOK)
	  return 0;
	
	if (dr->pcount == dr->total - 1)
	  dinky_setdrop(dr);

	if (dr->pcount++ == dr->dropid[dr->dcount]) {
		dr->dcount++;
		dr->pkt_dropped++;
		pthread_mutex_unlock(&dr->mutex);
		return 1;
	} else {
		pthread_mutex_unlock(&dr->mutex);
		return 0;
	}
}

static int
dinky_block(char *data, struct dinky_control *dc, int outbound)
{
	droprate_t *dr;
	
	dr = (outbound ? &dc->orate : &dc->irate);

	if (!dr->dropid) {
		return 0;
	}
	
	if ((dc->dropflag & DROP_FLAG_ARP) 
		&& (data[12] == 0x08) && (data[13] == 0x06))
	{
		return dinky_drop(dr);
	}
	
	if ((dc->dropflag & (DROP_FLAG_IP | DROP_FLAG_TCP | DROP_FLAG_UDP | DROP_FLAG_QNET))
		&& (data[12] == 0x08) && (data[13] == 0x00))
	{
		if (dc->dropflag & DROP_FLAG_IP) 
		{
			return dinky_drop(dr);
		}
		
		if ((dc->dropflag & DROP_FLAG_TCP) 
			&& data[offsetof(struct ip, ip_p) + 14] == IPPROTO_TCP)
		{
			return dinky_drop(dr);
		}
		
		if ((dc->dropflag & DROP_FLAG_UDP)
			&& data[offsetof(struct ip, ip_p) + 14] == IPPROTO_UDP)
		{
			return dinky_drop(dr);
		}
			
		if ((dc->dropflag & DROP_FLAG_QNET)
			&& data[offsetof(struct ip, ip_p) + 14] == IPPROTO_QNET)
		{
			return dinky_drop(dr);
		}
	}
	return 0;
}


int
dinky_rx_down(npkt_t *npkt, void *func_hdl)
{
	struct dinky_control *dc = func_hdl;
	char   data[sizeof(struct ether_header) + sizeof(struct ip)], *dp;

	if(npkt->flags & _NPKT_MSG)
	{
		/*
		 * It's a control packet of some sort.
		 * Pass it on
		 */
		return dc->ion->tx_down(dc->reg_hdl, npkt);
	}

	atomic_add(&dc->orate.pkt_all, 1);

	/* snapshot the data we need */
	if (npkt->tot_iov != 1) {
		iov_t iov;
		
		SETIOV(&iov, data, sizeof(data));
		if (dc->ion->memcpy_from_npkt(&iov, 1, 0, npkt, 0, npkt->framelen) != sizeof(data))
		{
			return dc->ion->tx_down(dc->reg_hdl, npkt);
		}
		dp = data;
	} else {
		dp = npkt->buffers.tqh_first->net_iov->iov_base;
	}

	/* See if packet fit drop rules.
	 * 
	 * If the "ONWIRE" flag is set, we drop packet silently,
	 * so the upper layer won't know the packet is dropped.
	 * (as if the packet disppared on wire). Otherwise,
	 * we return an error to upper layer, indicate that an
	 * error during tx (as if a hardware error cauing driver
	 * return error).
	 */
	if (dinky_block(dp, dc, 1)) {
		int ret;
		
		ret = (dc->dropflag & DROP_FLAG_ONWIRE) ? TX_DOWN_FAILED : TX_DOWN_OK;
		dc->ion->tx_done(dc->reg_hdl, npkt);
		return ret;
	}
	
	/* Then pass it on */
	return dc->ion->tx_down(dc->reg_hdl, npkt);
}

int
dinky_rx_up(npkt_t *npkt, void *func_hdl, int off, int len_sub, uint16_t cell, uint16_t endpoint, uint16_t iface)
{
	struct dinky_control *dc = func_hdl;
	char   data[sizeof(struct ether_header) + sizeof(struct ip)], *dp;
	
	if(npkt->flags & _NPKT_MSG)
	{
		/*
		 * It's a control packet of some sort.
		 * You can learn about interfaces coming and going from below you this way
		 * (see _IO_NET_MSG_DL_ADVERT and io_net_msg_dl_advert_t in <sys/io-net.h>).
		 * You have to handle duplicate adverts.  You get a npkt->flags & (_NPKT_MSG |_NPKT_MSG_DYING)
		 * when cell endpoint is going away.
		 */

		/* 
		 * It is possiable to remember the interface here, and give our user
		 * an option to only drop packet on specific interface.
		 *                                           -- xtang
		 */

		/*
		 * pass it on.
		 */
		if(dc->ion->tx_up(dc->reg_hdl, npkt, off, len_sub, cell, endpoint, iface) == 0)
		{
			/* noone above you took it */
			dc->ion->tx_done(dc->reg_hdl, npkt);
		}
		return 0;
	}

	atomic_add(&dc->irate.pkt_all, 1);
	/* snapshot the data we need */
	if (npkt->tot_iov != 1) {
		iov_t iov;
		
		SETIOV(&iov, data, sizeof(data));
		if (dc->ion->memcpy_from_npkt(&iov, 1, off, npkt, 0, npkt->framelen) != sizeof(data))
		{
			if(dc->ion->tx_up(dc->reg_hdl, npkt, off, len_sub, cell, endpoint, iface) == 0)
			{
				dc->ion->tx_done(dc->reg_hdl, npkt);
			}
			return 0;
		}
		dp = data;
	} else {
		dp = npkt->buffers.tqh_first->net_iov->iov_base;
	}

	/* See if the packet fit drop rules. */
	if (dinky_block(dp, dc, 0)) {
		return dc->ion->tx_done(dc->reg_hdl, npkt);
	}
	
	/* Pass it on */
	if(dc->ion->tx_up(dc->reg_hdl, npkt, off, len_sub, cell, endpoint, iface) == 0)
	{
		dc->ion->tx_done(dc->reg_hdl, npkt);
	}

	return 0;
}


int
dinky_tx_done(npkt_t *npkt, void *done_hdl, void *func_hdl)
{
	/*
	 * If you ever call dc->ion->reg_tx_done(dc->reg_hdl, npkt, done_hdl) (ever add something to
 	 * head or tail of down headed packet or ever originate your own up / down packet (don't think
	 * a sniffer would ever do this)), this func will be called when the packet is consumed so you
	 * can undo / reclaim what you did.  In this example it should never be called (we never call
	 * dc->ion->reg_tx_done()).
	 */

	return 0;
}

/*
 * Any devctl on /dev/io-net/en_enX where X is dc->endpoint (filters can be stacked)
 * will pop out here.
 */
#include <stdio.h>
int
dinky_devctl(void *hdl, int dcmd, void *data, size_t size, int *ret)
{
	struct dinky_control *dc = hdl;
	struct dinky_rate *drate;
	droprate_t *dr;

	*ret = EOK;
	switch (dcmd) {
	  case DCMD_DINKY_GETSTAT:
		memcpy((char *)data, (char *)&dc->irate, size);
		break;

	  case DCMD_DINKY_SETFLAG:
		dc->dropflag = *(unsigned *)data;
		break;
		
	  case DCMD_DINKY_SETIRATE:
	  case DCMD_DINKY_SETORATE:
	  case DCMD_DINKY_SETRATE:
		
		drate = (struct dinky_rate *)data;
		if (drate->drop > drate->total) {
			*ret = EINVAL;
			break;
		}
		
		if (dcmd == DCMD_DINKY_SETIRATE || dcmd == DCMD_DINKY_SETRATE) {
			dr = &dc->irate;
			
			if ((*ret = pthread_mutex_lock(&dr->mutex)) != EOK) {
				break;
			}

			if (dr->drop != drate->drop &&
				(dr->dropid = realloc(dr->dropid, drate->drop * sizeof(unsigned))) != NULL)
			{
				dr->total = drate->total;
				dr->drop  = drate->drop;
				dr->pkt_all = dr->pkt_dropped = 0;
				dinky_setdrop(dr);
			} else {
				*ret = ENOMEM;
			}
			pthread_mutex_unlock(&dr->mutex);
		}
		
		if (dcmd == DCMD_DINKY_SETORATE || dcmd == DCMD_DINKY_SETRATE) {
			dr = &dc->orate;
			
			if ((*ret = pthread_mutex_lock(&dr->mutex)) != EOK) {
				break;
			}

			if (dr->drop != drate->drop &&
				(dr->dropid = realloc(dr->dropid, drate->drop * sizeof(unsigned))) != NULL)
			{
				dr->total = drate->total;
				dr->drop  = drate->drop;
				dr->pkt_all = dr->pkt_dropped = 0;
				dinky_setdrop(dr);
			} else {
				*ret = ENOMEM;
			}
			pthread_mutex_unlock(&dr->mutex);
		}
		break;
		
	  default:
		*ret = ENOTSUP;
	}

	
	return *ret == EOK ? 0 : -1;
}

