h23302
s 00118/00051/00191
d D 1.3 05/05/25 14:31:28 ytse 3 2
c Update
e
s 00001/00001/00241
d D 1.2 05/05/19 17:57:14 ytse 2 1
c Optimize with register
e
s 00242/00000/00000
d D 1.1 05/05/19 10:35:21 ytse 1 0
c date and time created 05/05/19 10:35:21 by ytse
e
u
U
f e 0
t
T
I 1
// Basic types
//
typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned char uint8;
typedef char int8;


// Constants
//
#define NUM_QAM                         48
#define NUM_FLOW_PER_QAM                8
#define NUM_FLOW                        (NUM_QAM * NUM_FLOW_PER_QAM)
#define NUM_PSP_OUT_CMD_PER_FLOW        1024
#define NUM_PSP_OUT_CMD                 (NUM_QAM * NUM_PSP_OUT_CMD_PER_FLOW)
I 3
#define NUM_RX_DESC                     1024
#define IP_BUF_SIZE                     1540
#define TP_SIZE                         188
E 3

D 3
#define UDP_PORT_CISCO_BONDING_MASK     0xf000
#define UDP_PORT_QAM_ID_MASK            0x3f
#define UDP_PORT_DEPI_PSP_MASK          0x40
E 3
I 3
#define UDP_PORT_MASK_CISCO_BONDING     0x40
#define UDP_PORT_MASK_QAM_ID            0x3f
E 3

I 3
#define PSP_SEG_C_BIT                   0x8000
#define PSP_SEG_B_BIT                   0x4000
#define PSP_SEG_E_BIT                   0x2000
#define PSP_SEG_SIZE                    0x1FFF
E 3

//
// Data structures
//

// IP header
//
typedef struct ip_hdr_t_ {
    uint8 version : 4;
    uint8 hdr_len : 4;
    uint8 tos;
    uint16 tot_len;
    uint16 id;
    uint16 flags : 3;
    uint16 frag_offset : 13;
    uint8 ttl;
    uint8 protocol;
    uint16 hdr_chksum;
    uint32 src_ip;
    uint32 dst_ip;
    uint8 payload[0];
} ip_hdr_t;


// UDP header
//
typedef struct udp_hdr_t_ {
    uint16 src_port;
    uint16 dst_port;
    uint16 length;
    uint16 checksum;
    uint16 payload[0];
} udp_hdr_t;


D 3
// PSP segment info
//
typedef struct psp_seg_info_t_ {
    uint16 c_bit : 1;
    uint16 b_bit : 1;
    uint16 e_bit : 1;
    uint16 size  : 13;
} psp_seg_info_t;


E 3
// L2TPv3 header (over UDP)
//
typedef struct l2tp_hdr_t_ {
    uint16 t_bit     : 1;
    uint16 reserved1 : 11;
    uint16 version   : 4;
    uint16 reserved2;
    uint32 session_id;

    // DEPI sub-layer header
    uint8 v_bit : 1;
    uint8 s_bit : 1;
    uint8 h_bit : 1;
    uint8 d_bit : 1;
    uint8 x_bit : 1;
    uint8 flow_id : 3;
    uint8 reserved3;
    uint16 seq_no;

D 3
    psp_seg_info_t seg_table[0];    // for PSP mode only
E 3
I 3
    uint16 seg_table[0];            // for PSP mode only
E 3
} l2tp_hdr_t;


// QAM info
//
I 3
#define QAM_MODE_DEPI_MPT               1
#define QAM_MODE_DEPI_PSP               2
#define QAM_MODE_CISCO_BONDING          4

E 3
typedef struct qam_info_t_ {
D 3
    int qam_id;
E 3
I 3
    uint32 mode;
E 3
} qam_info_t;


// Output command buffer for PSP
//
typedef struct psp_out_cmd_t_ {
    uint32 buf_addr;
D 3
    psp_seg_info_t seg_info[4];
E 3
I 3
    uint16 seg_info[4];
E 3
} psp_out_cmd_t;


I 3
// Output command buffer for MPT or Cisco bonding
//
typedef struct mpt_out_cmd_t_ {
    uint32 buf_addr;
    uint32 num_tp;
} mpt_out_cmd_t;


E 3
// Flow info
//
typedef struct flow_info_t_ {
    uint32 out_cmd_buf_start_addr;
    uint32 out_cmd_buf_end_addr;
D 3
    psp_out_cmd_t* out_cmd_wr_ptr;     // write pointer for Cobia
    psp_out_cmd_t* out_cmd_cur_ptr;    // current write pointer
E 3
I 3
    uint32 out_cmd_wr_ptr;              // write pointer for Cobia
    uint32 out_cmd_cur_ptr;             // current write pointer
E 3
    uint16 prev_seq_no;
} flow_info_t;


I 3
// Receive buffer descriptor
//
#define RX_DESC_FLAG_EMPTY              0x8000
#define RX_DESC_FLAG_WRAP               0x2000

