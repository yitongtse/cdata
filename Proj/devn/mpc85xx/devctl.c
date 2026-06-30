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

#include <mpc85xx.h>
#include <sys/sockio.h>

uint16_t mpc_mii_read(void *handle, uint8_t phy_add, uint8_t reg_add);

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int mpc_devctl(void *hdl, int dcmd, void *data, size_t size,
               union _io_net_dcmd_ret_cred *ret_cred)
{
    mpc85xx_t *ext = (mpc85xx_t *) hdl;
    int status, promoff = 0;
    nic_config_t *cfg = &ext->cfg;
    uint32_t *base = ext->reg;

    status = EOK;

    switch (dcmd) {

    case DCMD_IO_NET_GET_STATS:
        _mutex_lock(&ext->tx_mutex);
        mpc_update_stats(ext);
        _mutex_unlock(&ext->tx_mutex);
        memcpy(data, &ext->stats, sizeof (nic_stats_t));
        break;

    case DCMD_IO_NET_GET_CONFIG:
        memcpy(data, &ext->cfg, sizeof (ext->cfg));
        status = EOK;
        break;

    case DCMD_IO_NET_PROMISCUOUS:
        if (ext->version != 0 && ret_cred->ret_cred.cred->euid) {
            status = EPERM;
            break;
        }

        if (*(int *)data) {
            cfg->flags |= NIC_FLAG_PROMISCUOUS;
            *(base + MPC_RCTRL) |= RCTRL_PROM;
        } else {
            cfg->flags &= ~NIC_FLAG_PROMISCUOUS;
            *(base + MPC_RCTRL) &= ~RCTRL_PROM;
            promoff = 1;
        }
        break;

    case DCMD_IO_NET_CHANGE_MCAST:
        if (cfg->flags & NIC_FLAG_MULTICAST) {
            status = do_multicast(ext, (io_net_msg_join_mcast_t *) data);
#if 0
            {   // easy multicast
                void set_reset_mcast_filt(mpc85xx_t * ext, int set);
                struct _io_net_msg_mcast *mcast =
                    (io_net_msg_join_mcast_t *) data;

                if (mcast->type == _IO_NET_REMOVE_MCAST) {
                    set_reset_mcast_filt(ext, 0);
                } else {
                    set_reset_mcast_filt(ext, 1);
                }
            }
#endif

        } else {
            status = ENOTSUP;
        }
        break;

    case DCMD_IO_NET_TX_FLUSH:
        _mutex_lock(&ext->tx_mutex);
        mpc_reap_pkts(ext, 0);
        _mutex_unlock(&ext->tx_mutex);
        break;

    default:
        status = ENOTSUP;
        break;
    }

    return (status);
}
