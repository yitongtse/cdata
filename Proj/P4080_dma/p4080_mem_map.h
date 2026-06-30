#ifndef __P4080_MEM_MAP_H__
#define __P4080_MEM_MAP_H__



/***********************************************************
 *
 *  CCSR Base and Size
 *
 **********************************************************/

#define P4080_CCSR_BASE     0xFE000000
#define P4080_CCSR_SIZE     0x01000000    // 16 Mbyte



/***********************************************************
 *
 *  CCSR Base Reg
 *
 **********************************************************/
#define P4080_CCSRBARH      (0x0000 >> 2)
#define P4080_CCSRBARL      (0x0004 >> 2)



/***********************************************************
 *
 *  DMA Controller 1
 *
 **********************************************************/

/***** DMA Controller 1 Block Base Address (0x10_0000) *****/
#define P4080_DMA0_BASE     0x100100
#define P4080_DMA1_BASE     (P4080_DMA0_BASE + 0x180)
#define P4080_DMA2_BASE     (P4080_DMA0_BASE + 0x200)
#define P4080_DMA3_BASE     (P4080_DMA0_BASE + 0x280)
#define P4080_DMAn_BASE(n)  (P4080_DMA0_BASE + ((n) & 0x3) * 0x80)


/***** DMA General Status Register *****/
#define P4080_DMA_DGSR      ((P4080_DMA0_BASE + 0x0300) >> 2)
#define DMA_DGSR(reg, n)    (((reg) & 0xff) >> ((3 - ((n) & 0x3)) * 8))
#define DMA_DGSR_MASK       0xbf
#define DMA_DGSR_TE         BIT7 // Transfer error
#define DMA_DGSR_CH         BIT5 // Channel halted
#define DMA_DGSR_PE         BIT4 // Programming error
#define DMA_DGSR_EOLNI      BIT3 // End-of-links interrupt
#define DMA_DGSR_CB         BIT2 // Channel busy
#define DMA_DGSR_EOSI       BIT1 // End-of-segment interrupt
#define DMA_DGSR_EOLSI      BIT0 // End-of-lists/direct interrupt


/***** Mode Registers *****/
#define P4080_DMA_MR        (0x00 >> 2)
#define DMA_MR_BWC(v)       (((v) & 0xf) << (31 - 7)) // BW/Pause control
#define DMA_MR_EMP_EN       (1 << (31 - 10)) // External master pause enab
#define DMA_MR_EMS_EN       (1 << (31 - 13)) // External master start enab
#define DMA_MR_DAHTS(v)     (((v) & 0x3) << (31 - 15)) // Dst addr hold Tx size
#define DMA_MR_SAHTS(v)     (((v) & 0x3) << (31 - 17)) // Src addr hold Tx size
#define DMA_MR_DAHE         (1 << (31 - 18)) // Destination addr hold enable
#define DMA_MR_SAHE         (1 << (31 - 19)) // Source addr hold enable
#define DMA_MR_SRW          (1 << (31 - 21)) // Single register write
#define DMA_MR_EOSIE        (1 << (31 - 22)) // End-of-segments interrupt enab
#define DMA_MR_EOLNIE       (1 << (31 - 23)) // End-of-links interrupt enable
#define DMA_MR_EOLSIE       (1 << (31 - 24)) // End-of-lists interrupt enable
#define DMA_MR_EIE          (1 << (31 - 25)) // Error interrupt enable
#define DMA_MR_XFE          (1 << (31 - 26)) // Extended features enable
#define DMA_MR_CDSM_SWSM    (1 << (31 - 27)) // ... too complicated, see manual
#define DMA_MR_CA           (1 << (31 - 28)) // Channel abort
#define DMA_MR_CTM          (1 << (31 - 29)) // Channel transfer mode
#define DMA_MR_CC           (1 << (31 - 30)) // Channel continue
#define DMA_MR_CS           (1 << (31 - 31)) // Channel start


/***** Status Registers *****/
#define P4080_DMA_SR        (0x04 >> 2)
#define DMA_SR_TE           (1 << (31 - 24)) // Transfer error
#define DMA_SR_CH           (1 << (31 - 26)) // Channel halted
#define DMA_SR_PE           (1 << (31 - 27)) // Programming error
#define DMA_SR_EOLNI        (1 << (31 - 28)) // End-of-links interrupt
#define DMA_SR_CB           (1 << (31 - 29)) // Channel busy
#define DMA_SR_EOSI         (1 << (31 - 30)) // End-of-segment interrupt
#define DMA_SR_EOLSI        (1 << (31 - 31)) // End-of-list interrupt


/***** Current Link Descriptor Extended Address Register *****/
#define P4080_DMA_ECLNDAR   (0x08 >> 2)
#define DMA_ECLNDA(a)       ((a) & 0xf) // Current link descriptor ext addr


