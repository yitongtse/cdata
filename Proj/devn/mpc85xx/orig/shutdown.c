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






 
#include <mpc85xx.h> 

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int 	mpc_shutdown1(int reg_hdl, void *func_hdl)

{
	mpc85xx_t	*ext = (mpc85xx_t *) func_hdl;

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: shutdown1() starting");
	}

	InterruptDetach (ext->iid [0]);
	InterruptDetach (ext->iid [1]);
	InterruptDetach (ext->iid [2]);
	ConnectDetach (ext->coid);
	ChannelDestroy (ext->chid);

	if (ext->mdi) {
		MDI_DisableMonitor (ext->mdi);
		MDI_DeRegister (&ext->mdi);
	}

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: shutdown1() done");
	}

	return (0);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int		mpc_shutdown2 (int reg_hdl, void *func_hdl)

{
	mpc85xx_t		*ext = (mpc85xx_t *) func_hdl;
	npkt_t			*npkt;
	uint32_t		*base = ext->reg;
	mpc85xx_pool_t  *p;	

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: shutdown2() starting");
	}

	pthread_cancel (ext->tid);	
	pthread_join (ext->tid, NULL);

	mpc_flush (ext->reg_hdl, ext);

	*(base + MPC_IMASK) = 0;

	mpc_reset(ext);

	/* Clear the GRS / GTS bits set during the reset process. */
	*(base + MPC_DMACTRL) &= ~(DMACTRL_GRS | DMACTRL_GTS);
	
	for( ; npkt = ext->rx_free_pkt_q; ) {
		ext->rx_free_pkt_q = npkt->next;
		if (npkt->org_data != NULL) {
			p = (mpc85xx_pool_t *)npkt->org_data;
			if (p->ref > 0) {
				if (atomic_sub_value(&(p->ref), 1) == 1) {
				  ion_free (p->buf_head);
				}
			}
		}
		ion_free (npkt);
	}

	munmap ((void *) ext->rx_bd, sizeof (mpc_bd_t) * ext->num_rx_descriptors);
	munmap ((void *) ext->tx_bd, sizeof (mpc_bd_t) * ext->num_tx_descriptors);
	free (ext->rx_pktq);
	free (ext->tx_pktq);
//	free (ext->tx_addr_table);
		
	pthread_mutex_destroy (&ext->tx_mutex);
	pthread_mutex_destroy (&ext->rx_free_pkt_q_mutex);

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: shutdown2() done");
	}

	free (ext);

	return (0);
}
