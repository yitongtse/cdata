#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <hw/inout.h>

#include "common.h"
#include "p4080_mem_map.h"
#include "p4080_util.h"



/***********************************************************
 *
 *  CCSR Base mem map
 *
 **********************************************************/
uint32 *ccsr_reg_mmap (void)
{
    uint32 *ccsr_base;

    ccsr_base = (uint32 *) mmap_device_memory((void *) NULL, P4080_CCSR_SIZE,
                                              (PROT_READ | PROT_WRITE | PROT_NOCACHE),
                                              0, P4080_CCSR_BASE);
    if (ccsr_base == (uint32 *) MAP_FAILED) {
        fprintf(stderr,
                "%s(): faile to mmap CCSR, phy addr 0x%08x, size 0x%08x\n",
                __FUNCTION__, P4080_CCSR_BASE, P4080_CCSR_SIZE);
        assert(0);
    }

    printf("%s(): CCSR Base: phy 0x%08x, vrt 0x%08x\n",
           __FUNCTION__, P4080_CCSR_BASE, (uint32) ccsr_base);
    return ccsr_base;
}



/***********************************************************
 *
 *  mmap tool (will return logical addr and phy addr
 *
 **********************************************************/
int32 mmap_tool (uint32 buff_size,
                 uint32 *buff_addr, uint32 *buff_addr_phy)
{
    uint32 addr, addr_phy;
    int32  rslt;

    addr = (uint32) mmap(NULL, buff_size,
                         PROT_READ | PROT_WRITE /*| PROT_NOCACHE */,
                         MAP_ANON | MAP_PHYS, NOFD, 0);
    if (addr == (uint32) MAP_FAILED) {
        fprintf(stderr, "%s(): fail to mmap %d byte memory\n",
                __FUNCTION__, buff_size);
        assert(0);
    }

    // get the physical address //                                                
    rslt = mem_offset((void *) addr, NOFD, buff_size, (off_t *) &addr_phy,
                      NULL);
    if (rslt == -1) {
        fprintf(stderr, "%s(): fail to get phy addr\n", __FUNCTION__);
        assert(0);
    }

    *buff_addr     = addr;
    *buff_addr_phy = addr_phy;
    return 1;
}



/***********************************************************
 *
 *  Mem continuous check
 *
 **********************************************************/
uint32 mem_contig_check (uint8 *addr_0, uint32 mem_size)
{
#define PAGESIZE  4096
    uint32 addr_phy_0, addr_phy_old, addr_phy;
    uint8  *addr;
    int n, seg_numb, size, frag;

    mem_offset(addr_0, NOFD, mem_size, (off_t *) &addr_phy_0, NULL);
    seg_numb = (mem_size + PAGESIZE - 1) / PAGESIZE;

    frag = 0;
    addr_phy_old = addr_phy_0;
    for (n = 1; n < seg_numb; ++n) {
        addr = addr_0 + (n * PAGESIZE);

        size = mem_size - (n * PAGESIZE);
        if (size > PAGESIZE) {
            size = PAGESIZE;
        }
        mem_offset(addr, NOFD, size, (off_t *) &addr_phy, NULL);

        // printf("seg %d: head 0x%08x, (prev seg %d: tail 0x%08x)",
        //        n, addr_phy, n - 1, (addr_phy_old + PAGESIZE));
        if (addr_phy != (addr_phy_old + PAGESIZE)) {
            frag++;
            fprintf(stderr, "%s(): memory is broken at %dth PAGE",
                    __FUNCTION__, n);
        }
        // printf("\n");

        addr_phy_old = addr_phy;
    }

    if (frag) {
        printf("WARNING %s(): %d discontiguity among %d segments\n",
               __FUNCTION__, frag, seg_numb);
    }
    return frag;
}



/***********************************************************
 *
 *  DMA control 
 *
 **********************************************************/
uint32 mpc_dma_check_free (uint32 *ccsr_base, uint32 dma_id)
{
    uint32 *base, addr;
    uint32  reg;

    assert(dma_id < 4);
    base = ccsr_base + (P4080_DMAn_BASE(dma_id) >> 2);
    addr = (uint32) (base + P4080_DMA_SR);

    reg = in32(addr);
    if (reg & (DMA_SR_CB | DMA_SR_CH)) {
        return 0;
    } else {
        return 1;
    }
}


