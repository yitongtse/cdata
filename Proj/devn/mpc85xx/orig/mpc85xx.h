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



#ifndef	_MPC85XX_INCLUDED
#define	_MPC85XX_INCLUDED

#include <atomic.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <sys/mman.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/io-net.h>
#include <sys/dcmd_io-net.h>
#include <sys/slogcodes.h>
#include <sys/cache.h>
#include <sys/syspage.h>
#include <hw/inout.h>
#include <hw/sysinfo.h>
#include <ppc/85xxintr.h>
#include <ppc/83xxintr.h>

#include <drvr/mdi.h>
#include <drvr/eth.h>
#include <drvr/nicsupport.h>

#include <hw/pci.h>
#include <hw/pci_devices.h>


#define	MPC_TSEC1_BASE		0x24000
#define	MPC_TSEC2_BASE		0x25000
#define	MPC_TSEC3_BASE		0x26000
#define	MPC_TSEC4_BASE		0x27000

#define NIC_INTR_EVENT_TX	0
#define NIC_INTR_EVENT_RX	1
#define NIC_INTR_EVENT_ERR	3

#define MPC_KEEP_PACKET		0x100000
#define MPC_DEFRAG_PACKET	0x1000000

#define MPC_MTU_SIZE		1536
#define MAX_DN_FRAGMENTS	8

#define DEFAULT_NUM_RX_DESCRIPTORS  64
#define DEFAULT_NUM_TX_DESCRIPTORS  256

#define MIN_NUM_RX_DESCRIPTORS  16
#define MIN_NUM_TX_DESCRIPTORS  16
#define MAX_NUM_RX_DESCRIPTORS  2048
#define MAX_NUM_TX_DESCRIPTORS  2048

#define MPC_DEFAULT_MAX_PACKETS 200

#define	MPC_TIMEOUT		1000

/* TSEC General Control and Status Registers */

#define	MPC_IEVENT			(0x0010 >> 2)	/* Interrupt event register */
	#define	IEVENT_BABR		(1 << 31)		/* Babbling receive error */
	#define	IEVENT_RXC		(1 << 30)		/* Receive control interrupt */
	#define	IEVENT_BSY		(1 << 29)		/* Busy condition interrupt */
	#define	IEVENT_EBERR	(1 << 28)		/* Ethernet bus error */
	#define	IEVENT_MSRO		(1 << 26)		/* MSTAT register overflow */
	#define	IEVENT_GTSC		(1 << 25)		/* Graceful transmit stop complete */
	#define	IEVENT_BABT		(1 << 24)		/* Babbling transmit error */
	#define	IEVENT_TXC		(1 << 23)		/* Transmit control interrupt */
	#define	IEVENT_TXE		(1 << 22)		/* Transmit error */
	#define	IEVENT_TXB		(1 << 21)		/* Transmit buffer */
	#define	IEVENT_TXF		(1 << 20)		/* Transmit frame interrupt */
	#define	IEVENT_LC		(1 << 18)		/* Late collision */
	#define	IEVENT_CRL		(1 << 17)		/* Collision retry limit */
	#define	IEVENT_XFUN		(1 << 16)		/* Transmit FIFO underrun */
	#define	IEVENT_RXBO		(1 << 15)		/* Receive buffer */
	#define	IEVENT_GRSC		(1 << 8)		/* Graceful receive stop complete */
	#define	IEVENT_RXFO		(1 << 7)		/* Receive frame interrupt */

