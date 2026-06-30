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
#include <net/if_ether.h>

io_net_dll_entry_t io_net_dll_entry = {
    2,
    mpc_init,
    NULL
};

io_net_registrant_funcs_t mpc_funcs = {
    8,
    NULL,
    mpc_send_packets,
    mpc_receive_complete,
    mpc_shutdown1,
    mpc_shutdown2,
    mpc_advertise,
    mpc_devctl,
    mpc_flush,
    NULL
};

io_net_registrant_t mpc_entry = {
    _REG_PRODUCER_UP | _REG_TRACK_MCAST,
    "devn-mpc85xx",
    "en",
    NULL,
    NULL,
    &mpc_funcs,
    0
};

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

/* 
 * mpc_init
 *
 * Initial entry point for io-net into driver.  Initiates a search
 * for nic.
 *
 * return  0 on success
 *        -1 and set errno on error
 */

int mpc_init(void *dll_hdl, dispatch_t * dpp, io_net_self_t * ion, char *opts)
{
    int rval;

    dpp = dpp;

    if (rval = mpc_detect(dll_hdl, ion, opts)) {
        errno = rval;
        return -1;
    }
    return (0);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
#define MPC_ROUNDUP(_ptr, _align) ((((unsigned)_ptr) + (_align - 1)) & ~(_align - 1))
npkt_t *mpc_alloc_npkt(mpc85xx_t *ext, size_t size, int num)
{
    npkt_t *npkt, *nph, **npt;
    net_buf_t *nb;
    net_iov_t *iov;
    char *ptr;
    int linesize = 64, psize;
    mpc85xx_pool_t *poolp;

    if (size < MPC_MTU_SIZE) {
        size = MPC_MTU_SIZE;
    }

    /* allocate enough space for packet data.  "linesize" extra bytes must
     * be allocated to ensure that the packet data area doesn't collide with the
     * pool information area when the packet area is re-aligned to a cache line
     * boundary. */
    psize = (MPC_ROUNDUP(size, linesize) * num + linesize) + sizeof (*poolp);

    if ((ptr = ion_alloc(psize, 0)) == NULL) {
        return (NULL);
    }

    /* the pool head is at the end of the space, to allow packet data 
     * properly aligned */
    poolp = (mpc85xx_pool_t *) (ptr + psize - sizeof (*poolp));
    poolp->buf_head = ptr;
    poolp->ref = num;

    /* get all packets */
    for (nph = NULL, npt = &nph; num; num--) {
        if ((npkt = ion_alloc_npkt(sizeof (net_buf_t) + sizeof (net_iov_t),
                                   (void **)(void *)&nb)) == NULL) {
            goto failed;
        }

        npkt->org_data = poolp;

        /* Pad buffer to begin on a cache line boundary */
        if (linesize != 0) {
            ptr = (char *)MPC_ROUNDUP(ptr, linesize);
        }

        TAILQ_INSERT_HEAD(&npkt->buffers, nb, ptrs);

        iov = (net_iov_t *) (nb + 1);

        nb->niov = 1;
        nb->net_iov = iov;

        iov->iov_base = (void *)ptr;
        iov->iov_len = size;
        iov->iov_phys = (paddr_t) (ion_mphys(iov->iov_base));

        npkt->next = NULL;
        npkt->tot_iov = 1;

        ptr += size;

        *npt = npkt;
        npt = &npkt->next;
    }
    return (nph);

  failed:
    while (npkt = nph) {
        nph = npkt->next;
        ion_free(npkt);
    }
    ion_free(poolp->buf_head);
    return NULL;
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

void mpc_update_stats(mpc85xx_t * ext)
{
    uint32_t tmp, *base = ext->reg;
    nic_ethernet_stats_t *estats = &ext->stats.un.estats;
    uint32_t car1, car2;

    car1 = *(base + MPC_CAR1);
    car2 = *(base + MPC_CAR2);

    ext->stats.octets_rxed_ok += *(base + MPC_RBYT);
    ext->stats.rxed_ok += *(base + MPC_RPKT);
    estats->fcs_errors += *(base + MPC_RFCS);
    ext->stats.rxed_multicast += *(base + MPC_RMCA);
    ext->stats.rxed_broadcast += *(base + MPC_RBCA);
    estats->align_errors += *(base + MPC_RALN);
    estats->length_field_mismatch += *(base + MPC_RFLR);
    estats->no_carrier += *(base + MPC_RCSE);
    estats->short_packets += *(base + MPC_RUND);
    estats->oversized_packets += *(base + MPC_ROVR);
    estats->internal_rx_errors += *(base + MPC_RFRG);
    estats->jabber_detected += *(base + MPC_RJBR);
    estats->internal_rx_errors += *(base + MPC_RDRP);

    ext->stats.octets_txed_ok += *(base + MPC_TBYT);
    ext->stats.txed_ok += *(base + MPC_TPKT);
    ext->stats.txed_multicast += *(base + MPC_TMCA);
    ext->stats.txed_broadcast += *(base + MPC_TBCA);
    estats->excessive_deferrals += *(base + MPC_TDFR);
    estats->single_collisions += *(base + MPC_TSCL);
    estats->multi_collisions += *(base + MPC_TMCL);
    estats->late_collisions += *(base + MPC_TLCL);
    estats->xcoll_aborted += *(base + MPC_TXCL);
    estats->total_collision_frames += *(base + MPC_TNCL);
    estats->jabber_detected += *(base + MPC_TJBR);

    tmp = *(base + MPC_TR64);
    tmp = *(base + MPC_TR127);
    tmp = *(base + MPC_TR255);
    tmp = *(base + MPC_TR511);
    tmp = *(base + MPC_TR1K);
    tmp = *(base + MPC_TRMAX);
    tmp = *(base + MPC_TRMGV);
    tmp = *(base + MPC_TFRG);

    tmp = *(base + MPC_RXCF);
    tmp = *(base + MPC_RXPF);
    tmp = *(base + MPC_RXUO);
    tmp = *(base + MPC_RCDE);
    tmp = *(base + MPC_TXPF);
    tmp = *(base + MPC_TEDF);
    tmp = *(base + MPC_TDRP);

    if (car1) {
        *(base + MPC_CAR1) = car1;
    }
    if (car2) {
        *(base + MPC_CAR2) = car2;
    }
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int mpc_process_tx_interrupt(mpc85xx_t * ext)
{
    mpc_download_complete(ext);
    return (0);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int mpc_process_rx_interrupt(mpc85xx_t * ext)
{
    mpc_receive(ext);
    return (0);
}

static int mpc_process_interrupt(mpc85xx_t * ext, int id)
{
    uint32_t *base = ext->reg;
    uint32_t ievent;

    for (;;) {
        ievent = *(base + MPC_IEVENT);

        if (!ievent) {
            break;
        }

        if (ievent & IEVENT_MSRO) {
            mpc_update_stats(ext);
        }

        *(base + MPC_IEVENT) = ievent;
        if ((ievent & (IEVENT_RXC | IEVENT_RXBO | IEVENT_RXFO | IEVENT_BSY)) !=
            0) {
            mpc_process_rx_interrupt(ext);
            if (ievent & IEVENT_BSY)
                *(base + MPC_RSTAT) = RSTAT_QHLT;
        }

        if ((ievent & (IEVENT_TXC | IEVENT_TXB | IEVENT_TXF)) != 0) {
            mpc_process_tx_interrupt(ext);
        }

        if (ievent & IEVENT_TXE) {
            *(base + MPC_TSTAT) = TSTAT_THLT;
        }
    }
    InterruptUnmask(ext->cfg.irq[id], ext->iid[id]);
    return 0;
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static void mpc_event_handler(mpc85xx_t * ext)
{
    struct _pulse pulse;
    iov_t iov;
    int rcvid;

    SETIOV(&iov, &pulse, sizeof (pulse));

    while (1) {

        rcvid = MsgReceivev(ext->chid, &iov, 1, NULL);

        if (rcvid < 0) {
            if (errno == ESRCH) {
                pthread_exit(NULL);
            }
            continue;
        }

        switch (pulse.code) {

        case MDI_TIMER:
            if (ext->probe_phy ||
                (ext->cfg.flags & NIC_FLAG_LINK_DOWN) || !ext->rcvpkts) {
                // XXXXXXX //
                // printf("mpc_event_handler(): call MDI_MonitorPhy\n");
                MDI_MonitorPhy(ext->mdi);
            }
            mpc_download_complete(ext);
            ext->rcvpkts = 0;
            break;

        case NIC_INTR_EVENT_TX:
            mpc_process_interrupt(ext, 0);
            break;
        case NIC_INTR_EVENT_RX:
            mpc_process_interrupt(ext, 1);
            break;
        case NIC_INTR_EVENT_ERR:
            mpc_process_interrupt(ext, 2);
            // XXXXXXX //
            printf("mpc_event_handler(): NIC_INTR_EVENT_ERR\n");
            break;

        default:
            if (rcvid) {
                MsgReplyv(rcvid, ENOTSUP, &iov, 1);
            }
            break;
        }
    }
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static void *mpc_driver_thread(void *data)
{
    mpc_event_handler((mpc85xx_t *) data);
    return (NULL);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static paddr_t vtophys(void *addr)
{
    off64_t offset;

    if (mem_offset64(addr, NOFD, 1, &offset, 0) == -1) {
        return (-1);
    }
    return (offset);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int mpc_init_memory(mpc85xx_t * ext)
{
    /* set up the descriptors and packet pools/lists */

    if ((ext->rx_bd = mmap(NULL, sizeof (mpc_bd_t) * ext->num_rx_descriptors,
                           PROT_READ | PROT_WRITE /*| PROT_NOCACHE */ ,
                           MAP_ANON | MAP_PHYS, NOFD, 0)) == MAP_FAILED) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "mmap at \"%s\":%d: %s\n",
                  __FILE__, __LINE__, strerror(errno));
        return (errno);
    }

    if ((ext->rx_pktq =
         calloc(ext->num_rx_descriptors, sizeof (npkt_t *))) == NULL) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "calloc at \"%s\":%d: %s\n",
                  __FILE__, __LINE__, strerror(errno));
        return (errno);
    }

    if ((ext->tx_bd = mmap(NULL, sizeof (mpc_bd_t) * ext->num_tx_descriptors,
                           PROT_READ | PROT_WRITE /*| PROT_NOCACHE */ ,
                           MAP_ANON | MAP_PHYS, NOFD, 0)) == MAP_FAILED) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "mmap at \"%s\":%d: %s\n",
                  __FILE__, __LINE__, strerror(errno));
        return (errno);
    }

    if ((ext->tx_pktq =
         calloc(ext->num_tx_descriptors, sizeof (npkt_t *))) == NULL) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "calloc at \"%s\":%d: %s\n",
                  __FILE__, __LINE__, strerror(errno));
        return (errno);
    }
#if 0
    if ((ext->tx_addr_table =
         malloc(ext->num_tx_descriptors * sizeof (uint32_t))) == NULL) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "malloc at \"%s\":%d: %s\n",
                  __FILE__, __LINE__, strerror(errno));
        return (errno);
    }
