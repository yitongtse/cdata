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
#include "ppc/e500cpu.h"
#include "ppc/inline.h"
#include <ppc/cpu.h>
#include <hw/sysinfo.h>



struct tsec_params {
    paddr_t paddr;
    unsigned int irq_present;
    unsigned int irq_rx;
    unsigned int irq_tx;
    unsigned int irq_err;
    unsigned int mac_present;
    unsigned char mac[6];
    unsigned int phy_present;
    unsigned int phy_addr;
};

#define NIC_PRIORITY 			21

static char *mpc_opts[] = {
    "receive",        // 0, Rx BD number
    "transmit",       // 1, Tx BD number
    "pauseignore",    // 2, Rx Flow control
    "pausesuppress",  // 3, Tx Flow control
    "loopback",       // 4, Loop Back
    "fifo",           // 5, FIFO threshold
    "probe_phy",      // 6, Periodic Phy status probe
    "phy_addr",       // 7, ????
    "syspage",        // 8, use syspage to setup
    "phy_incr",       // 9, phy address increment
    "tsec_index",     // 10, TSEC index (from 0 to 3 for MPC8548)
    NULL
};

static int tsec_index = 0;



/***************************************************************************/
/*                                                                         */
/***************************************************************************/

static int mpc_parse_options(mpc85xx_t *ext, const char *optstring,
                             nic_config_t *cfg)
{
    char *value;
    int opt;
    char *options, *freeptr;
    char *c;
    int rc = 0;
    int err = EOK;

    if (optstring == NULL) {
        return 0;
    }

    /* getsubopt() is destructive */
    freeptr = options = strdup(optstring);

    while (options && *options != '\0') {
        c = options;
        if ((opt = getsubopt(&options, mpc_opts, &value)) == -1) {
            if (nic_parse_options(cfg, value) == EOK) {
                continue;
            }
            goto error;
        }

        switch (opt) {
        case 0:
            // receive BD
            if (ext != NULL) {
                ext->num_rx_descriptors = strtoul(value, 0, 0);
            }
            continue;

        case 1:
            // transmit BD
            if (ext != NULL) {
                ext->num_tx_descriptors = strtoul(value, 0, 0);
            }
            continue;

        case 2:
            // pauseignore
            if (ext != NULL) {
                ext->flowctl_flag &= ~MACCFG1_RX_FLOW;
            }
            continue;

        case 3:
            // pausesuppress
            if (ext != NULL) {
                ext->flowctl_flag &= ~MACCFG1_TX_FLOW;
            }
            continue;

        case 4:
            // loopback
            if (ext != NULL) {
                ext->loopback |= MACCFG1_LP_BACK;
            }
            continue;

        case 5:
            // fifo
            if (ext != NULL) {
                ext->fifo = strtoul(value, 0, 0);
                ext->fifo_starve = ext->fifo / 2;
                ext->fifo_starve_shutoff = ext->fifo;
            }
            continue;

        case 6:
            // probe_phy
            if (ext != NULL) {
                ext->probe_phy = strtoul(value, 0, 0);
            }
            continue;

        case 7:
            // phy_addr
            if (ext != NULL) {
                ext->phy_addr = strtoul(value, 0, 0);
            }
            continue;

        case 8:
            // syspage
            if (ext != NULL) {
                ext->use_syspage = 1;
            }
            continue;

        case 9:
            // phy_incr
            if (ext != NULL) {
                ext->phy_incr = strtoul(value, 0, 0);
            }
            continue;

        case 10:
            // tse_index
            tsec_index = strtoul(value, 0, 0);

        default:
            continue;
        }

      error:
        nic_slogf(_SLOGC_NETWORK, _SLOG_WARNING,
                  "devn-mpc85xx: unknown option %s", c);
        err = EINVAL;
        rc = -1;
    }

    free(freeptr);

    errno = err;

    return (rc);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

static int mpc_create_instance(void *dll_hdl, io_net_self_t * ion,
                               char *options, int idx, uint32_t iobase,
                               struct tsec_params *tparms_p)
{
    nic_config_t *cfg;
    mpc85xx_t *ext;

    if ((ext = calloc(1, sizeof (*ext) + 256 * sizeof (int))) == NULL) {
        return (-1);
    }
    printf("mpc_create_instance(): ext size %d bytes\n",
           (sizeof (*ext) + 256 * sizeof (int))); // WAYNE

    cfg = &ext->cfg;

    ext->iobase = ext->phy_base = (uintptr_t) iobase;

    if (tparms_p == NULL) {
        switch (idx) {
        case 0:
            ext->iobase += MPC_TSEC1_BASE;
            if (cfg->num_irqs == 0) {
                if (PPC_GET_FAM_MEMBER(get_pvr()) == PPC_E300C1) {
                    cfg->irq[0] = PPC83xx_INTR_GEN_TSEC1_Tx;
                    cfg->irq[1] = PPC83xx_INTR_GEN_TSEC1_Rx;
                    cfg->irq[2] = PPC83xx_INTR_GEN_TSEC1_Err;
                } else {
                    cfg->irq[0] = PPC85xx_INTR_TSEC1_TX;
                    cfg->irq[1] = PPC85xx_INTR_TSEC1_RX;
                    cfg->irq[2] = PPC85xx_INTR_TSEC1_ERROR;
                }
                cfg->num_irqs = 3;
            }
            break;
        case 1:
            ext->iobase += MPC_TSEC2_BASE;
            if (cfg->num_irqs == 0) {
                if (PPC_GET_FAM_MEMBER(get_pvr()) == PPC_E300C1) {
                    cfg->irq[0] = PPC83xx_INTR_GEN_TSEC2_Tx;
                    cfg->irq[1] = PPC83xx_INTR_GEN_TSEC2_Rx;
                    cfg->irq[2] = PPC83xx_INTR_GEN_TSEC2_Err;
                } else {
                    cfg->irq[0] = PPC85xx_INTR_TSEC2_TX;
                    cfg->irq[1] = PPC85xx_INTR_TSEC2_RX;
                    cfg->irq[2] = PPC85xx_INTR_TSEC2_ERROR;
                }
                cfg->num_irqs = 3;
            }
            break;
        case 2:
            ext->iobase += MPC_TSEC3_BASE;
            if (cfg->num_irqs == 0) {
                cfg->irq[0] = PPC85xx_INTR_TSEC3_TX;
                cfg->irq[1] = PPC85xx_INTR_TSEC3_RX;
                cfg->irq[2] = PPC85xx_INTR_TSEC3_ERROR;
                cfg->num_irqs = 3;
            }
            break;
        case 3:
            ext->iobase += MPC_TSEC4_BASE;
            if (cfg->num_irqs == 0) {
                cfg->irq[0] = PPC85xx_INTR_TSEC4_TX;
                cfg->irq[1] = PPC85xx_INTR_TSEC4_RX;
                cfg->irq[2] = PPC85xx_INTR_TSEC4_ERROR;
                cfg->num_irqs = 3;
            }
            break;
        }
        ext->phy_addr = 0;
        ext->phy_incr = 1;

        printf("mpc_create_instance(): TSEC %d <AA>\n", idx); // WAYNE
        printf("\tTx: %d, Rx: %d, Er: %d <AA>\n",
               cfg->irq[0], cfg->irq[1], cfg->irq[2]); // WAYNE

    } else {

        /* Parameters in tsec struct must be valid. */
        ext->iobase = tparms_p->paddr;
        cfg->irq[0] = tparms_p->irq_tx;
        cfg->irq[1] = tparms_p->irq_rx;
        cfg->irq[2] = tparms_p->irq_err;
        cfg->num_irqs = 3;

        if (tparms_p->mac_present) {
            memcpy(cfg->current_address, tparms_p->mac, 6);
        }
        ext->phy_addr = tparms_p->phy_addr;

        printf("mpc_create_instance(): TSEC %d <BB>\n", idx); // WAYNE
        printf("\tTx: %d, Rx: %d, Er: %d <BB>\n",
               cfg->irq[0], cfg->irq[1], cfg->irq[2]); // WAYNE
    }

    ext->phy_base += MPC_TSEC1_BASE;    /* PHYs are based off the primary TSEC */
    if ((ext->reg =
         mmap_device_memory(NULL, 0x1000, PROT_READ | PROT_WRITE | PROT_NOCACHE,
                            MAP_SHARED, ext->iobase)) == MAP_FAILED) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx: mmap memory failed");
        return (-1);
    }

    if ((ext->phy_reg =
         mmap_device_memory(NULL, 0x1000, PROT_READ | PROT_WRITE | PROT_NOCACHE,
                            MAP_SHARED, ext->phy_base)) == MAP_FAILED) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx: mmap PHY memory failed");
        return (-1);
    }

    printf("\tphy_addr: 0x%08x\n", ext->phy_addr); // WAYNE

    /* set some defaults for the command line options */
    cfg->flags = NIC_FLAG_MULTICAST;
    cfg->device_index = idx;
    cfg->media_rate = cfg->duplex = -1;
    cfg->priority = NIC_PRIORITY;
    cfg->iftype = IFT_ETHER;
    cfg->lan = -1;
    strcpy(cfg->uptype, "en");
    ext->num_tx_descriptors = DEFAULT_NUM_TX_DESCRIPTORS;
    ext->num_rx_descriptors = DEFAULT_NUM_RX_DESCRIPTORS;
    ext->flowctl_flag = MACCFG1_RX_FLOW | MACCFG1_TX_FLOW;
    ext->loopback = 0;
    ext->fifo = 0x40;
    ext->fifo_starve = 0x20;
    ext->fifo_starve_shutoff = 0x40;
    ext->probe_phy = 1; // ????????

    if (mpc_parse_options(ext, options, cfg) == -1) {
        return (-1);
    }

    if (tparms_p != NULL) {
        if (cfg->verbose) {
            nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
                      "devn-mpc85xx: IF %d: Base register 0x%08X", idx,
                      tparms_p->paddr);
            nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
                      "devn-mpc85xx: IF %d: IRQ Tx %2d Rx %2d Err %2d", idx,
                      tparms_p->irq_tx, tparms_p->irq_rx, tparms_p->irq_err);

            if (tparms_p->mac_present) {
                nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
                          "devn-mpc85xx: IF %d: MAC %02X:%02X:%02X:%02X:%02X:%02X",
                          idx, tparms_p->mac[0], tparms_p->mac[1],
                          tparms_p->mac[2], tparms_p->mac[3], tparms_p->mac[4],
                          tparms_p->mac[5]);
            }
            nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
                      "devn-mpc85xx: IF %d: PHY address %d", idx,
                      tparms_p->phy_addr);
        }
    }

    if (tparms_p == NULL) {
        ext->phy_addr += idx * ext->phy_incr;
    }

    if ((tparms_p == NULL) || (!tparms_p->mac_present)) {
        // ext->cfg.current_address[5] += idx;  // Wayne
    }

    if (cfg->mtu == 0 || cfg->mtu > ETH_MAX_PKT_LEN) {
        /* TODO Add oversized packet support */
        cfg->mtu = ETH_MAX_PKT_LEN;
    }

    if (cfg->mru == 0 || cfg->mru > ETH_MAX_PKT_LEN) {
        /* TODO Add oversized packet support */
        cfg->mru = ETH_MAX_PKT_LEN;
    }

    ext->force_advertise = -1;

    if (cfg->duplex != -1) {
        /* User forced the duplex, in this case, we disable autoneg. */
        ext->force_advertise = 0;
        if (cfg->media_rate == -1) {
            /* Force speed to a default */
            cfg->media_rate = 100 * 1000;
        }
    } else {
        if (cfg->media_rate != -1) {
            if (cfg->media_rate == 100 * 1000) {
                ext->force_advertise = MDI_100bTFD | MDI_100bT;
            } else {
                ext->force_advertise = MDI_10bTFD | MDI_10bT;
            }
        }
    }

    if (cfg->num_mem_windows == 0) {
        cfg->num_mem_windows = 1;
    }
    cfg->mem_window_base[0] = ext->iobase;
    cfg->mem_window_size[0] = 0x1000;

    strcpy(cfg->device_description, "MPC85XX TSEC");

    ext->num_rx_descriptors &= ~3;
    if (ext->num_rx_descriptors < MIN_NUM_RX_DESCRIPTORS) {
        ext->num_rx_descriptors = MIN_NUM_RX_DESCRIPTORS;
    }
    if (ext->num_rx_descriptors > MAX_NUM_RX_DESCRIPTORS) {
        ext->num_rx_descriptors = MAX_NUM_RX_DESCRIPTORS;
    }

    ext->num_tx_descriptors &= ~3;
    if (ext->num_tx_descriptors < MIN_NUM_TX_DESCRIPTORS) {
        ext->num_tx_descriptors = MIN_NUM_TX_DESCRIPTORS;
    }
    if (ext->num_tx_descriptors > MAX_NUM_TX_DESCRIPTORS) {
        ext->num_tx_descriptors = MAX_NUM_TX_DESCRIPTORS;
    }

    cfg->revision = NIC_CONFIG_REVISION;
    ext->stats.revision = NIC_STATS_REVISION;

    // WAYNE
    printf("\tio_base : 0x%08x, reg    : 0x%08x\n", ext->iobase, ext->reg);
    printf("\tphy_base: 0x%08x, phy_reg: 0x%08x\n", ext->phy_base,
           ext->phy_reg);
    printf("\ttx_descp: %d, rx_dscp: %d\n",
           ext->num_tx_descriptors, ext->num_rx_descriptors);
    printf("\tphy_addr: 0x%08x\n", ext->phy_addr);

    if (!mpc_register_device(ext, ion, dll_hdl)) {
        mpc_advertise(ext->reg_hdl, ext);
        return (EOK);
    }

    free(ext);

    errno = ENODEV;
    return (-1);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