typedef struct rx_desc_t_ {
    uint16 flags;
    uint16 length;
    uint32 buf_addr;
} rx_desc_t;


E 3
// LC wide data structures
//
qam_info_t qam_info_array[NUM_QAM];
flow_info_t flow_info_array[NUM_QAM * NUM_FLOW_PER_QAM];
psp_out_cmd_t psp_out_cmd[NUM_PSP_OUT_CMD];
I 3
rx_desc_t rx_desc_array[NUM_RX_DESC];
E 3

I 3
rx_desc_t *cur_rx_desc;                 // current receive descriptor pointer
uint32 rx_desc_start, rx_desc_end;      // receive descriptor buffer boundaries
E 3

I 3
uint32 free_buf_addr;                   // next free IP packet buffer address
uint32 buf_start_addr, buf_end_addr;    // IP packet buffer boundaries
E 3

I 3

// Process all low priority input packets
//
process_low_prio_input()
{
    while (!(cur_rx_desc->flags & RX_DESC_FLAG_EMPTY)) {
        // Check for packet error (TBD)

        // Process the IP packet
        process_ip_pkt(cur_rx_desc->buf_addr);

        // Update receive descriptor
        cur_rx_desc->flags &= ~RX_DESC_FLAG_EMPTY;
        cur_rx_desc->buf_addr = free_buf_addr;

        // Update receive descriptor pointer
        if (++cur_rx_desc > (rx_desc_t*)rx_desc_end) {
            cur_rx_desc = (rx_desc_t*)rx_desc_start;
        }

        // Update free buffer address
        free_buf_addr += IP_BUF_SIZE;
        if (free_buf_addr >= buf_end_addr) {
            free_buf_addr = buf_start_addr;
        }
    }
}


