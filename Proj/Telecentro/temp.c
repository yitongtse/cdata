int32 video_get_flow_avail_space (uint16 qam_id, uint16 flow_id)
{
    int32 space_left;

    dma_ring_buf_t* ring = get_cmd_ring(qam_id, flow_id);
    ring->rd = bb_get_fpga_rdptr(qam_id, flow_id);

    space_left = OUTCMDS_PER_FLOW
                     - ring_mod(ring->wr - ring->rd, OUTCMDS_PER_FLOW) - 4;

    if (space_left < 0) {
        space_left = 0;
    }
    return space_left;
}

