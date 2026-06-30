#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


// Ring buffer
//
typedef struct ring_buf_ {
    int rd;                     // read index (of next item to read)
    int wr;                     // write index (of next available item)
                                // Note: rd == wr means buffer is empty
} ring_buf_t;


int ring_mod (int idx, int size)
{
   return (idx + size) % size;
}


// Will update src_flow->rd and dst_flow->wr
// Assumption is that the destination always has enough space for the copy
//
void video_send_out_commands (ring_buf_t *src_flow, int src_flow_size,
                              ring_buf_t *dst_flow, int dst_flow_size)
{
    int src_rd, dst_wr;
    int src_size, dst_size;

    printf("Init: src: size %d, rd %d, wr %d;  dst: size %d, rd %d, wr %d\n",
           src_flow_size, src_flow->rd, src_flow->wr,
           dst_flow_size, dst_flow->rd, dst_flow->wr);

    do {
        // Set write limit depending on whether source buffer is wrapped around
        src_rd = (src_flow->wr < src_flow->rd)? src_flow_size : src_flow->wr;
        src_size = src_rd - src_flow->rd;

        // Set write limit depending on whether dest buffer is wrapped around
        dst_wr = (dst_flow->rd <= dst_flow->wr)? dst_flow_size : dst_flow->rd;
        dst_size = dst_wr - dst_flow->wr;

        if (dst_size < src_size) {
            // Adjust source size
            src_size = dst_size;
            src_rd = src_flow->rd + src_size;
        }

#if 0
        // Set up DMA link descriptor
        video_dma_set_desc(desc, (out_command_t*)src_flow->buf + src_flow->rd,
                           (out_command_t*)dst_flow.buf + dst_flow->wr,
                           src_size * sizeof(out_command_t));
#else
        printf("Copy %d items from src %d to dst %d\n",
               src_size, src_flow->rd, dst_flow->wr);
#endif

        // Update SW flow read pointer
        src_flow->rd = ring_mod(src_flow->rd + src_size, src_flow_size);
        dst_flow->wr = ring_mod(dst_flow->wr + src_size, dst_flow_size);

    } while (src_flow->rd != src_flow->wr);

    printf("Final: src: size %d, rd %d, wr %d;  dst: size %d, rd %d, wr %d\n\n",
           src_flow_size, src_flow->rd, src_flow->wr,
           dst_flow_size, dst_flow->rd, dst_flow->wr);
}


int main (int argc, char **argv)
{
    ring_buf_t src_flow, dst_flow;
    int src_flow_size, dst_flow_size;

    if (argc != 7) {
        printf("Usage: %s src_size src_rd src_wr dst_size dst_rd dst_wr\n",
               argv[0]);
        exit (-1);
    }

    src_flow_size = atoi(argv[1]);
    src_flow.rd = atoi(argv[2]);
    src_flow.wr = atoi(argv[3]);
    dst_flow_size = atoi(argv[4]);
    dst_flow.rd = atoi(argv[5]);
    dst_flow.wr = atoi(argv[6]);

    video_send_out_commands(&src_flow, src_flow_size, &dst_flow, dst_flow_size);
}