uint32 mpc_dma_check_error (uint32 *ccsr_base, uint32 dma_id)
{
    uint32 *base, addr;
    uint32 reg;

    assert(dma_id < 4);
    base = ccsr_base + (P4080_DMAn_BASE(dma_id) >> 2);
    addr = (uint32) (base + P4080_DMA_SR);

    reg = in32(addr);
    if (reg & (DMA_SR_TE | DMA_SR_PE)) {
        printf("%s(): DMA %d error (SR 0x%08x)\n", __FUNCTION__, dma_id, reg);
        if (reg & DMA_SR_TE)
            printf("\tTE    Transfer error\n");
        if (reg & DMA_SR_PE)
            printf("\tSR    Programming error\n");
        return 1;

    } else {
        return 0;
    }
}


void mpc_dma_clear_status (uint32 *ccsr_base, uint32 dma_id)
{
    uint32 *base, addr;

    assert(dma_id < 4);
    base = ccsr_base + (P4080_DMAn_BASE(dma_id) >> 2);
    addr = (uint32) (base + P4080_DMA_SR);

    out32(addr, 0x9B);
}


void mpc_dma_dump_status (uint32 *ccsr_base, uint32 dma_id)
{
    uint32 *base, addr;
    uint32 reg;

    assert(dma_id < 4);
    base = ccsr_base + (P4080_DMAn_BASE(dma_id) >> 2);
    addr = (uint32) (base + P4080_DMA_SR);

    reg = in32(addr);
    printf("%s(): DMA %d SR 0x%08x\n", __FUNCTION__, dma_id, reg);
    if (reg & DMA_SR_TE)
        printf("\tTE    Transfer error\n");
    if (reg & DMA_SR_PE)
        printf("\tSR    Programming error\n");
    if (reg & DMA_SR_CH)
        printf("\tCH    Channel halted\n");
    if (reg & DMA_SR_EOLNI)
        printf("\tEOLNI End-of-links interrupt\n");
    if (reg & DMA_SR_CB)
        printf("\tCB    Channel busy\n");
    if (reg & DMA_SR_EOSI)
        printf("\tEOSI  End-of-segment interrupt\n");
    if (reg & DMA_SR_EOLSI)
        printf("\tEOLSI End-of-list interrupt\n");
}


/* use basic direct mode */
void mpc_dma_bd_mode_org (uint32 *ccsr_base, uint32 dma_id,
                          uint32 src, uint32 dst, uint32 byte)
{
    volatile uint32 *base;

    assert(dma_id < 4);
    assert(ccsr_base);
    base = ccsr_base + (P4080_DMAn_BASE(dma_id) >> 2);

    /* Mode Registers */
    // set it in direct mode                                                          
    // set BWC to allow interleaving of 256 bytes                                     
    *(base + P4080_DMA_MR) = DMA_MR_CTM | DMA_MR_BWC(0x8);

    /* source attribute */
    // Read, snoop processor
    *(base + P4080_DMA_SATR) = DMA_SATR_SREADTTYPE(5);
    /* destination attribute */
    // write, snoop processor
    *(base + P4080_DMA_DATR) = DMA_DATR_DWRITETYPE(5);

    /* source addr, destination addr, and byte count */
    *(base + P4080_DMA_SAR) = src;
    *(base + P4080_DMA_DAR) = dst;
    *(base + P4080_DMA_BCR) = byte;

    mpc_dma_reg_dump(ccsr_base, dma_id);

    /* launch the DMA */
    *(base + P4080_DMA_MR) |= DMA_MR_CS;
}


void mpc_dma_bd_mode (uint32 *ccsr_base, uint32 dma_id,
                      uint32 src, uint32 dst, uint32 byte)
{
    uint32 base, mr_reg;

    assert(dma_id < 4);
    assert(ccsr_base);
    base = (uint32) ccsr_base + P4080_DMAn_BASE(dma_id);

    /* Mode Registers */
    // Direct mode, BWC to allow interleaving of 256 bytes
    mr_reg = DMA_MR_CTM | DMA_MR_BWC(0x8);
    out32((base + (P4080_DMA_MR << 2)), mr_reg);

    /* source attribute */
    // Read, snoop processor
    out32((base + (P4080_DMA_SATR << 2)), DMA_SATR_SREADTTYPE(5));
    /* destination attribute */
    // write, snoop processor
    out32((base + (P4080_DMA_DATR << 2)), DMA_DATR_DWRITETYPE(5));

    /* source addr, destination addr, and byte count */
    out32((base + (P4080_DMA_SAR << 2)), src);
    out32((base + (P4080_DMA_DAR << 2)), dst);
    out32((base + (P4080_DMA_BCR << 2)), byte);

    mpc_dma_reg_dump(ccsr_base, dma_id);

    /* launch the DMA */
    out32((base + (P4080_DMA_MR << 2)), (mr_reg | DMA_MR_CS));
}


