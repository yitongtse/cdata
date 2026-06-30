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





#include	"mpc85xx.h"
#include 	<ppc/inline.h>
#include	<ppc/cpu.h>


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

uint16_t	mpc_mii_read (void *handle, uint8_t phy_add, uint8_t reg_add)

{
	mpc85xx_t           *ext = (mpc85xx_t *) handle;
	volatile uint32_t   *base = ext->phy_reg;
	int                 timeout = MPC_TIMEOUT;
	uint16_t            val;

	*(base + MPC_MIIMADD) = (MIIMADD_PHYADD(phy_add) | MIIMADD_REGADD(reg_add));

	*(base + MPC_MIICOM) = 0;
	asm ("sync");

	*(base + MPC_MIICOM) = MIICOM_READ;
	asm("sync");

	while (*(base + MPC_MIIMIND) & (MIIMIND_NOT_VALID | MIIMIND_BUSY)) {
		nanospin_ns(1000);
		if (! --timeout) {
			break;
		}
	}
	if (! timeout) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "MII read timeout");
		return (0);
	}
	val = (*(base + MPC_MIIMSTAT) & 0xffff);
	return (val);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void	mpc_mii_write (void *handle, uint8_t phy_add, uint8_t reg_add, uint16_t data)

{
	mpc85xx_t           *ext = (mpc85xx_t *) handle;
	volatile uint32_t   *base = ext->phy_reg;
	int                 timeout = MPC_TIMEOUT;

	*(base + MPC_MIIMADD) = (MIIMADD_PHYADD(phy_add) | MIIMADD_REGADD(reg_add));
	*(base + MPC_MIIMCON) = (uint32_t) data;
	asm("sync");

	while (*(base + MPC_MIIMIND) & MIIMIND_BUSY) {
		nanospin_ns (1000);
		if (! --timeout) {
			break;
		}
	}
	if (! timeout) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "MII write timeout");
	}
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void	mpc_mii_callback (void *handle, uchar_t phy, uchar_t newstate)
{
	mpc85xx_t           *ext = (mpc85xx_t *) handle;
	int                 i, mode, oldduplex;
	char                *s, tmp [10];
	nic_config_t        *cfg = &ext->cfg;
	volatile uint32_t   *base = ext->reg;

	switch (newstate) {

		case MDI_LINK_UP:
			if ((i = MDI_GetActiveMedia(ext->mdi, ext->cfg.phy_addr, &mode)) != MDI_LINK_UP) {
				if (i == MDI_LINK_DOWN) {
					if (ext->cfg.verbose) {
						nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, 	
							"devn-ext: callback GetActiveMedia returned LINK DOWN");
					}
					cfg->flags |= NIC_FLAG_LINK_DOWN;
					MDI_AutoNegotiate (ext->mdi, ext->phy_addr, NoWait);
				} else {
					if (ext->cfg.verbose) {
    					nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, 
						"devn-ext: callback GetActiveMedia returned %x", i);
						mode = 0;
					}
				}
    	     }

			oldduplex = cfg->duplex;
			cfg->duplex = 0;
			switch (mode) {
				case MDI_10bTFD:
					cfg->duplex = 1;
					cfg->media_rate = 10000;
					s = "10bT FD";
					break;
				case MDI_10bT:
					cfg->media_rate = 10000;
					s = "10bT";
					break;
				case MDI_100bTFD:
					cfg->duplex = 1;
					cfg->media_rate = 100000;
					s = "100bT FD";
					break;
				case MDI_100bT:
					cfg->media_rate = 100000;
					s = "100bT";
					break;
				case MDI_100bT4:
					cfg->media_rate = 100000;
					s = "100bT4";
					break;
				case MDI_1000bTFD:
					cfg->duplex = 1;
					cfg->media_rate = 1000000;
					s = "1000bT FD";
					break;
				case MDI_1000bT:
					cfg->media_rate = 1000000;
					s = "1000bT";
					break;
				default:
					cfg->media_rate = 0;
					sprintf (tmp, "%d", mode);
					s = tmp;
					break;
			}

			if (cfg->media_rate == 1000000) { // If its 1Gb
				*(base + MPC_MACCFG2) = ((*(base + MPC_MACCFG2) & ~MACCFG2_IF) | MACCFG2_IF_MODE_BYT);
			} else { // 10/100Mb
				*(base + MPC_MACCFG2) = ((*(base + MPC_MACCFG2) & ~MACCFG2_IF) | MACCFG2_IF_MODE_NIB);
				
				/* Check for reduced mode operation. */
				if ( *(base + MPC_ECNTRL) & (ECNTRL_RMM | ECNTRL_RPM)) {
					if (cfg->media_rate == 100000) {
						*(base + MPC_ECNTRL) |= ECNTRL_R100M;
					} else {
						*(base + MPC_ECNTRL) &= ~ECNTRL_R100M;
					}
				}
			}
         
			if (mode != ext->phy_mode) {
				ext->phy_mode = mode;
			}

			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "Link up %d (%s)", cfg->lan, s);
			}

			if (oldduplex != (cfg->duplex)) {
				if (cfg->duplex) {
					*(base + MPC_MACCFG2) |= MACCFG2_FDX;
				} else {
					*(base + MPC_MACCFG2) &= ~MACCFG2_FDX;
				}
			}
			if (cfg->media_rate == 0) {
				/* Don't know media rate, so must mark link as down. */
				cfg->flags |= NIC_FLAG_LINK_DOWN;
			} else {
				cfg->flags &= ~NIC_FLAG_LINK_DOWN;
			}
			break;

		case    MDI_LINK_DOWN:
			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "Link down %d", cfg->lan);
			}
			cfg->media_rate = cfg->duplex = -1;
			cfg->flags |= NIC_FLAG_LINK_DOWN;
			MDI_AutoNegotiate (ext->mdi, ext->phy_addr, NoWait);
			break;

		default:
			break;
	}
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

