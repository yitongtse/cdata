/*
 *------------------------------------------------------------------
 * Maverick MPC8548-Cobia-Yellowfin sniff test 
 *
 * Nov  2007, Fang Fang 
 *
 * Copyright (c) 2006-2008 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

#include "ids_module.h"
#include "api.h"
#include "ids_slot.h"
#include "board.h"
#include "menu.h"
#include "testlist.h"
#include "ids_api.h"
#include "command.h"
#include "mb_diag.h"
#include "cobia.h"
#include "cb_util.h"
#include "mb_fs8548_dma_driver.h"
#include "mb_fs8548_pcie.h"
#include "yellowfin.h"
#include "discus.h"
#include "yf_util.h"
#include "mb_utils.h"

static volatile cobia_reg_struct_t *cobia =
                (volatile cobia_reg_struct_t *)COBIA_FPGA_BASE;
static volatile yellowfin_reg_struct_t *yellowfin =
                (volatile yellowfin_reg_struct_t *)YELLOWFIN_FPGA_BASE;

extern void * malloc(unsigned RqBytes);
extern void free(void *Ptr);
static COBIA_PCIE_BAR_WIN_SIZE cbPcieOutBoundWinSize;



#define TIMEOUT  10000
#define VIDEO_MPEG_PKTS_NUM_PER_FLOW     1
#define VIDEO_MPEG_PKTS_NUM_MAX          32
#define SNIFF_DISABLED                   0
typedef enum testType {
    STREAMING,
    SNIFF_ONLY,
    CC_REMAPPING,
    PID_REMAPPING,
    PCR_REMAPPING,
    SCHEDULER
};

static u_int32 loopback = 0;
static u_int32 continuous = 0;
static u_int32 qam_ch         = 0;
static u_int32 qam_ch_grp     = 0;
static u_int32 qam_ch_index   = 0;
static u_int32 QAM64or256     = 0;
static u_int32 Annex_sel      = 1;
static u_int32 random_pattern = 0;
static u_int32 qam_ch_start   = 0;
static u_int32 qam_ch_end     = 0;
static u_int32 sniff_point    = 0;

static u_int32 *data_buf_ptr      = NULL;
static u_int32 *cmd_buf_ptr       = NULL;
static u_int32 *sniff_buf_start_ptr     = NULL;
static malloc_aligned4k_t data_buf_malloc;
static malloc_aligned256_t cmd_buf_malloc;
static malloc_aligned4k_t sniff_buf_start_malloc;
extern u_int32 cmd_pkt[CMD_PKT_SIZE];
extern u_int32 mpeg_pkt[MPEG_PKT_SIZE_WORD];

typedef enum traffic_mode_e {
    DMPT_MODE  = 0,
    PSP_MODE   = 1,
    VIDEO_MODE = 2
} traffic_mode;


static video_cmd_t video_cmd;
static mpeg_ts_header_t mpeg_ts_header;
static u_int32 af_field_flag = 0;
static u_int32 cb_sniff_point = 0;
static u_int32 yf_sniff_point = 0;
static u_int32 mode = VIDEO_MODE;
static u_int32 flow_num = 0;
static u_int32 cmd_pkt_num_per_flow = 0;
static u_int32 flow_num_max = 1;
static u_int32 cmd_size = 0;
static u_int32 cmd_per_flow = 1;
static u_int32 sniff_flag = 1;


U_INT32_PARAM_DEC(P_qam_ch,"QAM channel to be tested : 0 to 47; 48 for
                  all", &qam_ch, NOT_INDEX)
U_INT32_PARAM(P_QAM64or256,"QAM64 = 0, QAM256 = 1",&QAM64or256,NOT_INDEX)
U_INT32_PARAM(P_Annex_sel,"Annex A = 0, Annex B = 1, Annex C = 2",&Annex_sel,NOT_INDEX)
//U_INT32_PARAM(P_random_pattern,"Fixed Pattern = 0, Random Pattern = 1",&random_pattern,NOT_INDEX)
U_INT32_PARAM(P_sniff_point, "0: At Output  of Cobia, 
                              1: At Input  of Yellowfin, 
                              2: At Input  of J.83 IP, 
                              3: At Output of J.83 IP",
                              &sniff_point, NOT_INDEX)
U_INT32_PARAM(P_cmd_per_flow,"Please choose cmd for per flow:",&cmd_per_flow,NOT_INDEX)
U_INT32_PARAM(P_flow_num_max,"Please choose total flows for per channel:",&flow_num_max,NOT_INDEX)
U_INT32_PARAM(P_sniff_flag,"0: Streaming ouput(Sniffing disabled), 
                            1: Sniffing only, no remapping,
                            2: CC Remapping,
                            3: PID Remapping",&sniff_flag,NOT_INDEX)

TEST_START_NO_DEFAULT(yf_video_sniffing_test,"Maverick Video Sniffing Test",
           Init_yf_video_sniffing_test,
           Test_yf_video_sniffing_test,
           Iterate_yf_sniffing_test,
           Status_yf_sniffing_test,
           Error_yf_sniffing_test,
           Exception_yf_sniffing_test,
           Cleanup_yf_sniffing_test)
TEST_PARAMETER(P_qam_ch)
TEST_PARAMETER(P_QAM64or256)
TEST_PARAMETER(P_Annex_sel)
//TEST_PARAMETER(P_random_pattern)
TEST_PARAMETER(P_cmd_per_flow)
TEST_PARAMETER(P_flow_num_max)
TEST_PARAMETER(P_sniff_flag)
TEST_PARAMETER(P_sniff_point)
TEST_END (yf_video_sniffing_test)

u_int32 sinffed_packets_check(u_int32 *data_buf_ptr,
                              u_int32 *data_rec_buf_ptr,
                              u_int32 pkt_num,
                              u_int32 sniff_point) {
    u_int32 ret_val = 0;
    
    switch (sniff_point) {
    case CB_OUT:
    case YF_INPUT+1:
    case J83IP_INPUT+1:
        programDebug("Begin to verifying... Available sniff point ... Done\n ");
        ret_val = verify_sniffed_packets(data_buf_ptr, data_rec_buf_ptr, pkt_num);
        
        break;
    case J83IP_OUTPUT+1:
        /*
         * Need to generate the new packets after FEC
         */
        /*
        verify_mpt_fec_packets(data_buf_ptr, sniff_point); 
        */
        fatalError("J83IP Output Sniffing point was not supported for now!");
        ret_val = TEST_FAILED;
        break;
    default:
        fatalError("Invalid Sniffe Point!");
        return (INIT_FAILED); 
    }
    if (ret_val) {
        fatalError("Sniffed data check error!");
        return (TEST_FAILED);    
    }
    return (TEST_PASSED);

}