#endif

    return (EOK);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int mpc_init_list(mpc85xx_t * ext)
{
    mpc_bd_t *bd;
    int i;
    net_iov_t *iov;
    npkt_t *npkt;

    /* initialize the receive ring */

    if ((npkt =
         mpc_alloc_npkt(ext, MPC_MTU_SIZE, ext->num_rx_descriptors)) == NULL) {
        errno = ENOBUFS;
        return -1;
    }

    for (i = 0, bd = (mpc_bd_t *) ext->rx_bd; i < ext->num_rx_descriptors;
         i++, bd++) {
        if (i == ext->num_rx_descriptors - 1) {
            bd->status = (RXBD_W | RXBD_E | RXBD_I);    /* Wrap indicator */
        } else {
            bd->status = (RXBD_E | RXBD_I);
        }

        ext->rx_pktq[i] = npkt;
        npkt = npkt->next;
        ext->rx_pktq[i]->next = NULL;

        // ext->rx_pktq[i]->flags = (_NPKT_UP | MPC_KEEP_PACKET);

        iov = TAILQ_FIRST(&(ext->rx_pktq[i]->buffers))->net_iov;

        bd->buffer = iov->iov_phys;
        bd->length = 0;
    }
    ext->rx_cidx = 0;

#if 0
    /* let's also initialize the freeq */
    if ((npkt =
         mpc_alloc_npkt(ext, MPC_MTU_SIZE, ext->num_rx_descriptors)) == NULL) {
        errno = ENOBUFS;
        return -1;
    }
    _mutex_lock(&ext->rx_free_pkt_q_mutex);
    ext->rx_free_pkt_q = npkt;
    ext->num_rx_free = ext->num_rx_descriptors;
    _mutex_unlock(&ext->rx_free_pkt_q_mutex);
#endif
    /* setup the tx_addr_table */
    for (i = 0; i < ext->num_tx_descriptors; i++) {
        ext->tx_bd[i].status = 0;
#if 0
        if (i == ext->num_tx_descriptors - 1) {
            ext->tx_bd[i].status = TXBD_W;      /* Wrap */
        }
        if ((ext->tx_addr_table[i] = vtophys((void *)&ext->tx_bd[i])) == -1) {
            errno = ENOBUFS;
            return (-1);
        }
#endif
    }
    ext->tx_bd[ext->num_tx_descriptors - 1].status = TXBD_W;

    ext->tx_pidx = ext->tx_cidx = 0;

    return (EOK);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static void read_phys_addr(mpc85xx_t * ext)
{
    uint32_t *base = ext->reg;
    uint32_t addr[2];

    addr[0] = *(base + MPC_MACSTNADDR1);
    addr[1] = *(base + MPC_MACSTNADDR2);
    if (ext->cfg.verbose) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "MAC addr1 %08x - addr2 %08x",
                  addr[0], addr[1]);
    }
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static void set_phys_addr(mpc85xx_t * ext)
{
    uint32_t *base = ext->reg;
    union {
        uint32_t addr[2];
        uint8_t caddr[8];
    } tab;
    int i, j;

    memset((char *)tab.caddr, 0, 8);
    for (i = 0, j = 5; i < 6; i++, j--) {
        tab.caddr[i] = ext->cfg.current_address[j];
    }
    *(base + MPC_MACSTNADDR1) = tab.addr[0];
    *(base + MPC_MACSTNADDR2) = tab.addr[1];
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

void mpc_reset(mpc85xx_t * ext)
{
    uint32_t *base = ext->reg;
    uint32_t status;
    int timeout = MPC_TIMEOUT;

    /* Graceful transmit stop and wait for completion. */
    *(base + MPC_DMACTRL) |= DMACTRL_GTS;
    timeout = MPC_TIMEOUT;
    do {
        nanospin_ns(10);
        if (!--timeout)
            break;
        status = *(base + MPC_IEVENT);
    } while ((status & IEVENT_GTSC) != IEVENT_GTSC);

    if (!timeout) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "DMA GTS stop failed");
    }

    /* Disable Rx and Tx */
    *(base + MPC_MACCFG1) &= ~(MACCFG1_TXEN | MACCFG1_RXEN);

    /* Wait for 9.6KBytes worth of data (worst case ~ 8ms) */
    delay(9);

    /* Graceful receive stop and wait for completion. */
    *(base + MPC_DMACTRL) |= DMACTRL_GRS;
    timeout = MPC_TIMEOUT;
    do {
        nanospin_ns(10);
        if (!--timeout)
            break;
        status = *(base + MPC_IEVENT);
    } while ((status & IEVENT_GRSC) != IEVENT_GRSC);

    if (!timeout) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "DMA GRS stop failed");
    }

    *(base + MPC_IEVENT) = (IEVENT_GTSC | IEVENT_GRSC);

    *(base + MPC_MACCFG1) = MACCFG1_SFT_RESET;
    nanospin_ns(1000);
    *(base + MPC_MACCFG1) &= ~MACCFG1_SFT_RESET;

}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int mpc_config(mpc85xx_t *ext)
{
    nic_config_t *cfg = &ext->cfg;
    nic_ethernet_stats_t *estats = &ext->stats.un.estats;
    nic_stats_t *gstats = &ext->stats;
    uint32_t *base = ext->reg;

    /* nic ethernet default params */
    cfg->media = NIC_MEDIA_802_3;
    cfg->mac_length = ETH_MAC_LEN;
    if (cfg->mtu == 0 || cfg->mtu > ETH_MAX_PKT_LEN) {
        cfg->mtu = ETH_MAX_PKT_LEN;
    }
    cfg->mru = cfg->mtu;

    gstats->media = cfg->media;
    estats->valid_stats =
        NIC_ETHER_STAT_INTERNAL_TX_ERRORS |
        NIC_ETHER_STAT_INTERNAL_RX_ERRORS |
        NIC_ETHER_STAT_TX_DEFERRED |
        NIC_ETHER_STAT_XCOLL_ABORTED |
        NIC_ETHER_STAT_LATE_COLLISIONS |
        NIC_ETHER_STAT_SINGLE_COLLISIONS |
        NIC_ETHER_STAT_MULTI_COLLISIONS |
        NIC_ETHER_STAT_TOTAL_COLLISION_FRAMES |
        NIC_ETHER_STAT_ALIGN_ERRORS |
        NIC_ETHER_STAT_FCS_ERRORS |
        NIC_ETHER_STAT_JABBER_DETECTED |
        NIC_ETHER_STAT_OVERSIZED_PACKETS |
        NIC_ETHER_STAT_SHORT_PACKETS |
        NIC_ETHER_STAT_LENGTH_FIELD_OUTRANGE |
        NIC_ETHER_STAT_LENGTH_FIELD_MISMATCH |
        NIC_ETHER_STAT_EXCESSIVE_DEFERRALS;

    if (ext->num_rx_descriptors <= 0) {
        ext->num_rx_descriptors = DEFAULT_NUM_RX_DESCRIPTORS;
    }

    if (ext->num_tx_descriptors <= 0) {
        ext->num_tx_descriptors = DEFAULT_NUM_TX_DESCRIPTORS;
    }

    if (cfg->media_rate != -1) {
        ext->data_rate = (cfg->media_rate / 1000);
    } else {
        ext->data_rate = 0;
    }

    if (mpc_init_memory(ext) != EOK) {
        return (-1);
    }

    if (mpc_init_list(ext) != EOK) {
        return (-1);
    }

    mpc_reset(ext);

    /* get out physical address */
    if (nic_get_syspage_mac(cfg->permanent_address) == -1) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx: MAC address not found in system page");
        // read_phys_addr (ext);
    }

    /* set up our address */

    /* check for command line override */
    if (memcmp(cfg->current_address, "\0\0\0\0\0\0", 6) == 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "You must specify a MAC address");
        return (-1);
    }

    set_phys_addr(ext);
    read_phys_addr(ext);

    // XXXXXXX //
    printf("%s():\n", __FUNCTION__);
    printf("\tBase: 0x%08x, MPC_TBASE: 0x%08x, (base + MPC_TBASE): 0x%08x\n",
           base, MPC_TBASE, (base + MPC_TBASE));

    *(base + MPC_TBASE) = vtophys((void *)ext->tx_bd);
    *(base + MPC_RBASE) = vtophys((void *)ext->rx_bd);
    // WAYNE vvv
    printf("\tTx BD: 0x%08x, Rx BD: 0x%08x\n",
           *(base + MPC_TBASE), *(base + MPC_RBASE));
    *(base + MPC_MACCFG2) = (MACCFG2_IF_MODE_BYT | MACCFG2_PAD_CRC |
                             (cfg->duplex ? MACCFG2_FDX : 0) | MACCFG2_PRE_LEN(7));
    *(base + MPC_ECNTRL) = (ECNTRL_CLRCNT | ECNTRL_STEN | ECNTRL_AUTOZ);
    *(base + MPC_MRBLR) = MPC_MTU_SIZE;

    *(base + MPC_DMACTRL) |=
        (DMACTRL_WWR | DMACTRL_TBDSEN | DMACTRL_TDSEN /*| DMACTRL_WOP */ );

    /* Suprisingly enough, when performing loopback tests, the polled mode is FASTER than the
     * wait mode for smaller packets */
    *(base + MPC_DMACTRL) &= ~(DMACTRL_WOP);

    *(base + MPC_FIFO_TX_THR) = ext->fifo;
    *(base + MPC_FIFO_TX_STARVE) = ext->fifo_starve;
    *(base + MPC_FIFO_TX_STARVE_SHUTOFF) = ext->fifo_starve_shutoff;