#define	MPC_IMASK			(0x0014 >> 2)	/* Interrupt mask register */
	#define	IMASK_BABR		(1 << 31)		/* Babbling receive error */
	#define	IMASK_RXC		(1 << 30)		/* Receive control interrupt */
	#define	IMASK_BSY		(1 << 29)		/* Busy condition interrupt */
	#define	IMASK_EBERR		(1 << 28)		/* Ethernet bus error */
	#define	IMASK_MSRO		(1 << 26)		/* MSTAT register overflow */
	#define	IMASK_GTSC		(1 << 25)		/* Grace transmit stop complete */
	#define	IMASK_BABT		(1 << 24)		/* Babbling transmit error */
	#define	IMASK_TXC		(1 << 23)		/* Transmit control interrupt */
	#define	IMASK_TXE		(1 << 22)		/* Transmit error */
	#define	IMASK_TXB		(1 << 21)		/* Transmit buffer */
	#define	IMASK_TXF		(1 << 20)		/* Transmit frame interrupt */
	#define	IMASK_LC		(1 << 18)		/* Late collision */
	#define	IMASK_CRL		(1 << 17)		/* Collision retry limit */
	#define	IMASK_XFUN		(1 << 16)		/* Transmit FIFO underrun */
	#define	IMASK_RXBO		(1 << 15)		/* Receive buffer */
	#define	IMASK_GRSC		(1 << 8)		/* Graceful receive stop complete */
	#define	IMASK_RXFO		(1 << 7)		/* Receive frame interrupt */

#define	MPC_EDIS			(0x0018 >> 2)	/* Error disable register */
	#define	EDIS_BSYDIS		(1 << 29)		/* Busy disable */
	#define	EDIS_EBERRDIS	(1 << 28)		/* Ethernet controller bus error disable */
	#define	EDIS_TXEDIS		(1 << 22)		/* Transmit error disable */
	#define	EDIS_LCDIS		(1 << 18)		/* Late collision disable */
	#define	EDIS_CRLDIS		(1 << 17)		/* Collision retry limit disable */
	#define	EDIS_XFUNDIS	(1 << 16)		/* Transmit fifo underrun disable */

#define	MPC_ECNTRL			(0x0020 >> 2)	/* Ethernet Control register */
	#define	ECNTRL_CLRCNT	(1 << 14)		/* Clear all statistics counters */
	#define	ECNTRL_AUTOZ	(1 << 13)		/* Automatically zero statistics counters */
	#define	ECNTRL_STEN		(1 << 12)		/* Stastics enabled */
	#define	ECNTRL_TBIM		(1 << 5)		/* Ten-bit interface mode */
	#define	ECNTRL_RPM		(1 << 4)		/* Reduced pin mode */
	#define	ECNTRL_R100M	(1 << 3)		/* RGMII 100 mode */
	#define ECNTRL_RMM		(1 << 2)		/* Reduced pin mode for 10/100 */
	
#define	MPC_MINFLR			(0x0024 >> 2)	/* Minimum frame length register */

#define	MPC_PTV				(0x0028 >> 2)	/* Pause time value register */

#define	MPC_DMACTRL			(0x002c >> 2)	/* DMA control register */
	#define	DMACTRL_TDSEN	(1 << 7)		/* Tx Data snoop enable */
	#define	DMACTRL_TBDSEN	(1 << 6)		/* TxBD snoop enable */
	#define	DMACTRL_GRS		(1 << 4)		/* Graceful receive stop */
	#define	DMACTRL_GTS		(1 << 3)		/* Graceful transmit stop */
	#define	DMACTRL_TOD		(1 << 2)		/* Transmit on demand */
	#define	DMACTRL_WWR		(1 << 1)		/* Write with response */
	#define	DMACTRL_WOP		(1 << 0)		/* Wait or poll */

#define	MPC_TBIPA			(0x0030 >> 2)	/* TBI PHY address register */

/* TSEC FIFO Control and Status Registers */

#define	MPC_FIFO_TX_THR		(0x008c >> 2)	/* FIFO transmit threshold register */
#define	MPC_FIFO_TX_STARVE	(0x0098 >> 2)	/* FIFO transmit starve register */
#define	MPC_FIFO_TX_STARVE_SHUTOFF	(0x009c >> 2)	/* FIFO transmit starve shutoff register */

/* TSEC Transmit Control and Status Registers */

#define	MPC_TCTRL			(0x0100 >> 2)	/* Transmit control register */
	#define	TCTRL_THDF		(1 << 11)		/* Transmit half-duplex flow control */
	#define	TCTRL_RFC_PAUSE	(1 << 4)		/* Receive flow control pause frame */
	#define	TCTRL_TFC_PAUSE	(1 << 3)		/* Transmit flow control pause frame */

