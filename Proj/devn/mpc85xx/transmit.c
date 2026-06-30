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

#define TIMEOUT 100000000
#define DEFRAG_LIMIT MAX_DN_FRAGMENTS

/****************************************************************************/
/* WAYNE                                                                    */
/****************************************************************************/

static paddr_t vtophys(void *addr)
{
    off64_t offset;

    if (mem_offset64(addr, NOFD, 1, &offset, 0) == -1) {
        return (-1);
    }
    return (offset);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

static npkt_t *mpc_defrag(mpc85xx_t * ext, npkt_t * npkt)
{
    npkt_t *dpkt;
    char *dst;
    net_iov_t *iov;
    net_buf_t *dov, *buf;
    int totalLen;
    int i, len;

    /* Setup our default return value */
    dpkt = NULL;

    /* Do some checks */
    buf = TAILQ_FIRST(&npkt->buffers);
    totalLen = 0;
    while (buf != NULL) {
        for (i = 0, iov = buf->net_iov; i < buf->niov; i++, iov++) {
            totalLen += iov->iov_len;
        }
        buf = TAILQ_NEXT(buf, ptrs);
    }

    if (totalLen <= MPC_MTU_SIZE) {
        if ((dpkt = mpc_alloc_npkt(ext, npkt->framelen, 1)) != NULL) {
            dpkt->framelen = npkt->framelen;
            dpkt->csum_flags = npkt->csum_flags;
            dpkt->flags = MPC_DEFRAG_PACKET;
            dpkt->next = NULL;

            dov = TAILQ_FIRST(&dpkt->buffers);
            dst = dov->net_iov->iov_base;

            len = 0;
            buf = TAILQ_FIRST(&npkt->buffers);
            while (buf != NULL) {
                for (i = 0, iov = buf->net_iov; i < buf->niov; i++, iov++) {
                    memcpy(dst, iov->iov_base, iov->iov_len);
                    dst += iov->iov_len;
                    len += iov->iov_len;
                }

                buf = TAILQ_NEXT(buf, ptrs);
            }

            dov->net_iov->iov_len = len;
        }
    }

    npkt->next = NULL;
    ion_tx_complete(ext->reg_hdl, npkt);
    return (dpkt);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int mpc_reap_pkts(mpc85xx_t * ext, int max_reap)
{
    int idx;
    npkt_t *npkt;
    mpc_bd_t *bd;
    int reaped = 0;

    if ((ext->tx_pkts_queued == 0) && (ext->tx_pidx == ext->tx_cidx)) {
        return (0);
    }

    do {
        uint16_t status;

        idx = ext->tx_cidx;
        bd = &ext->tx_bd[idx];
        status = bd->status;
        if (status & TXBD_R)
            break;
        bd->status = (TXBD_W & status);

        npkt = ext->tx_pktq[idx];
        if (npkt) {
            npkt->next = NULL;
            if (npkt->flags & MPC_DEFRAG_PACKET) {
                mpc85xx_pool_t *p = npkt->org_data;

                if (atomic_sub_value(&p->ref, 1) == 1) {
                    ion_free(p->buf_head);
                    npkt->org_data = NULL;
                }
                ion_free(npkt);
            } else {
                ion_tx_complete(ext->reg_hdl, npkt);
            }
            ext->tx_pkts_queued--;
        }

        ext->tx_pktq[idx] = NULL;
        ext->tx_q_len--;
        ext->tx_cidx = NEXT_TX(ext->tx_cidx);
        if (max_reap && ++reaped == max_reap) {
            break;
        }
    } while (ext->tx_cidx != ext->tx_pidx);

    return (0);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

/* 
 * mpc_send
 *
 * Initiates transmission of a packet to the NIC.  
 *
 */

int mpc_send(mpc85xx_t * ext, npkt_t * npkt)
{
    int idx;
    npkt_t *npkt2;
    net_buf_t *buf;
    net_iov_t *iov;
    mpc_bd_t *tx_bd, *tx_bd_start, *tx_bd_prev;
    int i;
    uint16_t status;
    int pckt_cnt; // WAYNE

    pckt_cnt = 0;

    if (npkt->tot_iov > DEFRAG_LIMIT) {
        if ((npkt2 = mpc_defrag(ext, npkt)) == NULL) {
            /* If that fails, probably not worth sticking around */
            return (-1);
        }
        npkt = npkt2;
    } else {
        npkt->flags &= ~MPC_DEFRAG_PACKET;
    }

    _mutex_lock(&ext->tx_mutex);
    if (ext->tx_q_len >= DEFRAG_LIMIT) {
        mpc_reap_pkts(ext, DEFRAG_LIMIT);
    }

    if (npkt->tot_iov + 1 > ext->num_tx_descriptors - ext->tx_q_len) {
        npkt->flags |= _NPKT_NOT_TXED;
        errno = ENOBUFS;
        if (npkt->flags & MPC_DEFRAG_PACKET) {
            mpc85xx_pool_t *p = npkt->org_data;

            if (atomic_sub_value(&p->ref, 1) == 1) {
                ion_free(p->buf_head);
                npkt->org_data = NULL;
            }
            ion_free(npkt);
        } else {
            ion_tx_complete(ext->reg_hdl, npkt);
        }
        _mutex_unlock(&ext->tx_mutex);
        return (TX_DOWN_FAILED);
    }

    idx = ext->tx_pidx;
    tx_bd_start = tx_bd_prev = tx_bd = NULL;
    status = TXBD_R;
    for (buf = TAILQ_FIRST(&npkt->buffers); buf; buf = TAILQ_NEXT(buf, ptrs)) {
        for (i = 0, iov = buf->net_iov; i < buf->niov; i++, iov++) {
            ++pckt_cnt; // WAYNE
            tx_bd = &ext->tx_bd[idx];
            tx_bd->buffer = iov->iov_phys;
            tx_bd->length = iov->iov_len;
            // WAYNE vvv
            // printf("%s() >> Tx BD 0x%08x, Pckt 0x%08x (%d bytes)\n", __FUNCTION__,
            //        vtophys((void *) tx_bd), tx_bd->buffer, tx_bd->length);
            if (tx_bd->status & TXBD_R) {
                npkt->flags |= _NPKT_NOT_TXED;
                errno = ENOBUFS;
                if (npkt->flags & MPC_DEFRAG_PACKET) {
                    mpc85xx_pool_t *p = npkt->org_data;

                    if (atomic_sub_value(&p->ref, 1) == 1) {
                        ion_free(p->buf_head);
                        npkt->org_data = NULL;
                    }
                    ion_free(npkt);
                } else {
                    ion_tx_complete(ext->reg_hdl, npkt);
                }
                _mutex_unlock(&ext->tx_mutex);
                return (TX_DOWN_FAILED);
            }
            if (!tx_bd_start) {
                tx_bd_start = tx_bd;
            } else {
                if (tx_bd_prev) {
                    tx_bd_prev->status |= status;
                }
                tx_bd_prev = tx_bd;
            }

            idx = NEXT_TX(idx);
            ext->tx_q_len++;
        }
    }

    ext->tx_pktq[PREV_TX(idx)] = npkt;
    ext->tx_pkts_queued++;
    ext->tx_pidx = idx;

    /*
     * Start the transmitter.
     */
    if (tx_bd != tx_bd_start) {
        tx_bd->status |= (TXBD_R | TXBD_L);
    } else {
        status |= TXBD_L;
    }
    tx_bd_start->status |= status;
    // printf("%s() >> send total %d pckt\n", __FUNCTION__, pckt_cnt); // WAYNE

    /* WAIT mode seems to be slower than POLLED mode for smaller packets. */
    //      *(ext->reg + MPC_TSTAT) = TSTAT_THLT;

    _mutex_unlock(&ext->tx_mutex);
    return (EOK);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

/*
 * mpc_send_packets
 *
 * Entry point from io-net layer by protocol to send
 * packets out the NIC
 *
 */

int mpc_send_packets(npkt_t * npkt, void *hdl)
{
    mpc85xx_t *ext = (mpc85xx_t *) hdl;
    net_buf_t *buf;
    int ret = EOK;

    if (npkt->flags & _NPKT_MSG) {
        buf = TAILQ_FIRST(&npkt->buffers);
        if (buf != NULL) {
            /* If selective multicasting worked on this card 
             * this is where it would happen */
            if (ext->cfg.flags & NIC_FLAG_MULTICAST) {
                do_multicast(ext,
                             (io_net_msg_join_mcast_t *) buf->net_iov->
                             iov_base);
            } else {
                ret = ENOTSUP;
            }
        }

        ion_tx_complete(ext->reg_hdl, npkt);
    } else {
        mpc_send(ext, npkt);
    }

    return (ret);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

/* 
 * mpc_download_complete
 *
 * Check to see if any pkts can be reaped (released back to protocol).
 *
 */

int mpc_download_complete(mpc85xx_t * ext)
{
    _mutex_lock(&ext->tx_mutex);
    mpc_reap_pkts(ext, 0);
    _mutex_unlock(&ext->tx_mutex);
    return (EOK);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int mpc_flush(int reg_hdl, void *func_hdl)
{
#if 0
    mpc85xx_t *ext = (mpc85xx_t *) func_hdl;
    npkt_t *npkt;
    int i;

    _mutex_lock(&ext->tx_mutex);
    ext->tx_pidx = ext->tx_cidx = ext->num_tx_descriptors - 1;
    ext->tx_pkts_queued = 0;
    ext->tx_q_len = 0;

    for (i = 0; i < ext->num_tx_descriptors; i++) {
        ext->tx_pktq[i] = NULL;
    }

    for (i = 0; i < ext->num_rx_descriptors; i++) {
        if (npkt = ext->rx_pktq[i]) {
            ion_free(npkt->org_data);

            ext->rx_pktq[i] = NULL;
        }
    }
    _mutex_unlock(&ext->tx_mutex);
#endif
    return (EOK);
}