#if 1
    /* ask chip put 64bytes in L2 cache */
    *(base + MPC_ATTR) = 0x40c0;
    *(base + MPC_ATTRELI) = 0x00400000;
#else
    *(base + MPC_ATTR) = 0x000000c0;
#endif
    *(base + MPC_IADDR0) = 0x0;
    *(base + MPC_IADDR1) = 0x0;
    *(base + MPC_IADDR2) = 0x0;
    *(base + MPC_IADDR3) = 0x0;
    *(base + MPC_IADDR4) = 0x0;
    *(base + MPC_IADDR5) = 0x0;
    *(base + MPC_IADDR6) = 0x0;
    *(base + MPC_IADDR7) = 0x0;
    *(base + MPC_GADDR0) = 0x0;
    *(base + MPC_GADDR1) = 0x0;
    *(base + MPC_GADDR2) = 0x0;
    *(base + MPC_GADDR3) = 0x0;
    *(base + MPC_GADDR4) = 0x0;
    *(base + MPC_GADDR5) = 0x0;
    *(base + MPC_GADDR6) = 0x0;
    *(base + MPC_GADDR7) = 0x0;
    *(base + MPC_CAR1) = 0xffffffff;
    *(base + MPC_CAR2) = 0xffffffff;
    *(base + MPC_CAM1) = 0x0;
    *(base + MPC_CAM2) = 0x0;
    if (cfg->flags & NIC_FLAG_PROMISCUOUS) {
        *(base + MPC_RCTRL) = RCTRL_PROM;
    }

    *(base + MPC_IMASK) =
        (IMASK_TXE | IMASK_TXB | IMASK_TXF | IMASK_RXBO | IMASK_RXFO |
         IMASK_BABR | IMASK_BSY | IMASK_EBERR);

    *(base + MPC_TSTAT) = TSTAT_THLT;
    *(base + MPC_RSTAT) = RSTAT_QHLT;

    *(base + MPC_DMACTRL) &= ~(DMACTRL_GRS | DMACTRL_GTS);
    *(base + MPC_MACCFG1) =
        (MACCFG1_RXEN | MACCFG1_TXEN | ext->flowctl_flag | ext->loopback);

    mpc_init_phy(ext, (cfg->media_rate == -1) ? 0 : cfg->media_rate);

    return (EOK);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