int		mpc_init_phy (mpc85xx_t *ext, int speed)

{
	int                 i, phy_idx;
	struct sigevent     mdi_event;
	nic_config_t        *cfg = &ext->cfg;
	int                 negotiate = (!speed && cfg->duplex == -1);
	int                 an_capable;
	uint16_t            reg, mcsr, mcsr1;
	uint32_t            *base = ext->reg;
	volatile uint32_t   *phy_base = ext->phy_reg;
	uint32_t            maccfg2;
	int                 timeout = MPC_TIMEOUT;

	phy_idx = ext->phy_addr;
	if (cfg->verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "mpc_init_phy: speed: %d, duplex: %d, PHY: %d", speed, cfg->duplex, ext->phy_addr);
	}
	
	memset (&mdi_event, 0, sizeof (struct sigevent));
	mdi_event.sigev_coid = ext->coid;
	MDI_Register_Extended (ext, mpc_mii_write, mpc_mii_read, mpc_mii_callback, 
		(mdi_t **) &ext->mdi, &mdi_event, 10, 3);
		
	*(base + MPC_TBIPA) = 0x1f;
	*(phy_base + MPC_MIIMCFG) = MIIMCFG_RESET;
	nanospin_ns (100000);
	*(phy_base + MPC_MIIMCFG) = 3;
	
	while ((*(phy_base + MPC_MIIMIND) & MIIMIND_BUSY)) {
		nanospin_ns(1000);
		if (! --timeout) {
			nic_slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "MII timeout during init.  Continuing anyway...");
			break;
		}
	}

	if ((i = MDI_FindPhy (ext->mdi, ext->phy_addr)) == MDI_SUCCESS) {
		if ((i = MDI_InitPhy (ext->mdi, ext->phy_addr)) == MDI_SUCCESS) {
			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: PHY found at address %d.", ext->phy_addr);
			}
	
			MDI_ResetPhy (ext->mdi, ext->phy_addr, WaitBusy);
			
			if ((PPC_GET_FAM_MEMBER(get_pvr()) == PPC_E500) && PPC_GET_REVISION(get_pvr()) < 2) {
				/* reset errata */
				if (cfg->verbose) {
					nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: Reset PHY errata being run.");
				}
				mpc_mii_write (ext, ext->phy_addr, 0x1d, 0x001f);
				mpc_mii_write (ext, ext->phy_addr, 0x1e, 0x200c);
				mpc_mii_write (ext, ext->phy_addr, 0x1d, 0x0005);
				mpc_mii_write (ext, ext->phy_addr, 0x1e, 0x0000);
				mpc_mii_write (ext, ext->phy_addr, 0x1e, 0x0100);		
			}
		} else {
			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: PHY %d not initialized (InitPhy returned %d).", ext->phy_addr, i);
			}
		}
	} else {
		if (cfg->verbose) {
			nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: PHY %d not found (FindPhy returned %d).", ext->phy_addr, i);
		}
	}

	cfg->phy_addr = ext->phy_addr;
	if (negotiate) {
		/* Mark link as down and start autonegotiation. */
		cfg->flags |= NIC_FLAG_LINK_DOWN;
		if ((i = MDI_AutoNegotiate(ext->mdi, ext->phy_addr, NoWait)) != MDI_SUCCESS) {
			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "MDI_AutoNegotiate returned %x", i);
			}
		}
		
		if ((i = MDI_EnableMonitor (ext->mdi, 1)) != MDI_SUCCESS) {
			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "MDI_EnableMonitor returned %x", i);
			}
		}
	} else {
		an_capable = mpc_mii_read (ext, cfg->phy_addr, MDI_BMSR) & 8;

		if (ext->force_advertise != -1 || !an_capable) {
			reg = mpc_mii_read (ext, cfg->phy_addr, MDI_BMCR);

			reg &= ~(BMCR_RESTART_AN | BMCR_SPEED_100 | BMCR_SPEED_1000 | BMCR_FULL_DUPLEX);

			if (an_capable && ext->force_advertise != 0) {
				/*
				 * If we force the speed, but the link partner
				 * is autonegotiating, there is a greater chance
				 * that everything will work if we advertise with
				 * the speed that we are forcing to.
				 */
				MDI_SetAdvert (ext->mdi, cfg->phy_addr, ext->force_advertise);

				reg |= BMCR_RESTART_AN | BMCR_AN_ENABLE;

				if (cfg->verbose) {
					nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: "
					    "restricted autonegotiate (%dMbps only)", cfg->media_rate / 1000);
				}
			} else {
				reg &= ~BMCR_AN_ENABLE;

				if (cfg->verbose) {
					nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "devn-mpc85xx: forcing the link");
				}
			}

			mcsr = 0;
			if (cfg->duplex > 0) {
				reg |= BMCR_FULL_DUPLEX;
			}
			if (cfg->media_rate == 100 * 1000) {
				reg |= BMCR_SPEED_100;
				an_capable = (cfg->duplex > 0) ? ANAR_100bTFD : ANAR_100bT;
			} else {
				if (cfg->media_rate == 1000 * 1000) {
					reg |= BMCR_SPEED_1000;
					an_capable = 0;
					mcsr = (cfg->duplex > 0) ? MSCR_ADV_1000bTFD : MSCR_ADV_1000bT;
				} else {
					an_capable = (cfg->duplex > 0) ? ANAR_10bTFD : ANAR_10bT;
				}
			}

			mpc_mii_write (ext, ext->phy_addr, MDI_ANAR, an_capable);
			mcsr1 = mpc_mii_read (ext, ext->phy_addr, 9);
			mcsr1 &= ~0x300;
			mcsr1 |= mcsr;
			mpc_mii_write (ext, ext->phy_addr, MDI_MSCR, mcsr1);
			/* This PHY has to be reset for the advertisement to take effect */
			reg |= BMCR_RESET;
			mpc_mii_write (ext, cfg->phy_addr, MDI_BMCR, reg);
			delay (10);

			maccfg2 = (*(base + MPC_MACCFG2) & ~(MACCFG2_IF | MACCFG2_FDX));
			if (cfg->media_rate == 1000 * 1000) {
				maccfg2 |= MACCFG2_IF_MODE_BYT;
			} else {
				maccfg2 |= MACCFG2_IF_MODE_NIB;
			}
			if (cfg->duplex > 0) {
				maccfg2 |= MACCFG2_FDX;
			}
			*(base + MPC_MACCFG2) = maccfg2;

			if (reg & BMCR_AN_ENABLE) {
				MDI_EnableMonitor (ext->mdi, 1);
			}
		}
	}
	cfg->connector = NIC_CONNECTOR_MII;

	return (phy_idx);
}