#define	MPC_TSTAT			(0x0104 >> 2)	/* Transmit status register */
	#define	TSTAT_THLT		(1 << 31)		/* Transmit halt */

#define MPC_TBDLEN			(0x010c >> 2)	/* TxBD data length */

#define MPC_CTBPTR			(0x0124 >> 2)	/* Current TxBD pointer */

#define MPC_TBPTR			(0x0184 >> 2)	/* TxBD pointer */

#define MPC_TBASE			(0x0204 >> 2)	/* TxBD base address */

#define MPC_OSTBD			(0x02b0 >> 2)	/* Out-of-sequence TxBD register */
	#define	OSTBD_R			(1 << 31)		/* Ready */
	#define	OSTBD_PAD		(1 << 30)		/* Padding for short frames */
	#define	OSTBD_W			(1 << 29)		/* Wrap */
	#define	OSTBD_I			(1 << 28)		/* Interrupt */
	#define	OSTBD_L			(1 << 27)		/* Last in frame */
	#define	OSTBD_TC		(1 << 26)		/* Tx CRC */
	#define	OSTBD_DEF		(1 << 25)		/* Defer indication */
	#define	OSTBD_LC		(1 << 23)		/* Late collision */
	#define	OSTBD_RL		(1 << 22)		/* Retransmission limit */
	#define	OSTBD_RC(x)		((x & 0x0f) << 18)	/* Retry count */
	#define	OSTBD_UN		(1 << 17)		/* Underrun */

#define MPC_OSTBDP			(0x02b4 >> 2)	/* Out-of-sequence Tx data buffer pointer register */

/* TSEC Receive Control and Status Registers */

#define MPC_RCTRL			(0x0300 >> 2)	/* Receive control register */
	#define	RCTRL_BC_REJ	(1 << 4)		/* Broadcast frame reject */
	#define	RCTRL_PROM		(1 << 3)		/* Promiscuous mode */
	#define	RCTRL_RSF		(1 << 2)		/* Receive short frame mode */

#define MPC_RSTAT			(0x0304 >> 2)	/* Receive status register */
	#define	RSTAT_QHLT		(1 << 23)		/* RxBD queue is halted */

#define MPC_RBDLEN			(0x030c >> 2)	/* RxBD data length */
#define MPC_CRBPTR			(0x0324 >> 2)	/* Current RxBD pointer */
#define MPC_MRBLR			(0x0340 >> 2)	/* Maximum receive buffer length register */
#define MPC_RBPTR			(0x0384 >> 2)	/* RxBD pointer */
#define MPC_RBASE			(0x0404 >> 2)	/* RxBD base address */

/* TSEC MAC Registers */

#define MPC_MACCFG1			(0x0500 >> 2)	/* MAC Configuration #1 */
	#define	MACCFG1_SFT_RESET	(1 << 31)	/* Soft reset */
	#define	MACCFG1_RST_RXMC	(1 << 19)	/* Reset receive MAC control block */
	#define	MACCFG1_RST_TXMC	(1 << 18)	/* Reset transmit MAC control block */
	#define	MACCFG1_RST_RXFUN	(1 << 17)	/* Reset receive function block */
	#define	MACCFG1_RST_TXFUN	(1 << 16)	/* Reset transmit function block */
	#define	MACCFG1_LP_BACK		(1 << 8)	/* Loop back */
	#define	MACCFG1_RX_FLOW		(1 << 5)	/* Receive flow */
	#define	MACCFG1_TX_FLOW		(1 << 4)	/* Transmit flow */
	#define	MACCFG1_SYNC_RXEN	(1 << 3)	/* Receive enable synchronized */
	#define	MACCFG1_RXEN		(1 << 2)	/* Receive enable */
	#define	MACCFG1_SYNC_TXEN	(1 << 1)	/* Transmit enable synchronized */
	#define	MACCFG1_TXEN		(1 << 0)	/* Transmit enable */