int mpc_register_device(mpc85xx_t *ext, io_net_self_t *ion, void *dll_hdl)
{
    nic_config_t *cfg = &ext->cfg;
    pthread_attr_t pattr;
    pthread_mutexattr_t mattr;
    struct sched_param param;
    struct sigevent event;
    uint16_t lan;
    int ret, err = 0;

    // XXXXXXXX //
    printf("mpc_register_device():\n");

    ext->ion = ion;
    ext->dll_hdl = dll_hdl;

    if ((ext->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) < 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "ChannelCreate at \"%s\":%d: %s\n", __FILE__, __LINE__,
                  strerror(errno));
        goto error;
    }
    err++;

    /* we need a valid connection id before calling mpc_config */
    if ((ext->coid = ConnectAttach(0, 0, ext->chid, _NTO_SIDE_CHANNEL, 0)) < 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx:  Unable to ConnectAtttach");
        goto error;
    }
    err++;

#if	0
    if (cache_init(0, &ext->cachectl, NULL) == -1) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx: cache_init() failed");
        goto error;
    }
    err++;
#endif

    /* config the device */
    if (mpc_config(ext) != EOK) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx:  mpc_config error");
        goto error;
    }

    pthread_mutexattr_init(&mattr);

    if (pthread_mutex_init(&ext->tx_mutex, &mattr) != EOK) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx:  Unable to initialize TX mutex");
        goto error;
    }
    err++;

    if (pthread_mutex_init(&ext->rx_free_pkt_q_mutex, &mattr) != EOK) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx:  Unable to initialize RX Free Queue mutex");
        goto error;
    }
    err++;

    mpc_entry.func_hdl = (void *)ext;
    mpc_entry.top_type = cfg->uptype;

    if (cfg->lan != -1) {
        mpc_entry.flags |= _REG_ENDPOINT_ARG;
        lan = cfg->lan;
    }

    if (ion_register(dll_hdl, &mpc_entry, &ext->reg_hdl, &ext->cell, &lan) < 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "ion_register at \"%s\":%d: %s\n", __FILE__, __LINE__,
                  strerror(errno));
        goto error;
    }
    cfg->lan = lan;

    if (ext->ion->
        devctl(ext->reg_hdl, DCMD_IO_NET_VERSION, &ext->version,
               sizeof (ext->version), NULL)) {
        ext->version = 0;
    }

    if (ext->ion->devctl(ext->reg_hdl, DCMD_IO_NET_STATIC, &ext->io_static,
                         sizeof (ext->io_static), NULL)) {
        ext->io_static = 0;
    }

    if (!ext->max_pkts) {
        ext->max_pkts = MPC_DEFAULT_MAX_PACKETS;
    }

    pthread_attr_init(&pattr);
    pthread_attr_setschedpolicy(&pattr, SCHED_RR);
    param.sched_priority = cfg->priority;
    pthread_attr_setschedparam(&pattr, &param);
    pthread_attr_setinheritsched(&pattr, PTHREAD_EXPLICIT_SCHED);

    if ((ret =
         pthread_create(&ext->tid, &pattr, (void *)mpc_driver_thread,
                        ext)) != EOK) {
        errno = ret;
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "pthread_create at \"%s\":%d: %s\n", __FILE__, __LINE__,
                  strerror(errno));
        goto error;
    }
    err++;

    memset(&event, 0, sizeof (event));
    SIGEV_PULSE_INIT(&event, ext->coid, cfg->priority, NIC_INTR_EVENT_TX, 0);

    if ((ext->iid[0] =
         InterruptAttachEvent(cfg->irq[0], &event,
                              _NTO_INTR_FLAGS_TRK_MSK)) < 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "InterruptAttachEvent at \"%s\":%d: %s\n", __FILE__, __LINE__,
                  strerror(errno));
        goto error;
    }
    err++;

    memset(&event, 0, sizeof (event));
    SIGEV_PULSE_INIT(&event, ext->coid, cfg->priority, NIC_INTR_EVENT_RX, 0);

    if ((ext->iid[1] =
         InterruptAttachEvent(cfg->irq[1], &event,
                              _NTO_INTR_FLAGS_TRK_MSK)) < 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "InterruptAttachEvent at \"%s\":%d: %s\n", __FILE__, __LINE__,
                  strerror(errno));
        goto error;
    }
    err++;

    memset(&event, 0, sizeof (event));
    SIGEV_PULSE_INIT(&event, ext->coid, cfg->priority, NIC_INTR_EVENT_ERR, 0);

    if ((ext->iid[2] =
         InterruptAttachEvent(cfg->irq[2], &event,
                              _NTO_INTR_FLAGS_TRK_MSK)) < 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "InterruptAttachEvent at \"%s\":%d: %s\n", __FILE__, __LINE__,
                  strerror(errno));
        goto error;
    }

    return (EOK);

  error:

    switch (err) {
    case 6:
        pthread_kill(ext->tid, SIGKILL);
    case 5:
        pthread_mutex_destroy(&ext->rx_free_pkt_q_mutex);
    case 4:
        pthread_mutex_destroy(&ext->tx_mutex);
    case 3:
        //                      cache_fini (&ext->cachectl);
    case 2:
        ConnectDetach(ext->coid);
    case 1:
        ChannelDestroy(ext->chid);
    }
    return (errno);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

