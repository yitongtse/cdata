void snoop_pes (uint8 *tp_buf)
{
    uint8* tp_ptr;
    int remain_size;
    tp_header_t* tp_hdr = (tp_header_t*)tp_buf;

    // Check if TP has payload
    if (!(tp_hdr->af_ctrl & 1)) {
        printf("TP has no payload\n");
        exit (-1);
    }

    // Skip adaptation field if present
    tp_ptr = (uint8*)(tp_hdr + 1);
    if ((tp_hdr->af_ctrl & 2)) {
        tp_ptr += (*tp_ptr) + 1;
    }

    remain_size = TP_SIZE - (tp_ptr - (uint8*)tp_hdr);
    memcpy(pes_buf, tp_ptr, remain_size);
}