/***** Current link Descriptor Address Register *****/
#define P4080_DMA_CLNDAR    (0x0C >> 2)
#define DMA_CLNDA(a)        ((a) & ~0x1f) // Current link descriptor address
#define DMA_EOSIE           (1 << (31 - 28)) // End-of-segment IRQ enable


/***** Source Attributes Register *****/
#define P4080_DMA_SATR      (0x10 >> 2)
#define DMA_SATR_SBPATMU    (1 << (31 - 2)) // Source Bypass ATMU (RapidIO)
#define DMA_SATR_STFLOWLVL(v)  (((v) & 0x3) << (31 - 5)) // RapidIO flow level
#define DMA_SATR_SPCIORDER  (1 << (31 - 6)) // Follow PCI trans ordering rules
#define DMA_SATR_SSME       (1 << (31 - 7)) // Source stride mode enable
#define DMA_SATR_STRANSINT(v)  (((v) & 0xf) << (31 - 11)) // Src tran interface
#define DMA_SATR_SREADTTYPE(v) (((v) & 0xf) << (31 - 15)) // Src tran type
#define DMA_SATR_ESAD(v)    ((v) & 0x3ff) // Extended source address


/***** Source Address Register *****/
#define P4080_DMA_SAR       (0x14 >> 2)


/***** Destination Attributes Register *****/
#define P4080_DMA_DATR      (0x18 >> 2)
#define DMA_DATR_DBPATMU    (1 << (31 - 2)) // Bypass ATMU for this DMA oper
#define DMA_DATR_DTFLOWLVL(v)  (((v) & 0x3) << (31 - 5)) // RapidIO flow level
#define DMA_DATR_DPCIORDER  (1 << (31 - 6)) // PCI trans ordering rules enab
#define DMA_DATR_DSME       (1 << (31 - 7)) // Dest stride mode enable
#define DMA_DATR_DTRANSINT(v)  (((v) & 0xf) << (31 - 11)) // Dst tran interface
#define DMA_DATR_DWRITETYPE(v) (((v) & 0xf) << (31 - 15)) // Dst tran type
#define DMA_DATR_EDAD(v)    ((v) & 0x3ff) // Extended destination address


/***** Destination Address Register *****/
#define P4080_DMA_DAR       (0x1C >> 2)


/***** Byte Count Register *****/
#define P4080_DMA_BCR       (0x20 >> 2)
#define DMA_BCR_BC(v)       (((v) & 0x3ffffff) // number of bytes to transfer


/***** Next Link Descriptor Extended Address Register *****/
#define P4080_DMA_ENLNDAR     (0x24 >> 2)
#define DMA_ENLNDAR_ENLNDA(v) ((v) & 0xf) // Next link dscrptr ext addr bits


/***** Next Link Descriptor Address Register *****/
#define P4080_DMA_NLNDAR    (0x28 >> 2)
#define DMA_NLNDAR_NLNDA(v) (((v) & 0x7ffffff) << (31 - 26)) // Nxt Lnk Dsp adr
#define DMA_NLNDAR_NDEOSIE  (1 << (31 - 28)) // Nxt Dsp end-of-segment IRQ enab
#define DMA_NLNDAR_EOLND    (1 << (31 - 31)) // End-of-links descriptor


/***** Current List Descriptor Extended Address Register *****/
#define P4080_DMA_ECLSDAR     (0x30 >> 2)
#define DMA_ECLSDAR_ECLSDA(v) ((v) & 0xf) // Current list dscrptr ext addr bit


/***** Current List Descriptor Address Register *****/
#define P4080_DMA_CLSDAR    (0x34 >> 2)
#define DMA_CLSDAR_CLSDA(v) (((v) & 0x7ffffff) << (31 - 26)) // Cur Lst Dsp adr


/***** Next List Descriptor Extended Address Register *****/
#define P4080_DMA_ENLSDAR     (0x38 >> 2)
#define DMA_ENLSDAR_ENLSDA(v) ((v) & 0xf) // Next list dscrptr ext addr bits


/***** Next List Descriptor Address Register *****/
#define P4080_DMA_NLSDAR    (0x3C >> 2)
#define DMA_NLSDAR_NLSDA(v) (((v) & 0x7ffffff) << (31 - 26)) // Nxt Lst Dsp adr
#define DMA_NLSDAR_EOLSD    (1 << (31 - 31)) // End-of-lists descriptor


/***** Source Stride Register *****/
#define P4080_DMA_SSR       (0x40 >> 2)
#define DMA_SSR_SSS(v)      (((v) & 0xfff) << (31 - 19)) // Source stride size
#define DMA_SSR_SSD(v)      (((v) & 0xfff) << (31 - 31)) // Source stride dist


/***** Destination Stride Register *****/
#define P4080_DMA_DSR       (0x44 >> 2)
#define DMA_DSR_DSS(v)      (((v) & 0xfff) << (31 - 19)) // Dest stride size
#define DMA_DSR_DSD(v)      (((v) & 0xfff) << (31 - 31)) // Dest stride dist



#endif // __P4080_DMA_H__