int mpc_advertise(int reg_hdl, void *func_hdl)
{
    npkt_t *npkt;
    net_iov_t *iov;
    mpc85xx_t *ext = (mpc85xx_t *) func_hdl;
    io_net_msg_dl_advert_t *ap;
    nic_config_t *cfg = &ext->cfg;

    _mutex_lock(&ext->rx_free_pkt_q_mutex);
    if ((npkt = ext->rx_free_pkt_q)) {
        ext->rx_free_pkt_q = npkt->next;
        npkt->next = NULL;
        ext->num_rx_free--;
        _mutex_unlock(&ext->rx_free_pkt_q_mutex);
    } else {
        _mutex_unlock(&ext->rx_free_pkt_q_mutex);
        if ((npkt = mpc_alloc_npkt(ext, MPC_MTU_SIZE, 1)) == NULL) {
            return (0);
        }
    }

    iov = TAILQ_FIRST(&npkt->buffers)->net_iov;
    ap = iov->iov_base;

    memset(ap, 0x00, sizeof *ap);
    ap->type = _IO_NET_MSG_DL_ADVERT;
    ap->iflags = (IFF_SIMPLEX | IFF_BROADCAST | IFF_RUNNING);
    if (cfg->flags & NIC_FLAG_MULTICAST) {
        ap->iflags |= IFF_MULTICAST;
    }

    ap->mtu_min = 0;
    ap->mtu_max = cfg->mtu;
    ap->mtu_preferred = cfg->mtu;
    strcpy(ap->up_type, "en");
    itoa(cfg->lan, ap->up_type + 2, 10);

    strcpy(ap->dl.sdl_data, ap->up_type);

    ap->dl.sdl_len = sizeof (struct sockaddr_dl);
    ap->dl.sdl_family = AF_LINK;
    ap->dl.sdl_index = cfg->lan;
    ap->dl.sdl_type = IFT_ETHER;
    ap->dl.sdl_nlen = strlen(ap->dl.sdl_data);  /* not null terminated */
    ap->dl.sdl_alen = 6;
    memcpy(ap->dl.sdl_data + ap->dl.sdl_nlen, cfg->current_address, 6);

    npkt->flags |= _NPKT_MSG;
    npkt->iface = 0;
    npkt->framelen = iov->iov_len = sizeof *ap;
    npkt->tot_iov = 1;

    if (ion_add_done(ext->reg_hdl, npkt, ext) == -1) {
        mpc_receive_complete(npkt, ext, ext);
        return (0);
    }

    if (ion_rx_packets(ext->reg_hdl, npkt, 0, 0, ext->cell, cfg->lan, 0) == 0) {
        ion_tx_complete(ext->reg_hdl, npkt);
    }

    return (0);
}