void mpc_dma_reg_dump (uint32 *ccsr_base, uint32 dma_id)
{
    uint32 dma_base, addr;
    uint32 reg;

    assert(dma_id < 4);
    assert(ccsr_base);
    dma_base = (uint32) (ccsr_base + (P4080_DMAn_BASE(dma_id) >> 2));

    printf("CCSR base  0x%08x\n", (uint32) ccsr_base);
    printf("DMA %d base 0x%08x\n", dma_id, dma_base);

    addr = dma_base + (P4080_DMA_MR << 2);
    reg  = in32(addr);
    printf("\tP4080_DMA_MR  (0x%08x)   0x%08x\n", addr, reg);

    addr = dma_base + (P4080_DMA_SR << 2);
    reg  = in32(addr);
    printf("\tP4080_DMA_SR  (0x%08x)   0x%08x\n", addr, reg);

    addr = dma_base + (P4080_DMA_SAR << 2);
    reg  = in32(addr);
    printf("\tP4080_DMA_SAR (0x%08x)   0x%08x\n", addr, reg);

    addr = dma_base + (P4080_DMA_DAR << 2);
    reg  = in32(addr);
    printf("\tP4080_DMA_DAR (0x%08x)   0x%08x\n", addr, reg);

    addr = dma_base + (P4080_DMA_BCR << 2);
    reg  = in32(addr);
    printf("\tP4080_DMA_BCR (0x%08x)   0x%08x\n", addr, reg);
}



/*******************************************************************
 *
 * Function:     msecdelay()
 * Arguments:    msec - # of milliseconds to suspend calling thread
 *
 ******************************************************************/
void msecdelay (uint32 msec)
{
    struct timespec delay_value;

    delay_value.tv_sec  = (msec / 1000);
    delay_value.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&delay_value, 0);
}


/*******************************************************************
 *
 * Function:     usecdelay()
 * Arguments:    usec - # of microseconds to suspend calling thread
 *
 ******************************************************************/
void usecdelay (uint32 usec)
{
    struct timespec delay_value;

    delay_value.tv_sec  = (usec / 1000000);
    delay_value.tv_nsec = (usec % 1000000) * 1000;
    nanosleep(&delay_value, 0);
}


/*******************************************************************
 *
 * Function:     msecspin()
 * Arguments:    msec - # of milliseconds to spin the calling thread
 *
 ******************************************************************/
void msecspin (uint32 msec)
{
    struct timespec delay_value;

    delay_value.tv_sec  = (msec / 1000);
    delay_value.tv_nsec = (msec % 1000) * 1000000;
    nanospin(&delay_value);
}


/*******************************************************************
 *
 * Function:     usecspin()
 * Arguments:    usec - # of microseconds to spin the calling thread
 *
 ******************************************************************/
void usecspin (uint32 usec)
{
    struct timespec delay_value;

    delay_value.tv_sec  = (usec / 1000000);
    delay_value.tv_nsec = (usec % 1000000) * 1000;
    nanospin(&delay_value);
}



/***********************************************************
 *
 *  Memory 
 *
 **********************************************************/
void fill_mem_ptrn (uint32 *addr, uint32 size, uint32 ptrn)
{
    uint32  size_word, n;

    size_word = size / sizeof(uint32);
    if ((size_word * sizeof(uint32)) != size) {
        fprintf(stderr,
                "%s(): size %d is not word aligned\n", __FUNCTION__, size);
    }

    for (n = 0; n < size_word; ++n) {
        addr[n] = ptrn;
    }
}


uint32 mem_ptrn_match (uint32 *addr_a, uint32 *addr_b, uint32 size)
{
    uint32  size_word, n;
    uint32  cntr;

    size_word = size / sizeof(uint32);
    if ((size_word * sizeof(uint32)) != size) {
        fprintf(stderr,
                "%s(): size %d is not word aligned\n", __FUNCTION__, size);
    }

    for (n = cntr = 0; n < size_word; ++n) {
        if (addr_a[n] != addr_b[n])
            ++cntr;
    }

    if (cntr == 0) {
        printf("%s(): mem ptrn match\n", __FUNCTION__);
        return 1;
    } else {
        printf("%s(): mem ptrn NOT match, %d out of %d (in word)\n",
               __FUNCTION__, size_word, cntr);
        return 0;
    }
}