E 3
// This routine is used to process all IP packets recevied in the packet buffer.
// The following IP packets are expected:
//   DEPI-PSP (L2TP over UDP)
//   DEPI-MPT (L2TP over UDP)
//   Cisco bonding packet (over UDP)
//
// We assume that the payload type can be determined by examining the bits
// on the destination UDP port:
D 3
//   UDP port map: 0000SSSSCTQQQQQQ
E 3
I 3
//   UDP port map: 00000SSSSCQQQQQQ
E 3
//   S: Slot bits
//   C: Cisco bonding (1)
D 3
//   T: DEPI type, MPT (0) or PSP (1)
E 3
//   Q: QAM bits
//
process_ip_pkt(uint8* buf)
{
    ip_hdr_t* ip_hdr = (ip_hdr_t*)buf;
    udp_hdr_t* udp_hdr = (udp_hdr_t*)ip_hdr->payload;
D 3
    l2tp_hdr_t* l2tp_hdr = (l2tp_hdr_t*)udp_hdr->payload;
E 3
I 3
    int qam_id = udp_hdr->dst_port & UDP_PORT_MASK_QAM_ID;
    qam_info_t* qam_info = &qam_info_array[qam_id];
    flow_info_t* flow_info;
    l2tp_hdr_t* l2tp_hdr;
    int flow_id;
    int num_tp;
E 3

D 3
    // Check for packet error

E 3
    // Determine payload type
D 3
    if (udp_hdr->dst_port & UDP_PORT_CISCO_BONDING_MASK) {
E 3
I 3
    if (udp_hdr->dst_port & UDP_PORT_MASK_CISCO_BONDING) {
E 3
        // Cisco bonding
D 3
        process_cisco_bonding_payload(udp_hdr->payload);
E 3
I 3
        flow_id = qam_id * NUM_FLOW_PER_QAM;
        flow_info = &flow_info_array[flow_id];
        num_tp = udp_hdr->length / TP_SIZE;
        process_mpt_payload((uint32)udp_hdr->payload, num_tp, flow_info);
E 3

    } else {
        // DEPI
D 3
        int qam_id = udp_hdr->dst_port & UDP_PORT_QAM_ID_MASK;
        int flow_id = qam_id * NUM_FLOW_PER_QAM + l2tp_hdr->flow_id;
        l2tp_hdr_t* l2tp_hdr = (l2tp_hdr_t*)udp_hdr->payload;
        flow_info_t* flow_info = &flow_info_array[flow_id];
E 3
I 3
        l2tp_hdr = (l2tp_hdr_t*)udp_hdr->payload;
        flow_id = qam_id * NUM_FLOW_PER_QAM + l2tp_hdr->flow_id;
        flow_info = &flow_info_array[flow_id];
E 3

        // Check for lost packet
        if (l2tp_hdr->seq_no != flow_info->prev_seq_no + 1) {
            // Sequence number error
        }
        flow_info->prev_seq_no = l2tp_hdr->seq_no;

D 3
        if (udp_hdr->dst_port & UDP_PORT_DEPI_PSP_MASK) {
            process_psp_payload(l2tp_hdr);
E 3
I 3
        if (qam_info->mode == QAM_MODE_DEPI_PSP) {
            process_psp_payload(l2tp_hdr, flow_info);
E 3
        } else {
D 3
            process_mpt_payload(l2tp_hdr);
E 3
I 3
            num_tp = udp_hdr->length / TP_SIZE;
            process_mpt_payload((uint32)l2tp_hdr->seg_table, num_tp, flow_info);
E 3
        }
    }
}


I 3
// Process a DEMP PSP packet
//
E 3
process_psp_payload(l2tp_hdr_t* ptr, flow_info_t* flow_info)
{
D 3
    psp_out_cmd_t* out_cmd = flow_info->out_cmd_cur_ptr;
    psp_seg_info_t* seg_info = ptr->seg_table;
E 3
I 3
    psp_out_cmd_t* out_cmd = (psp_out_cmd_t*)flow_info->out_cmd_cur_ptr;
    uint16* seg_info = ptr->seg_table;
E 3
    uint32 buf_addr;
    int seg_cnt = 0;
    int tot_seg_len = 0;
    int incomplete_frame;
D 2
    int i, j;
E 2
I 2
D 3
    register int i, j;
E 3
I 3
    int i, j;
E 3
E 2

    // Scan segment table to determine number of segments and starting address
    do {
        seg_cnt++;
D 3
    } while ( (seg_info++)->c_bit );
E 3
I 3
    } while ( (*seg_info++) & PSP_SEG_C_BIT );
E 3

    buf_addr = (uint32)seg_info;
D 3
    incomplete_frame = !(--seg_info->e_bit);    // end bit of last segment
E 3
I 3
    incomplete_frame = !((*--seg_info) & PSP_SEG_E_BIT);
                                  // end bit of last segment
E 3

    if (incomplete_frame) {
        seg_cnt--;
    }

    seg_info = ptr->seg_table;    // rewind segment table pointer

    // Process all the completed frames within the PSP packet
    for (i=j=0; i<seg_cnt; i++) {
        if (j == 0) {
            if (++out_cmd >= (psp_out_cmd_t*)flow_info->out_cmd_buf_end_addr) {
                out_cmd = (psp_out_cmd_t*)flow_info->out_cmd_buf_start_addr;
            }
            out_cmd->buf_addr = buf_addr;
D 3
            memset(out_cmd->seg_info, 0, 16);
E 3
I 3
            out_cmd->seg_info[1] = 0;
            out_cmd->seg_info[2] = 0;
            out_cmd->seg_info[3] = 0;
E 3
        }

D 3
        out_cmd->seg_info[j] = *seg_info;
        out_cmd->seg_info[j].c_bit = 1;
E 3
I 3
        out_cmd->seg_info[j] = *seg_info | PSP_SEG_C_BIT;
E 3

D 3
        buf_addr += seg_info->size;
E 3
I 3
        buf_addr += *seg_info & PSP_SEG_SIZE;
E 3
        seg_info++;

        if (++j == 4) {
            j = 0;
        }
    }
D 3
    flow_info->out_cmd_wr_ptr = out_cmd;
E 3
I 3
    if (seg_cnt > 0) {
        flow_info->out_cmd_wr_ptr = (uint32)out_cmd;
    }
E 3

    // Put the remaining incompleted segment, if there is any,
    // in another out command
    if (incomplete_frame) {
        if (++out_cmd >= (psp_out_cmd_t*)flow_info->out_cmd_buf_end_addr) {
            out_cmd = (psp_out_cmd_t*)flow_info->out_cmd_buf_start_addr;
        }
        out_cmd->buf_addr = buf_addr;
D 3
        out_cmd->seg_info[0] = *seg_info;
        out_cmd->seg_info[0].c_bit = 1;
        memset(out_cmd->seg_info + 1, 0, 12);
E 3
I 3
        out_cmd->seg_info[0] = *seg_info | PSP_SEG_C_BIT;
        out_cmd->seg_info[1] = 0;
        out_cmd->seg_info[2] = 0;
        out_cmd->seg_info[3] = 0;
E 3
    }
D 3
    flow_info->out_cmd_cur_ptr = out_cmd;
E 3
I 3
    flow_info->out_cmd_cur_ptr = (uint32)out_cmd;
E 3
}


D 3
process_mpt_payload(uint8* buf)
E 3
I 3
// Process a DEMP MPT packet or a Cisco bonding packet
//
process_mpt_payload(uint32 buf, uint32 num_tp, flow_info_t* flow_info)
E 3
{
I 3
    mpt_out_cmd_t* out_cmd = (mpt_out_cmd_t*)flow_info->out_cmd_cur_ptr;
    out_cmd->buf_addr = buf;
    out_cmd->num_tp = num_tp;
E 3
}

D 3
process_cisco_bonding_payload(uint8* buf)
{
}

E 3
E 1