/****************************************************************************/
/*                                                                          */
/*  The Usual Multicast Setup                                               */
/*                                                                          */
/****************************************************************************/

// CRC table (256 entries) generated by RFC3309

unsigned crctab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
    0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
    0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
    0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
    0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
    0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
    0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
    0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
    0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
    0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
    0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
    0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
    0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
    0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
    0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
    0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
    0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
    0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
    0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
    0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
    0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
    0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
    0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
    0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
    0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
    0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
    0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
    0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
    0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
    0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
    0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
    0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
    0x2d02ef8d,
};

// crc32core: returns RFC3309 CRC value
unsigned crc32core(unsigned char *buf, int len)
{
    unsigned crc = 0xffffffff;

    while (len--) {
        crc = (crc >> 8) ^ crctab[(crc ^ *buf) & 0xff];
        buf++;
    }
    return crc;
}

// reflect: bit-reverse value, swap 0 for n, 1 for n-1 and so on
unsigned reflect(unsigned val, int nbits)
{
    unsigned ret = 0;
    int k;

    for (k = 1; k < (nbits + 1); k++) {
        if (val & 1)
            ret |= 1 << (nbits - k);
        val >>= 1;
    }
    return ret;
}

// bump_macaddr: increment multicast address by one
void bump_macaddr(uint8_t * mcaddr)
{
    int i;

    for (i = 5; i >= 0; i--) {
        if (mcaddr[i] + 1 < 0x100) {
            mcaddr[i]++;
            while (i++ < 6)
                mcaddr[i] = 0;
            return;
        }
    }
}