static short init_sniff_traffic_test (void)
{
    
    MB_PCIE_ERR_CODE pcieErrCode = MB_PCIE_PASSED;
    /*
     * Verify parameters 
     */
    printf("channel number: 0x%08x \n", qam_ch);
    if (qam_ch < 0 || qam_ch > QAMS_NUM) {
        fatalError("There is an incorrect QAM channel selected for "
                   "testing");
        fatalError("Please select from 0 to 48.");
        return (INIT_FAILED);
    } 

    if (qam_ch == QAMS_NUM) {
       /*
        * Test all
        */
        printf("Will test all channel...");
        qam_ch_start = 0;
        qam_ch_end = QAMS_NUM - 1;
    } else {
        qam_ch_start = qam_ch;
        qam_ch_end = qam_ch;
    }
    /*
     * Initial cobia and yellowfin sniff point
     */
    if (sniff_point == 0) {
        cb_sniff_point = CB_OUT;
    } else {
        cb_sniff_point = CB_IN;
        yf_sniff_point = sniff_point - 1;
    }
    /*
     * Initial PCIe Interface
     */
    pcieErrCode = mb_pcie_init(&cbPcieOutBoundWinSize);
    if (pcieErrCode) {
        fatalError("Test Initialization Failed");
        return (TEST_FAILED);
    }

    /*
     * for initial bringup, just send pkt to one flow, 
     * finally will be VIDEO_SNIFF_FLOW_NUM_MAX
     */
    flow_num_max = 1;
    //flow_num_max = VIDEO_SNIFF_FLOW_NUM_MAX;
    /*
     * for initial bringup, just send one cmd pkt to one flow,
     * finallly will send more
     */
    cmd_pkt_num_per_flow = cmd_per_flow;


    /*
     * setup the pointer pointed to the buffer
     */
    printf("Begin to allocate the memory!!!"); 
    cmd_buf_ptr = (u_int32 *)malloc_aligned256(cmd_size *
                                               cmd_pkt_num_per_flow *
                                               32 *
                                               QAMS_NUM, 
                                               &cmd_buf_malloc);
    if (cmd_buf_ptr == NULL) {
        fatalError("%s(): malloc failed for Command Buffer", __FUNCTION__);
        return(INIT_FAILED);
    }
    programDebug("cmd_buf_ptr ok, start = 0x%08x, size = 0x%08x, end addr = 0x%08x \n", 
           cmd_buf_ptr,
           (cmd_size*cmd_pkt_num_per_flow *32 *QAMS_NUM ),
           cmd_buf_ptr + (cmd_size/4)); 
    programDebug("cmd_pkt size = 0x%08x, qam_num = 0x%08x \n", cmd_size, QAMS_NUM);
    
    data_buf_ptr  = (u_int32 *)malloc_aligned4k(sizeof(mpeg_pkt) * 
                                                MPEG_PKTS_NUM*
                                                32*
                                                cmd_pkt_num_per_flow *
                                                QAMS_NUM, 
                                                &data_buf_malloc);
    if (data_buf_ptr == NULL) {
        fatalError("%s(): malloc failed for Data Buffer", __FUNCTION__);
        return(INIT_FAILED);
    }
    programDebug("data_buf_ptr ok, start = 0x%08x, size = 0x%08x, end addr = 0x%08x \n", 
           data_buf_ptr,
           (sizeof(mpeg_pkt) * MPEG_PKTS_NUM*32*cmd_pkt_num_per_flow *QAMS_NUM),
           data_buf_ptr + 
           (sizeof(mpeg_pkt) * MPEG_PKTS_NUM*32*cmd_pkt_num_per_flow *QAMS_NUM));
    programDebug("mpeg_pkt size = 0x%08x, qam_num = 0x%08x \n", sizeof(mpeg_pkt), QAMS_NUM);

    sniff_buf_start_ptr  = (u_int32 *)malloc_aligned4k(sizeof(mpeg_pkt) * 
                                                 MPEG_PKTS_NUM * 
                                                 32 *
                                                 cmd_pkt_num_per_flow *
                                                 QAMS_NUM, 
                                                 &sniff_buf_start_malloc);
    if (sniff_buf_start_ptr == NULL) {
        fatalError("%s(): malloc failed for Sniffing Buffer", __FUNCTION__);
        return(INIT_FAILED);
    }
    programDebug("sniff_buf_start_ptr ok, start = 0x%08x, size = 0x%08x, end addr = 0x%08x \n", 
           sniff_buf_start_ptr,
           (sizeof(mpeg_pkt) * MPEG_PKTS_NUM*32*cmd_pkt_num_per_flow *QAMS_NUM),
           data_buf_ptr + 
           (sizeof(mpeg_pkt) * MPEG_PKTS_NUM*32*cmd_pkt_num_per_flow *QAMS_NUM));
      
    printf("Initia Cobia QDR... \n");
    //cobia_qdr_init();

    /*
     * setup sniff address and size
     */
    cobia->cb_sniff_ddr2_addr_start = (u_int32)sniff_buf_start_ptr;
    cobia->cb_sniff_ddr2_addr_size  = (sizeof(mpeg_pkt)/4) *
                                      MPEG_PKTS_NUM *
                                      32 *
                                      cmd_pkt_num_per_flow *
                                      QAMS_NUM;
    programDebug("cb_sniff_ddr2_addr_start = %08x \n",
                 cobia->cb_sniff_ddr2_addr_start);
    programDebug("cb_sniff_ddr2_addr_size = 0x%08x \n",
                 cobia->cb_sniff_ddr2_addr_size);

   
    cobia->cb_qdr2_mem_test_reg &= ~0x03; 
    cobia->cb_pio_mem_test_reg  &= (~CB_PIO_MEM_TEST_EN);
    yellowfin->yf_qdr2_mem_test_en_reg = 0;
    
    cobia->cb_cet_det_expire_chk_en_reg = 0x3;
    cobia->cb_com_pkt_rd_off_reg = 0;

    cb_err_reg_dump();
    yf_err_reg_dump();


    return (TEST_PASSED);
}


    
static short Init_yf_video_sniffing_test (void)
{
    loopback = 1;
    mode = VIDEO_MODE;
    cmd_size = VIDEO_CMD_PKT_SIZE_BYTE;
  //  cobia->cb_cet_det_expire_chk_en_reg = CB_DET_EXPIRY_CHECK_DISABLE;
    programDebug("cb_cet_det_expire_chk_en_reg = 0x%08x @ 0x%08x\n",
                 cobia->cb_cet_det_expire_chk_en_reg,
                 &(cobia->cb_cet_det_expire_chk_en_reg));
    if (init_sniff_traffic_test()) {
        return (INIT_FAILED);
    } else {
        return (INIT_PASSED);
    }
}

