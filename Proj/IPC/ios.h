typedef uint ipc_seat;

struct ipc_seat_data_ {
    thread_linkage links;
    ipc_seat seat;                      /* The seat address */
    char *name;
    ipc_transport_type transport_type;  /* Method used to get there */
    ipc_send_vector send_vector;
    boolean interrupt_ok;
    ipc_transport_t ipc_transport;
    ipc_port_info *seat_master_info;    /* Info for seats master port */
    ulong mtu;

    ipc_sequence last_sent;             /* Last sequence number assigned */
    ipc_sequence last_heard;            /* Last valid sequence number heard
                                           from this seat */
    ipc_sequence last_xmitted;          /* Last message handed out to driver */
    ipc_sequence last_ack;              /* Last ack sequence number rcv'd */

    uint ack_pending;                   /* Number of pak waiting for ack */
    uint max_pak;                       /* Max number of packet for this seat*/

    ipc_message *reply_msg;             /* Reserved message for ACK/NACK */
    boolean enable_seq_window;          /* Enable/Disable seq window */
    thread_table *sess_waiting_table;   /* Table: sessions waiting for START*/
    void *issu_info;                    /* Issu session information  */
    boolean understand_versioning;      /* Seat understands versioning or not */
};
