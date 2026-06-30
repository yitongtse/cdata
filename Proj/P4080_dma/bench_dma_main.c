#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <hw/inout.h>
#include <sys/neutrino.h>

#include "common.h"
#include "p4080_mem_map.h"
#include "p4080_util.h"



#define  MEM_BUFF_SIZE          (1 * 1024 * 1024)   // 256 Mbyte
#define  DMA_READY_POLL         100



uint32   *ccsr_base;

uint32   *dma_mem_src;
uint32   *dma_mem_dst;
uint32   dma_mem_src_phy;
uint32   dma_mem_dst_phy;



int32 main (int32 argc, char *argv[])
{
    uint32  dma_id;
    uint32  reg, n;

    if (ThreadCtl(_NTO_TCTL_IO, 0) < 0) {
        fprintf(stderr, "Failed to call ThreadCtl\n");
        assert(0);
    }

    ccsr_base = ccsr_reg_mmap();
    dma_id = 0;

    /* check the CCSRBARH and CCSRBARL */
    reg = *(ccsr_base + P4080_CCSRBARH);
    printf("P4080_CCSRBARH:  0x%08x\n", reg);
    reg = *(ccsr_base + P4080_CCSRBARL);
    printf("P4080_CCSRBARL:  0x%08x\n", reg);


    /* allocate source memory block */
    (void) mmap_tool(MEM_BUFF_SIZE,
                     (uint32 *) &dma_mem_src, (uint32 *) &dma_mem_src_phy);
    if (mem_contig_check((uint8 *) dma_mem_src, MEM_BUFF_SIZE) != 0) {
        fprintf(stderr, "Can not get continuous memory for src");
        assert(0);
    }
    printf("allocate src buffer, 0x%08x byte, addr 0x%08x, addr_phy 0x%08x\n",
           MEM_BUFF_SIZE, (uint32) dma_mem_src, dma_mem_src_phy);

    /* allocate destination memory block */
    (void) mmap_tool(MEM_BUFF_SIZE,
                     (uint32 *) &dma_mem_dst, (uint32 *) &dma_mem_dst_phy);
    if (mem_contig_check((uint8 *) dma_mem_dst, MEM_BUFF_SIZE) != 0) {
        fprintf(stderr, "Can not get continuous memory for dst");
        assert(0);
    }
    printf("allocate dst buffer, 0x%08x byte, addr 0x%08x, addr_phy 0x%08x\n",
           MEM_BUFF_SIZE, (uint32) dma_mem_dst, dma_mem_dst_phy);


    /* fill ptrn */
    fill_mem_ptrn(dma_mem_src, MEM_BUFF_SIZE, 0x01234567);
    fill_mem_ptrn(dma_mem_dst, MEM_BUFF_SIZE, 0x89ABCDEF);
    /* check the mem ptrn */
    if (mem_ptrn_match(dma_mem_src, dma_mem_dst, MEM_BUFF_SIZE)) {
        printf("memory ptrn match\n");
    } else {
        printf("memory ptrn NOT match\n");
    }


    /* check DMA engine error */
    if (mpc_dma_check_error(ccsr_base, dma_id)) {
        fprintf(stderr, "DMA %d shows error\n", dma_id);
        assert(0);
    } else {
        printf("DMA %d NO error\n", dma_id);
        mpc_dma_reg_dump(ccsr_base, dma_id);
    }

    /* check if the DMA engine is free */
    for (n = 0; n < DMA_READY_POLL; ++n) {
        if (mpc_dma_check_free(ccsr_base, dma_id) == 1)
            break;
        msecdelay(10);
    }
    if (n < DMA_READY_POLL) {
        printf("DMA %d is ready (after poll %d times)\n", dma_id, (n + 1));
    } else {
        fprintf(stderr, "DMA %d is NOT ready (after poll %d times)\n",
                dma_id, DMA_READY_POLL);
        assert(0);
    }


    /* launch the DMA */
    printf("Start DMA\n");
    mpc_dma_bd_mode(ccsr_base, dma_id,
                    dma_mem_src_phy, dma_mem_dst_phy, MEM_BUFF_SIZE);


    /* check DMA engine status */
    if (mpc_dma_check_error(ccsr_base, dma_id)) {
        fprintf(stderr, "DMA %d shows error\n", dma_id);
        assert(0);
    } else {
        printf("DMA %d NO error\n", dma_id);
        mpc_dma_reg_dump(ccsr_base, dma_id);
    }

    /* check if the DMA engine is free */
    for (n = 0; n < DMA_READY_POLL; ++n) {
        if (mpc_dma_check_free(ccsr_base, dma_id) == 1)
            break;
        msecdelay(10);
    }
    if (n < DMA_READY_POLL) {
        printf("DMA %d is ready (after poll %d times)\n", dma_id, (n + 1));
    } else {
        fprintf(stderr, "DMA %d is NOT ready (after poll %d times)\n",
                dma_id, DMA_READY_POLL);
        assert(0);
    }

    mpc_dma_reg_dump(ccsr_base, dma_id);


    /* check the mem ptrn */
    if (mem_ptrn_match(dma_mem_src, dma_mem_dst, MEM_BUFF_SIZE)) {
        printf("memory ptrn match\n");
    } else {
        printf("memory ptrn NOT match\n");
    }


    exit(0);
}