u_int32 sendCmds (u_int32 *cmd_ptr,
                  u_int32 outbWin,
                  u_int32 buf_size,
                  u_int32 cmd_pkt_num_per_flow,
                  u_int32 sniff_flag) {
    
    u_int32 index = 0;
    u_int32 ret_val = 0;

    if (sniff_flag != SNIFF_DISABLED) {
        ret_val = outboundDmaWrite(cmd_ptr, 
                                   outbWin, 
                                   buf_size);
        programDebug("DMAing: from 0x%08x,buf_size = 0x%08x, cmd_pkt_num_per_flow = 0x%08x \n", 
                     cmd_ptr,
                     buf_size,
                     cmd_pkt_num_per_flow);
                               
        if (ret_val) {
            fatalError("Dma Write Error!");
            return (TEST_FAILED);
        }
    } else {
        //continuous sending packets
         idsPrintf("Sending packets...,ESC to quit. \n");
         idsPrintf(". ");
         while (getchar_no_block() != 033 ) {
             if (index == 0x100) {
                 index = 0;
                 idsPrintf(". ");
             } else {
                 index++;
             }
             ret_val = outboundDmaWrite(cmd_ptr, 
                                        outbWin, 
                                        buf_size);
                               
            if (ret_val) {
                fatalError("Dma Write Error!");
                return (TEST_FAILED);
            }
        }
        idsPrintf("Quiting... \n");
    }
}