void setup_mcast_range(mpc85xx_t * ext, unsigned char *da, uint64_t len, int on)
{
    unsigned crc;
    unsigned lsb;
    unsigned bitIndex;
    unsigned regIndex;
    uint32_t *gaddr_reg = ext->reg + MPC_GADDR0;

    while (len--) {

        crc = crc32core(da, ETHER_ADDR_LEN);
        lsb = reflect(crc, 8);  // reverse bottom 8 bits
        bitIndex = lsb & 0x1f;  // least 5 bits for bit index
        regIndex = lsb >> 5;    // most 3 bits for register index

        if (ext->cfg.verbose) {
            fprintf(stderr,
                    "setup_mcast_range(): mac addr: %X %X %X %X %X %X\n", da[0],
                    da[1], da[2], da[3], da[4], da[5]);
            fprintf(stderr, "setup_mcast_range(): crc:      %X\n", crc);
            fprintf(stderr, "setup_mcast_range(): rev lsb:  %X\n", lsb);
            fprintf(stderr, "setup_mcast_range(): regIndex: %d\n", regIndex);
            fprintf(stderr, "setup_mcast_range(): bitIndex: %d\n", bitIndex);
            fprintf(stderr, "setup_mcast_range(): on:       %d\n", on);
        }

        if (on) {       // set bit in indicated register
            ext->gaddr[regIndex] |= (0x80000000 >> bitIndex);
        } else {        // drop bit in indicated register
            ext->gaddr[regIndex] &= ~(0x80000000 >> bitIndex);
        }

        // if we are not in multicast promiscusous mode, write
        // changed hash mask to TSEC.  When we come out of
        // multicast promiscuous mode, our in-memory copy of
        // the hash mask will be rewritten to TSEC by mcast_prom()

        if (!ext->mcast_nprom) {
            gaddr_reg[regIndex] = ext->gaddr[regIndex];
        }

        bump_macaddr(da);
    }

}