#define MPC_MACCFG2			(0x0504 >> 2)	/* MAC Configuration #2 */
	#define	MACCFG2_PRE_LEN(x)	((x & 0x0f) << 12)	/* Preamble length */
	#define	MACCFG2_IF_MODE_NIB	(1 << 8)	/* Nibble mode (MII) */
	#define	MACCFG2_IF_MODE_BYT	(2 << 8)	/* Byte mode (TBI) */
	#define	MACCFG2_IF			(MACCFG2_IF_MODE_NIB | MACCFG2_IF_MODE_BYT)
	#define	MACCFG2_HUGE_FRM	(1 << 5)	/* Huge frame enable */
	#define	MACCFG2_LCHECK		(1 << 4)	/* Length check */
	#define	MACCFG2_PAD_CRC		(1 << 2)	/* Pad and append CRC */
	#define	MACCFG2_CRC_EN		(1 << 1)	/* CRC enable */
	#define	MACCFG2_FDX			(1 << 0)	/* Full duplex configure */

#define	MPC_IPGIFGI			(0x0508 >> 2)	/* Inter packet / frame gap */

#define MPC_HAFDUP			(0x050c >> 2)	/* Half-duplex */

#define MPC_MAXFRM			(0x0510 >> 2)	/* Maximum frame */

#define MPC_MIIMCFG			(0x0520 >> 2)	/* MII Mgmt: configuration */
	#define	MIIMCFG_RESET	(1 << 31)		/* Reset management */
	#define	MIIMCFG_NOPRE	(1 << 4)		/* Preamble suppress */

#define MPC_MIICOM			(0x0524 >> 2)	/* MII Mgmt: command */
	#define	MIICOM_SCAN		(1 << 1)		/* Scan cycle */
	#define	MIICOM_READ		(1 << 0)		/* Read cycle */

#define MPC_MIIMADD			(0x0528 >> 2)	/* MII Mgmt: address */
	#define	MIIMADD_PHYADD(x)	((x & 0x1f) << 8)	/* PHY address */
	#define	MIIMADD_REGADD(x)	(x & 0x1f)	/* Register address */

#define MPC_MIIMCON			(0x052c >> 2)	/* MII Mgmt: control */

#define MPC_MIIMSTAT		(0x0530 >> 2)	/* MII Mgmt: status */

#define MPC_MIIMIND			(0x0534 >> 2)	/* MII Mgmt: indicators */
	#define	MIIMIND_NOT_VALID (1 << 2)		/* Not valid */
	#define	MIIMIND_SCAN	(1 << 1)		/* Scan in progress */
	#define	MIIMIND_BUSY	(1 << 0)		/* Busy */
#define	MARV_STAT			0x11			/* Marvell 88E1011S PHY status register */

#define MPC_IFSTAT			(0x053c >> 2)	/* Interface status */
	#define	IFSTAT_EX_DEFER	(1 << 9)		/* Excessive transmission defer */

#define MPC_MACSTNADDR1		(0x0540 >> 2)	/* Station address, part 1 */

#define MPC_MACSTNADDR2		(0x0544 >> 2)	/* Station address, part 2 */

/* MIB Registers */

#define	MPC_TR64			(0x0680 >> 2)	/* Transmit/Receive 64-Byte Frame counter */
#define	MPC_TR127			(0x0684 >> 2)	/* Transmit/Receive 65-127-Byte Frame counter */
#define	MPC_TR255			(0x0688 >> 2)	/* Transmit/Receive 128-255-Byte Frame counter */
#define	MPC_TR511			(0x068c >> 2)	/* Transmit/Receive 256-511-Byte Frame counter */
#define	MPC_TR1K			(0x0690 >> 2)	/* Transmit/Receive 512-1023-Byte Frame counter */
#define	MPC_TRMAX			(0x0694 >> 2)	/* Transmit/Receive 1024-1518-Byte Frame counter */
#define	MPC_TRMGV			(0x0698 >> 2)	/* Transmit/Receive 1519-1522-Byte VLAN Frame counter */

/* TSEC Receive counters */

