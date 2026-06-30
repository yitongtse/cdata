/*
 * $QNXLicenseC:  
 * Copyright 2006, QNX Software Systems. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software 
 * Systems (QSS) and its licensors.  Any use, reproduction, modification, 
 * disclosure, distribution or transfer of this software, or any software 
 * that includes or is based upon any of this code, is prohibited unless 
 * expressly authorized by QSS by written agreement.  For more information 
 * (including whether this source code file has been published) please
 * email licensing@qnx.com. $
*/






 
#include "mpc85xx.h"

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

static int mpc_add_pkt (mpc85xx_t *ext, npkt_t *npkt, int idx)
{
	mpc_bd_t		*bd = &ext->rx_bd [idx];
	net_buf_t		*buf = TAILQ_FIRST (&npkt->buffers);
	net_iov_t		*iov = buf->net_iov;
	uint16_t        status;

	ext->rx_pktq [idx] = npkt;
	bd->buffer = iov->iov_phys;
	status = (RXBD_E | RXBD_I);
	if (idx == ext->num_rx_descriptors -1) {
	  status |= RXBD_W;
	}
	bd->length = 0;
	bd->status = status;

	return (EOK);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int mpc_receive_complete (npkt_t *npkt, void *done_hdl, void *func_hdl)
{
	mpc85xx_t		*ext = (mpc85xx_t *) done_hdl;
	net_buf_t		*buf;

	_mutex_lock (&ext->rx_free_pkt_q_mutex);

	if (ext->num_rx_free < ext->num_rx_descriptors) {
		npkt->next = ext->rx_free_pkt_q;
		ext->rx_free_pkt_q = npkt;
		ext->num_rx_free++;

		npkt->ref_cnt = 1;
		npkt->tot_iov = 1;
		npkt->req_complete = 0;
		npkt->flags = _NPKT_UP;
		npkt->iface = 0;

		buf = TAILQ_FIRST (&npkt->buffers);
		buf->net_iov->iov_len = MPC_MTU_SIZE;
		buf->niov = 1;
		_mutex_unlock (&ext->rx_free_pkt_q_mutex);
	} else {
		mpc85xx_pool_t *p = (mpc85xx_pool_t *)npkt->org_data;
		int ret = atomic_sub_value(&p->ref, 1);
		_mutex_unlock (&ext->rx_free_pkt_q_mutex);
		if (ret == 1) {
		  ion_free (p->buf_head);
		  npkt->org_data = NULL;
		}
		ion_free (npkt);
	}
	return (0);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

void mpc_receive (mpc85xx_t *ext)
{
	npkt_t					*npkt, *new;
	net_buf_t				*buf;
	int						cidx;
	uint16_t				status;
	mpc_bd_t				*rx_bd;
	npkt_done_t             *done;

	/* mask off the rx_interrupt */
	while (1) {
		cidx = ext->rx_cidx;
		rx_bd = &ext->rx_bd [cidx];
		status = rx_bd->status;
		if (status & RXBD_E) {
			break;
		}
		
		npkt = ext->rx_pktq [cidx];                
		ext->rx_pktq [cidx] = NULL;
		ext->rx_cidx = NEXT_RX (ext->rx_cidx);

		if (status & RXBD_ERR) {
			nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mpc85xx: status = %08x\n", status);			
			
			/* Return the packet to the receive queue.  No "receive
			 * complete" required since the packet wasn't used. */
			mpc_add_pkt (ext, npkt, cidx);

			continue;                        
		}

		/* Take a packet off the free queue to replace the packet
		 * we're about to send up to io-net */
		_mutex_lock (&ext->rx_free_pkt_q_mutex);
		if ((new = ext->rx_free_pkt_q)) {
			ext->rx_free_pkt_q = new->next;
			new->next = NULL;
			ext->num_rx_free--;
			_mutex_unlock (&ext->rx_free_pkt_q_mutex);
		} else {
			_mutex_unlock (&ext->rx_free_pkt_q_mutex);
			if (!(new = mpc_alloc_npkt (ext, MPC_MTU_SIZE, 1))) {
				errno = ENOBUFS;
				nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mpc85xx: Packet Allocation failure.");
				break;
			} else {
//				nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "new %p reallocated");
			}
		}

		npkt->framelen = (rx_bd->length - 4);
		buf = TAILQ_FIRST (&npkt->buffers);
		buf->net_iov->iov_len = npkt->framelen;


		/* Give the received packet to io-net */
		npkt->next = NULL;

		if (ext->cfg.flags & NIC_FLAG_PROMISCUOUS) {
			npkt->flags |= _NPKT_PROMISC;
		}
		
		ext->rcvpkts++;

		if (ext->io_static) {
			done = (npkt_done_t *)(npkt + 1);
			done->registrant_hdl = ext->reg_hdl;
			done->done_hdl = ext;
			npkt->req_complete++;
			if (ext->ion->tx_up(ext->reg_hdl, npkt, 0, 0, ext->cell, ext->cfg.lan, 0) == 0)	{
				mpc_receive_complete (npkt, ext, ext);
			}
		} else {
			if (npkt = ext->ion->tx_up_start(ext->reg_hdl, npkt, 0, 0, ext->cell, ext->cfg.lan, 0, ext))	{
				mpc_receive_complete (npkt, ext, ext);
			}
		}


		mpc_add_pkt (ext, new, cidx);
	}
	return;
}