void mcast_prom(mpc85xx_t * ext, int enable)
{
    int I;
    uint32_t *gaddr_reg = ext->reg + MPC_GADDR0;

    if (enable) {
        if (++ext->mcast_nprom != 1) {
            return;     // stacked request - already done
        }
        // first enable request - enable all in TSEC
        for (I = 0; I < 8; I++) {
            gaddr_reg[I] = 0xffffffff;
        }
    } else {    // disable (previous) request
        if (ext->mcast_nprom <= 0) {
            return;     // no underflow allowed during regression testing
        }
        // decrement positive valued counter
        if (--ext->mcast_nprom) {
            return;     // still outstanding stacked enable all requests
        }
        // coming out of multicast promiscuous mode - refresh TSEC     
        for (I = 0; I < 8; I++) {
            gaddr_reg[I] = ext->gaddr[I];
        }
    }
}

#define CONVERT_MAC_ADDR_2_VAL(x)  \
(x[5] + (x[4]<<8) + (x[3]<<16) + (x[2]<<24) + \
(((unsigned long long)(x[1]))<<32) + (((unsigned long long)x[0])<<40) )

int do_multicast(mpc85xx_t * ext, struct _io_net_msg_mcast *mcast)
{
    unsigned long long start_val;
    unsigned long long end_val;
    unsigned long long range_length;
    unsigned char start[ETHER_ADDR_LEN];
    unsigned char end[ETHER_ADDR_LEN];

    if (ext->cfg.verbose) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
                  "multicast msg %p - type %x - flags %x", mcast, mcast->type,
                  mcast->flags);
    }

    if (mcast->type == _IO_NET_REMOVE_MCAST) {
        if ((mcast->flags & _IO_NET_MCAST_ALL) == 0) {
            memcpy(start, LLADDR(&mcast->mc_min.addr_dl), ETHER_ADDR_LEN);
            memcpy(end, LLADDR(&mcast->mc_max.addr_dl), ETHER_ADDR_LEN);

            start_val = CONVERT_MAC_ADDR_2_VAL(start);
            end_val = CONVERT_MAC_ADDR_2_VAL(end);

            if (start_val > end_val) {
                if (ext->cfg.verbose) {
                    nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                              "devn-mpc85xx: _IO_NET_REMOVE_MCAST: start_val > end_val",
                              mcast->type);
                }
                return EINVAL;
            }

            range_length = end_val - start_val + 1;

            if (range_length > 256) {
                mcast_prom(ext, 0);     // was never added to filter
            } else {
                setup_mcast_range(ext, start, range_length, 0);
            }
        } else {
            mcast_prom(ext, 0); // remove all multicast
        }
    } else if (mcast->type == _IO_NET_JOIN_MCAST) {
        if ((mcast->flags & _IO_NET_MCAST_ALL) == 0) {
            if (nic_ether_mcast_valid(mcast) == -1) {
                return EINVAL;
            }

            memcpy(start, LLADDR(&mcast->mc_min.addr_dl), ETHER_ADDR_LEN);
            memcpy(end, LLADDR(&mcast->mc_max.addr_dl), ETHER_ADDR_LEN);

            start_val = CONVERT_MAC_ADDR_2_VAL(start);
            end_val = CONVERT_MAC_ADDR_2_VAL(end);

            if (start_val > end_val) {
                if (ext->cfg.verbose) {
                    nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                              "devn-mpc85xx: _IO_NET_JOIN_MCAST: start_val > end_val",
                              mcast->type);
                }
                return EINVAL;
            }

            range_length = end_val - start_val + 1;

            if (range_length > 256) {
                mcast_prom(ext, 1);     // dont add to filter
            } else {
                setup_mcast_range(ext, start, range_length, 1);
            }
        } else {
            mcast_prom(ext, 1); // join all multicast
        }
    } else {
        if (ext->cfg.verbose) {
            nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                      "devn-mpc85xx: message 0x%x not supported", mcast->type);
        }
        return EINVAL;
    }

    return (EOK);
}