#define MPC_RBYT			(0x069c >> 2)	/* Receive byte counter */
#define MPC_RPKT			(0x06a0 >> 2)	/* Receive packet counter */
#define MPC_RFCS			(0x06a4 >> 2)	/* Receive FCS error counter */
#define MPC_RMCA			(0x06a8 >> 2)	/* Receive multicast packet counter */
#define MPC_RBCA			(0x06ac >> 2)	/* Receive broadcast packet counter */
#define MPC_RXCF			(0x06b0 >> 2)	/* Receive control frame packet counter */
#define MPC_RXPF			(0x06b4 >> 2)	/* Receive PAUSE frame packet counter */
#define MPC_RXUO			(0x06b8 >> 2)	/* Receive unknown OP code counter */
#define MPC_RALN			(0x06bc >> 2)	/* Receive alignment error counter */
#define MPC_RFLR			(0x06c0 >> 2)	/* Receive frame length error counter */
#define MPC_RCDE			(0x06c4 >> 2)	/* Receive code error counter */
#define MPC_RCSE			(0x06c8 >> 2)	/* Receive carrier sense error counter */
#define MPC_RUND			(0x06cc >> 2)	/* Receive undersize packet counter */
#define MPC_ROVR			(0x06d0 >> 2)	/* Receive oversize packet counter */
#define MPC_RFRG			(0x06d4 >> 2)	/* Receive fragments counter */
#define MPC_RJBR			(0x06d8 >> 2)	/* Receive jabber counter */
#define MPC_RDRP			(0x06dc >> 2)	/* Receive drop */

/* TSEC Transmit counters */

#define MPC_TBYT			(0x06e0 >> 2)	/* Transmit byte counter */
#define MPC_TPKT			(0x06e4 >> 2)	/* Transmit packet counter */
#define MPC_TMCA			(0x06e8 >> 2)	/* Transmit multicast packet counter */
#define MPC_TBCA			(0x06ec >> 2)	/* Transmit broadcast packet counter */
#define MPC_TXPF			(0x06f0 >> 2)	/* Transmit PAUSE control frame counter */
#define MPC_TDFR			(0x06f4 >> 2)	/* Transmit deferral packet counter */
#define MPC_TEDF			(0x06f8 >> 2)	/* Transmit excessive deferral packet counter */
#define MPC_TSCL			(0x06fc >> 2)	/* Transmit single collision packet counter */
#define MPC_TMCL			(0x0700 >> 2)	/* Transmit multiple collision packet counter */
#define MPC_TLCL			(0x0704 >> 2)	/* Transmit late collision packet counter */
#define MPC_TXCL			(0x0708 >> 2)	/* Transmit excessive collision packet counter */
#define MPC_TNCL			(0x070c >> 2)	/* Transmit total collision counter */
#define MPC_TDRP			(0x0714 >> 2)	/* Transmit drop frame counter */
#define MPC_TJBR			(0x0718 >> 2)	/* Transmit jabber frame counter */
#define MPC_TFCS			(0x071c >> 2)	/* Transmit FCS error counter */
#define MPC_TXCF			(0x0720 >> 2)	/* Transmit control frame counter */
#define MPC_TOVR			(0x0724 >> 2)	/* Transmit oversize frame counter */
#define MPC_TUND			(0x0728 >> 2)	/* Transmit undersize frame counter */
#define MPC_TFRG			(0x072c >> 2)	/* Transmit fragments frame counter */

#define	MPC_CAR1			(0x0730 >> 2)	/* Carry Register One */
#define	MPC_CAR2			(0x0734 >> 2)	/* Carry Register Two */
#define	MPC_CAM1			(0x0738 >> 2)	/* Carry Mask Register One */
#define	MPC_CAM2			(0x073c >> 2)	/* Carry Mask Register Two */

/* Hash Function Registers */