static short Test_yf_sniffing_test (u_int32 mode)
{
    u_int32 ret_val             = 0;   
    u_int32 *cmd_ptr            = NULL;
    u_int32 *data_buf_ptr_saved = data_buf_ptr;
    u_int32 buf_size            = 0;
    u_int32 flow_index          = 0;
    u_int32 pkt_index           = 0;
    u_int32 mpeg_pkt_num        = 0;
    u_int32 *data_rec_buf_ptr   = NULL;
    u_int32 outbWin             = MB_BAR4_WIN_BASE_ADDR;

    cmd_ptr = cmd_buf_ptr;
   // cmd_size = cmd_size;
    cb_yf_chan_pkt_cnt_dump(0,1);
    cb_yf_chan_pkt_cnt_dump(1,1);
    /*
     * for now, we assume the cb_sniff_ddr2_wr_pntr will be reset once the 
     * cb_sniff_ddr2_addr_start is reloaded.
     */
    data_rec_buf_ptr = sniff_buf_start_ptr;
    programDebug("sniff_buf_start_ptr = 0x%08x \n", data_rec_buf_ptr);
    
    for (qam_ch = qam_ch_start; qam_ch <= qam_ch_end ; qam_ch++) {
        qam_ch_grp   = qam_ch / QAM_CH_NUM_PER_GRP;
        qam_ch_index = qam_ch % QAM_CH_NUM_PER_GRP;
        printf("\n\n\n\n\nChannel Group:0x%08x, QAM Index: 0x%08x \n\n", 
               qam_ch_grp, qam_ch_index);
        
        /*
         * sync data_rec_buf_ptr
         * Save pointer in sending buffer for one qam.
         */
        data_rec_buf_ptr += data_buf_ptr - data_buf_ptr_saved;
        data_buf_ptr_saved = data_buf_ptr;
        programDebug("sync data_rec_buf_ptr and save snding buffer 
                     start address at the begining of a new QAM channel \n");
        programDebug("Reg:data_rec_buf_ptr = 0x%08x \n", data_rec_buf_ptr);
        programDebug("Reg:data_buf_ptr_saved = 0x%08x \n",
                     data_buf_ptr_saved);
         
        
        /*
         * Dump channel initial information
         */
        programDebug("Dumping QAM initial information...\n");
        programDebug("QAM Channel:%d, Channel Group:%d, Channel Index:%d \n", 
                     qam_ch, 
                     qam_ch_grp, 
                     qam_ch_index);
        programDebug("Command pkt start address(cmd_ptr):0x%08x \n", cmd_ptr);
        programDebug("MPEG pkt start address(data_buf_ptr):0x%08x \n",data_buf_ptr);
        programDebug("Sniff start address(sniff_buf_start_ptr):0x%08x \n", sniff_buf_start_ptr);
        programDebug("Sniff data read back address(data_rec_buf_ptr):0x%08x \n", data_rec_buf_ptr);
        programDebug("Reg:cb_sniff_ddr2_addr_start = 0x%08x \n", 
                     cobia->cb_sniff_ddr2_addr_start);
        programDebug("Reg:cb_sniff_ddr2_addr_size = 0x%08x \n",
                     cobia->cb_sniff_ddr2_addr_size);
        
        /* 
         * Set up Cobia cmd and data pkts
         */
        switch (mode) {
        case DMPT_MODE:   
            programDebug("DMPT mode choosed!\n");
            for (pkt_index = 0; pkt_index < cmd_pkt_num_per_flow; pkt_index++) {
                /*
                 * one cmd pkt = 7 DMPT MPEG_TS max
                 */
                setup_mpt_packets_4_sniff(cmd_buf_ptr, 
                                          data_buf_ptr, 
                                          qam_ch, 
                                          random_pattern);
                cmd_buf_ptr  += cmd_size/4;
                data_buf_ptr += (sizeof(mpeg_pkt)/4)*DMPT_MPEG_PKTS_NUM_MAX;
            }
            outbWin = MB_BAR2_WIN_BASE_ADDR;
            flow_num = 1;
            buf_size = cmd_size*cmd_pkt_num_per_flow;
            mpeg_pkt_num = DMPT_MPEG_PKTS_NUM_MAX * cmd_pkt_num_per_flow * flow_num;
            break;
        case PSP_MODE:
            fatalError("Not supported now!");
            return (TEST_FAILED);
        case VIDEO_MODE:
            programDebug("VIDEO mode choosed!\n");
            video_cmd.qam_id = qam_ch;

            switch (sniff_flag) {
            case STREAMING:
                programDebug("Streaming Test... \n");
            case SNIFF_ONLY:
                video_cmd.pid_remapping_flag = 0;
                video_cmd.cc_restamping_flag = 0;
                video_cmd.pcr_restamping_flag = 0;
                programDebug("Sniffing Only Test... \n");
                break;
            case CC_REMAPPING:
                video_cmd.pid_remapping_flag = 0;
                video_cmd.cc_restamping_flag = 1;
                video_cmd.pcr_restamping_flag = 0;
                programDebug("CC Remapping Test... \n");
                break;
            case PID_REMAPPING:
                video_cmd.pid_remapping_flag = 1;
                video_cmd.cc_restamping_flag = 0;
                video_cmd.pcr_restamping_flag = 0;
                programDebug("PID Remapping Test... \n");
                break;
            case PCR_REMAPPING:
                video_cmd.pid_remapping_flag = 0;
                video_cmd.cc_restamping_flag = 0;
                video_cmd.pcr_restamping_flag = 1;
                programDebug("PCR Remapping Test... \n");
            case SCHEDULER:
            default:
                fatalError("Invalid test type!");
                return (TEST_FAILED);
            }
            for (flow_index = 0; flow_index < flow_num_max; flow_index++) {
                setup_video_packets(cmd_buf_ptr,
                                    data_buf_ptr,
                                    qam_ch,
                                    random_pattern,
                                    af_field_flag,
                                    cmd_pkt_num_per_flow,
                                    flow_index,
                                    &video_cmd,
                                    &mpeg_ts_header);
                cmd_buf_ptr += (cmd_size/4)*cmd_pkt_num_per_flow;
                data_buf_ptr += (sizeof(mpeg_pkt)/4)*VIDEO_MPEG_PKTS_NUM*cmd_pkt_num_per_flow;
            }
            outbWin = MB_BAR4_WIN_BASE_ADDR;
            flow_num = flow_num_max;
            buf_size = cmd_size*cmd_pkt_num_per_flow;
            mpeg_pkt_num = cmd_pkt_num_per_flow * flow_num;
            break;
        default:
            fatalError("Invalid traffic mode!");
            return (TEST_FAILED);
        }
        /* 
         * Set up Cobia registers for dma and loopback
         */
        if (sniff_flag != SNIFF_DISABLED) {
            setup_cobia_regs_4_sniff(qam_ch, 
                                     QAM64or256,
                                     Annex_sel, 
                                     mode,
                                     cb_sniff_point);
            setup_yf_regs_4_sniff(qam_ch, 
                                  QAM64or256,
                                  Annex_sel, 
                                  mode,
                                  yf_sniff_point);
        } else {
            cobia->cb_sniff_debug_cfg_reg = 0;
            yellowfin->yf_debug_cfg_reg = 0;
            setup_cobia_regs_4_traffic(qam_ch_grp, 
                                       QAM64or256,
                                       Annex_sel, 
                                       mode);
            setup_yf_regs_4_traffic(qam_ch_grp, 
                                    QAM64or256,
                                    Annex_sel, 
                                    mode);
        }
        

        /* 
         * DMA the D-MPT command packets in the DDR2 memory 
         * to the Cobia PCIe mapped space through the PCIe interface. 
         */
        for (flow_index = 0; 
             flow_index < flow_num; 
             flow_index++) {
            
            sendCmds(cmd_ptr, 
                     outbWin, 
                     buf_size, 
                     cmd_pkt_num_per_flow, 
                     sniff_flag);
            cmd_ptr += buf_size/4;
        }
    
        

        cb_yf_chan_pkt_cnt_dump(0,0);
        cb_yf_chan_pkt_cnt_dump(1,0);
        cb_err_reg_dump();
        yf_err_reg_dump();
        /*
         * Dump channel information after sending
         */
        programDebug("Dumping QAM information after sending...\n");
        programDebug("QAM Channel:%d, Channel Group:%d, Channel Index:%d \n", 
                     qam_ch, 
                     qam_ch_grp, 
                     qam_ch_index);
        programDebug("Command pkt end address(cmd_ptr):0x%08x \n", cmd_ptr);
        programDebug("MPEG pkt end address(data_buf_ptr):0x%08x \n",data_buf_ptr);

        
        programDebug("Verifying the sniffed data... \n");
        programDebug("Expected buf address start(data_buf_ptr_saved) 
                     = 0x%08x \n", 
                     data_buf_ptr_saved);
        programDebug("Received buf address start(data_rec_buf_ptr) 
                     = 0x%08x \n", 
                     data_rec_buf_ptr);
        programDebug("Actual buf address pointer(data_buf_ptr) 
                     = 0x%08x \n", data_buf_ptr);
        programDebug("MPEG count sniffed(mpeg_pkt_num) 
                     = 0x%08x \n", mpeg_pkt_num);
        programDebug("sniff_point = %d \n", sniff_point);
        if (sniff_flag != SNIFF_DISABLED) {
            ret_val = sinffed_packets_check(data_buf_ptr_saved,
                                            data_rec_buf_ptr,
                                            mpeg_pkt_num,
                                            sniff_point); 
            if (ret_val) {
                fatalError("Sniffed data check error!");
                return (TEST_FAILED);    
            }
        }
       // qam_ch++;
        if (continuous) {
            qam_ch++;
            if (qam_ch > qam_ch_end) {
                qam_ch = qam_ch_start - 1;
            }
        }
    }
    //void all channel loop when just test channel with multiple times
    qam_ch--;
    return (TEST_PASSED);    
}


static short Test_yf_video_sniffing_test (void)
{
    if (Test_yf_sniffing_test(mode)) {
        return (INIT_FAILED);
    } else {
        return (INIT_PASSED);
    }

}



static short 
Exception_yf_sniffing_test (void)
{
    return (UNEXPECTED_EXCEPTION);
}

static short 
Iterate_yf_sniffing_test (void)
{
    return (TEST_DONE);
}

static short 
Status_yf_sniffing_test (void)
{
    programDebug("\n Test Group:%d, Channel:%d", qam_ch_grp, qam_ch_index);
    return (STATUS_PASSED);
}

static short 
Cleanup_yf_sniffing_test (void)
{
    if (cmd_buf_ptr) {
        free_aligned256(&cmd_buf_malloc);
        cmd_buf_ptr = NULL;
    }

    if (data_buf_ptr) {
        free_aligned4k(&data_buf_malloc);
        data_buf_ptr = NULL;
    }

    if (sniff_buf_start_ptr) {
        free_aligned4k(&sniff_buf_start_malloc);
        sniff_buf_start_ptr = NULL;
    }
    return (CLEANUP_PASSED);
}

static short 
Error_yf_sniffing_test (void)
{
    return (ERROR_FUNC_PASSED);
}



TEST_START_NO_DEFAULT(yf_dmpt_sniffing_test,"Maverick D-MPT Sniffing Test",
           Init_yf_dmpt_sniffing_test,
           Test_yf_dmpt_sniffing_test,
           Iterate_yf_sniffing_test,
           Status_yf_sniffing_test,
           Error_yf_sniffing_test,
           Exception_yf_sniffing_test,
           Cleanup_yf_sniffing_test)
TEST_PARAMETER(P_qam_ch)
TEST_PARAMETER(P_QAM64or256)
TEST_PARAMETER(P_Annex_sel)
//TEST_PARAMETER(P_random_pattern)
TEST_PARAMETER(P_cmd_per_flow)
TEST_PARAMETER(P_sniff_flag)
TEST_PARAMETER(P_sniff_point)
TEST_END (yf_dmpt_sniffing_test)


static short Init_yf_dmpt_sniffing_test (void)
{
    loopback = 1;
    mode = DMPT_MODE;
    cmd_size = DOC_CMD_PKT_SIZE_BYTE;
    cobia->cb_cet_det_expire_chk_en_reg = CB_CET_EXPIRY_CHECK_DISABLE;
    if (init_sniff_traffic_test()) {
        return (INIT_FAILED);
    } else {
        return (INIT_PASSED);
    }
}

static short Test_yf_dmpt_sniffing_test (void)
{
    if (Test_yf_sniffing_test(mode)) {
        return (INIT_FAILED);
    } else {
        return (INIT_PASSED);
    }

}
    

