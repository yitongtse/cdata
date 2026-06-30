#ifndef  __P4080_UTIL_H__
#define  __P4080_UTIL_H__



extern uint32 *ccsr_reg_mmap (void);


extern int32 mmap_tool (uint32 buff_size,
                        uint32 *buff_addr, uint32 *buff_addr_phy);
extern uint32 mem_contig_check (uint8 *addr_0, uint32 mem_size);


extern uint32 mpc_dma_check_free (uint32 *ccsr_base, uint32 dma_id);
extern uint32 mpc_dma_check_error (uint32 *ccsr_base, uint32 dma_id);
extern void mpc_dma_clear_status (uint32 *ccsr_base, uint32 dma_id);
extern void mpc_dma_dump_status (uint32 *ccsr_base, uint32 dma_id);
extern void mpc_dma_bd_mode (uint32 *ccsr_base, uint32 dma_id,
                             uint32 src, uint32 dst, uint32 byte);
extern void mpc_dma_reg_dump (uint32 *ccsr_base, uint32 dma_id);


extern void msecdelay (uint32 msec);
extern void usecdelay (uint32 msec);
extern void msecspin (uint32 msec);
extern void usecspin (uint32 msec);


extern void fill_mem_ptrn (uint32 *addr, uint32 byte, uint32 ptrn);
extern uint32 mem_ptrn_match (uint32 *addr_a, uint32 *addr_b, uint32 byte);




#endif //  __P4080_UTIL_H__