#define MPC_IADDR0			(0x0800 >> 2)	/* Individual address register 0 */
#define MPC_IADDR1			(0x0804 >> 2)	/* Individual address register 1 */
#define MPC_IADDR2			(0x0808 >> 2)	/* Individual address register 2 */
#define MPC_IADDR3			(0x080c >> 2)	/* Individual address register 3 */
#define MPC_IADDR4			(0x0810 >> 2)	/* Individual address register 4 */
#define MPC_IADDR5			(0x0814 >> 2)	/* Individual address register 5 */
#define MPC_IADDR6			(0x0818 >> 2)	/* Individual address register 6 */
#define MPC_IADDR7			(0x081c >> 2)	/* Individual address register 7 */
#define	MPC_GADDR0			(0x0880 >> 2)	/* Group address register 0 */
#define	MPC_GADDR1			(0x0884 >> 2)	/* Group address register 1 */
#define	MPC_GADDR2			(0x0888 >> 2)	/* Group address register 2 */
#define	MPC_GADDR3			(0x088c >> 2)	/* Group address register 3 */
#define	MPC_GADDR4			(0x0890 >> 2)	/* Group address register 4 */
#define	MPC_GADDR5			(0x0894 >> 2)	/* Group address register 5 */
#define	MPC_GADDR6			(0x0898 >> 2)	/* Group address register 6 */
#define	MPC_GADDR7			(0x089c >> 2)	/* Group address register 7 */
#define MPC_ATTR			(0x0bf8 >> 2)	/* Attribute register */
#define MPC_ATTRELI			(0x0bfc >> 2)	/* Attribute EL & EI register */

/* Transmit/receive buffer descriptor */

typedef	struct {
	uint16_t	status;
	uint16_t	length;
	uint32_t	buffer;
} mpc_bd_t;

#define	TXBD_R				(1 << 15)		/* Ready */
#define	TXBD_PAD_CRC		(1 << 14)		/* PAD/CRC */
#define	TXBD_W				(1 << 13)		/* Wrap */
#define	TXBD_I				(1 << 12)		/* Interrupt */
#define	TXBD_L				(1 << 11)		/* Last */
#define	TXBD_TC				(1 << 10)		/* Tx CRC */
#define	TXBD_DEF			(1 << 9)		/* Defer Indication */
#define	TXBD_HFE_LC			(1 << 7)		/* Huge Frame Enable/ Late collision */
#define	TXBD_RL				(1 << 6)		/* Retransmission limit */
#define	TXBD_RC(x)			((x >> 2) & 0x0f)	/* Retry count */
#define	TXBD_UN				(1 << 1)		/* Underrun */

#define	RXBD_E				(1 <<15)		/* Empty */
#define	RXBD_RO1			(1 << 14)		/* Receive software ownership bit */
#define	RXBD_W				(1 << 13)		/* Wrap */
#define	RXBD_I				(1 << 12)		/* Interrupt */
#define	RXBD_L				(1 << 11)		/* Last in frame */
#define	RXBD_F				(1 << 10)		/* First in frame */
#define	RXBD_M				(1 << 8)		/* Miss */
#define	RXBD_BC				(1 << 7)		/* Broadcast */
#define	RXBD_MC				(1 << 6)		/* Multicast */
#define	RXBD_LG				(1 << 5)		/* Rx frame length violation */
#define	RXBD_NO				(1 << 4)		/* Rx non-octet aligned frame */
#define	RXBD_SH				(1 << 3)		/* Short frame */
#define	RXBD_CR				(1 << 2)		/* Rx CRC error */
#define	RXBD_OV				(1 << 1)		/* Overrun */
#define	RXBD_TR				(1 << 0)		/* Truncation */
#define	RXBD_ERR			(RXBD_TR | RXBD_OV | RXBD_CR | RXBD_SH | RXBD_NO | RXBD_LG)

#define	NEXT_TX(x)		((x + 1) % ext->num_tx_descriptors)
#define	NEXT_RX(x)		((x + 1) % ext->num_rx_descriptors)
#define	PREV_TX(x)		((x == 0) ? ext->num_tx_descriptors - 1 : x - 1)