uint32_t find_immr_info()
{
    struct asinfo_entry *asinfo;
    int32_t i, num, cnt;

    num = _syspage_ptr->asinfo.entry_size / sizeof (*asinfo);
    asinfo = SYSPAGE_ENTRY(asinfo);

    for (i = cnt = 0; i < num; ++i) {
        char *name;

        name = __hwi_find_string(asinfo->name);
        if (strcmp(name, "immr")) {
            asinfo++;
            continue;
        }
        return ((uint32_t) asinfo->start);
    }
    return (0);
}

/*******************************************************************************
 *
 * mpc_detect
 *
 *
 */

static inline unsigned
ppc8xxx_get_syspage_params(struct tsec_params *tsec, unsigned last)
{
    hwi_tag *tag;
    int irqnum = 0;
    unsigned off;
    unsigned item;
    char *name;

    /* Find network hardware information. */
    item = hwi_find_item(last, HWI_ITEM_DEVCLASS_NETWORK, "tsec", NULL);

    if (item == HWI_NULL_OFF) {
        return (item);
    }

    off = item;
    tsec->paddr = 0;
    tsec->irq_present = 0;
    tsec->mac_present = 0;
    tsec->phy_present = 0;

    while ((off = hwi_next_tag(off, 1)) != HWI_NULL_OFF) {
        tag = hwi_off2tag(off);
        name = __hwi_find_string(((hwi_tag *) tag)->prefix.name);

        if (strcmp(name, HWI_TAG_NAME_location) == 0) {
            tsec->paddr = (paddr_t) tag->location.base;
        } else if (strcmp(name, HWI_TAG_NAME_irq) == 0) {
            switch (irqnum) {
            case 0:
                tsec->irq_tx = tag->irq.vector;
                break;
            case 1:
                tsec->irq_rx = tag->irq.vector;
                break;
            case 2:
                tsec->irq_err = tag->irq.vector;
                break;
            default:
                break;
            }
            irqnum++;
        } else if (strcmp(name, HWI_TAG_NAME_nicaddr) == 0) {
            if (tag->nicaddr.len == 6) {
                memcpy(tsec->mac, tag->nicaddr.addr, 6);
                tsec->mac_present = 1;
            }
        } else if (strcmp(name, HWI_TAG_NAME_nicphyaddr) == 0) {
            tsec->phy_addr = tag->nicphyaddr.addr;
            tsec->phy_present = 1;
        }
    }

    if (irqnum == 3) {
        tsec->irq_present = 1;
    }

    return (item);
}

