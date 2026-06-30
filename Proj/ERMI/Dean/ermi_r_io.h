extern void ermi_r_io_connect(ermi_r_neighbor_t *nbr);
extern void ermi_r_io_disconnect(ermi_r_neighbor_t *nbr);
extern int ermi_r_io_send(ermi_r_neighbor_t *nbr, uchar *buf, 
                         int len);
extern int ermi_r_io_recv(ermi_r_neighbor_t *nbr, uchar *buf);