typedef	struct {
	nic_config_t	cfg;
	nic_stats_t		stats;
	int				chid;
	int 			coid;
	int 			tid;
	int 			iid [3];
	int				version;
	uintptr_t		iobase;
	uintptr_t		phy_base;
	uint32_t		*reg;
	uint32_t		*phy_reg;

	io_net_self_t	*ion;
	void			*dll_hdl;
	int				reg_hdl;
	uint16_t		cell;
	uint16_t		_pad1;
	int             io_static;
	
	uint32_t        flowctl_flag;
	uint32_t        rcvpkts;
	uint32_t		loopback;
	int				phy_mode;
	int				phy_addr;
	int				phy_incr;
	int				data_rate;
	mdi_t			*mdi;
	int				mdi_active;
	int				force_advertise;
	int				max_pkts;
	int				tx_q_len;
	int				num_tx_descriptors;
	npkt_t			**tx_pktq;
	mpc_bd_t		*tx_bd;
	int				tx_pidx;
	int 			tx_cidx;
	pthread_mutex_t	tx_mutex;
	uint32_t		*tx_addr_table;
	unsigned int	tx_pkts_queued;
	npkt_t			*rx_free_pkt_q;
	int			 	num_rx_descriptors;
	int				num_rx_free;
	int				rx_cidx;
	npkt_t			**rx_pktq;
	mpc_bd_t		*rx_bd;
	pthread_mutex_t	rx_free_pkt_q_mutex;
	int 			mcast_nprom;	    // number of times we have been asked 
										// to enable promis mcast
	uint32_t		gaddr[8];			// our in-memory copy of 256 hash bits
	uint32_t		fifo;					// FIFO start level
	uint32_t		fifo_starve;			// FIFO enter starve mode level
	uint32_t		fifo_starve_shutoff; 	// FIFO stop starve mode level
	uint32_t		probe_phy;				// Whether or not the PHY should be
											// constantly monitored for speed /
											// duplex change.
	uint32_t		use_syspage;			// Whether or not the system page should 
											// be read for TSEC hardware 
											// information.
} mpc85xx_t;

typedef struct {
	void            *buf_head;
	unsigned        ref;
} mpc85xx_pool_t;

/*
 * standard io-net macros
 */
#define ion_rx_packets          ext->ion->tx_up
#define ion_register            ext->ion->reg
#define ion_deregister          ext->ion->dereg
#define ion_alloc               ext->ion->alloc
#define ion_free                ext->ion->free
#define ion_alloc_npkt          ext->ion->alloc_up_npkt
#define ion_mphys               ext->ion->mphys
#define ion_tx_complete         ext->ion->tx_done
#define ion_add_done            ext->ion->reg_tx_done

#ifndef TAILQ_EMPTY
#define TAILQ_EMPTY(head) ((head)->tqh_first == NULL)
#endif
#ifndef TAILQ_FIRST
#define TAILQ_FIRST(head) ((head)->tqh_first)
#endif
#ifndef TAILQ_LAST
#define TAILQ_LAST(head) ((head)->tqh_last)
#endif
#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)
#endif
#ifndef TAILQ_PREV
#define TAILQ_PREV(elm, field) ((elm)->field.tqe_prev)
#endif

/*==============================================================================
 *
 * function prototypes by file 
 */

/* shutdown.c */
int mpc_shutdown1 (int, void *);
int mpc_shutdown2 (int, void *);

/* devctl.c */
int mpc_devctl (void *hdl, int dcmd, void *data, size_t size, union _io_net_dcmd_ret_cred *ret);

/* receive.c */
int mpc_receive_complete (npkt_t *npkt, void *, void *);
void mpc_receive (mpc85xx_t *ext);

/* transmit.c */
int mpc_send_packets (npkt_t *npkt, void *);
int mpc_download_complete (mpc85xx_t *ext);
int mpc_send (mpc85xx_t *ext, npkt_t *npkt);
int mpc_reap_pkts (mpc85xx_t *ext, int max_reap);
int mpc_flush (int, void *);


/* mpc.c */
int mpc_init (void *, dispatch_t *, io_net_self_t *, char *);
int mpc_advertise (int reg_hdl, void *func_hdl);
npkt_t *mpc_alloc_npkt (mpc85xx_t *ext, size_t size, int num);
void mpc_reset (mpc85xx_t *ext);
int mpc_register_device (mpc85xx_t *ext, io_net_self_t *, void *);
int mpc_detect (void *, io_net_self_t *, char *options);
int	mpc_init_phy (mpc85xx_t *ext, int speed);
int mpc_set_duplex (mpc85xx_t *ext, int on);
int	do_multicast (mpc85xx_t *ext, io_net_msg_join_mcast_t *msg);
void mpc_update_stats (mpc85xx_t *ext);

#endif
