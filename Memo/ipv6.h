// IPv6 Header
//
typedef struct ipv6_header_ {
    uint32 ver : 4;
    uint32 traffic_class : 8;
    uint32 flow_label : 20

    uint16 payload_length;
    uint8 next_header;
    uint8 hop_limit;

    uint32 src_addr[4];
    uint32 dst_addr[4];
} ipv6_header_t;