int mpc_detect(void *dll_hdl, io_net_self_t *ion, char *options)
{
    nic_config_t cfg;
    int idx = 0;
    int rc = EOK;
    uint32_t iobase;
    mpc85xx_t *ext = dll_hdl;


    printf("mpc_detect():\n"); // WAYNE

    memset(&cfg, 0, sizeof (cfg));

    ext->use_syspage = 0;
    cfg.device_index = 0xffffffff;
    if (options != NULL) {
        if (mpc_parse_options(ext, options, &cfg) != 0)
            return (errno);
    }

    errno = ENODEV;

    if ((iobase = find_immr_info()) == 0) {
        nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                  "devn-mpc85xx: Unable to find base address");
        return (ENODEV);
    }

    if (ext->use_syspage) {
        unsigned config_offset = HWI_NULL_OFF;
        struct tsec_params tsec;

        printf("mpc_detect(): use syspage to config TSEC\n"); // WAYNE

        /* Use info stored in the system page to retrieve configuration. */
        config_offset = ppc8xxx_get_syspage_params(&tsec, config_offset);
        if (config_offset == HWI_NULL_OFF) {
            nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                      "devn-mpc85xx: No syspage hardware configuration "
                      "information found");
            return (ENODEV);
        }

        while (config_offset != HWI_NULL_OFF) {

            if (tsec.paddr && tsec.irq_present) {

                if ((cfg.device_index == idx) ||
                    (cfg.device_index == 0xffffffff)) {
                    /* If device index specified, ignore all other
                     * device cfgs. */

                    printf("mpc_detect(): config TSEC %d\n", idx); // WAYNE

                    rc = mpc_create_instance(dll_hdl, ion, options, idx,
                                             iobase, &tsec);
                    if (rc != EOK) {
                        return (ENODEV);
                    }

                    if (cfg.device_index == idx) {
                        /* Configured required device... No need to look for 
                         * more. */
                        printf("mpc_detect(): quit TSEC %d\n", idx); // WAYNE

                        break;
                    }
                }

            } else {
                char missing[64];

                /* Must specify the base register address and interrupts from 
                 * the system page.  All other info can be obtained from
                 * default code / command line options. */
                strcpy(missing, "");
                if (!tsec.paddr) {
                    strcat(missing, "base address ");
                }
                if (!tsec.irq_present) {
                    strcat(missing, "interrupt(s) ");
                }
                if (!tsec.phy_present) {
                    strcat(missing, "phy address ");
                }

                nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                          "devn-mpc85xx: Missing %s in syspage configuration "
                          "for device index %d", missing, idx);
                printf("mpc_detect(): miss cnfg TSEC %d\n", idx); // WAYNE

                return (ENODEV);
            }

            idx++;
            config_offset = ppc8xxx_get_syspage_params(&tsec, config_offset);
        }

    } else {
        /* All cfg information specified on command line. */
        int ntsec;

        printf("mpc_detect(): use CLI to config (%d) TSEC\n", ntsec); // WAYNE

#if 0 // WAYNE (force to config only one TSEC)
        if ((get_spr(PPCE500_SPR_SVR) >> 16) == 0x8039) {
            /* 8548 chip */
            ntsec = 4;
        } else {
            ntsec = 2;
        }

        if (cfg.device_index != 0xffffffff) {
            idx = cfg.device_index;
        }

        while (idx < ntsec) {

            printf("mpc_detect(): config TSEC %d\n", idx); // WAYNE
            rc = mpc_create_instance(dll_hdl, ion, options, idx,
                                     iobase, NULL); 
            if (rc != EOK) {
                return (ENODEV);
            }

            if (cfg.device_index != 0xffffffff) {
                /* Only looking at a specific index */
                printf("mpc_detect(): quit TSEC %d\n", idx); // WAYNE
                break;
            }

            idx++;
        }

#else
        /* vvvv Wayne vvvv, special hack for MB LC */
        printf("mpc_detect(): config TSEC %d (0 ~ 3)\n", tsec_index);
        rc = mpc_create_instance(dll_hdl, ion, options, tsec_index,
                                 iobase, NULL); 
        if (rc != EOK) {
            return (ENODEV);
        }
#endif // WAYNE (force to config only one TSEC)
    }

    return (rc);
}
