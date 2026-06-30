// Unicast session setup
char ucast_setup_msg[] =
    "SETUP rtsp://192.0.2.2/ RTSP/1.0\r\n"              \
    "Require: com.cablelabs.ermi\r\n"                   \
    "CSeq: 100\r\n"                                     \
    "Transport: "                                       \
        "clab-MP2T/DVBC/UDP"                            \
            ";unicast"                                  \
            ";bit_rate=2700000"                         \
            ";destination=192.0.2.1"                    \
            ";destination_port=4000,"                   \
        "clab-MP2T/DVBC/QAM"                            \
            ";qam_name=MSO.Division.Hub.1234"           \
            ";qam_destination=550000000.15\r\n"         \
    "clab-ClientSessionId:8cd50800200c9a66abcd\r\n"     \
    "\r\n"                                              \
;


// Multicast session setup
char mcast_setup_msg[] =
    "SETUP rtsp://192.0.2.2/ RTSP/1.0\r\n"              \
    "CSeq: 101\r\n"                                     \
    "Require: com.cablelabs.ermi\r\n"                   \
    "Transport: "                                       \
        "clab-MP2T/DVBC/QAM"                            \
            ";qam_name=Division.Hub.20"                 \
            ";qam_destination=550000000.15,"            \
        "clab-MP2T/DVBC/UDP"                            \
            ";multicast"                                \
            ";bit_rate=2700000"                         \
            ";source_addrss=2.2.2.2"                    \
            ";destination=192.0.2.10"                   \
            ";destination_port=100"                     \
            ";multicast_address=232.1.1.1"              \
            ";rank=10,"                                 \
        "clab-MP2T/DVBC/UDP"                            \
            ";multicast"                                \
            ";bit_rate=2700000"                         \
            ";source_addrss=4.4.4.4"                    \
            ";destination=192.0.2.11"                   \
            ";destination_port=102"                     \
            ";multicast_address=232.3.3.3"              \
            ";rank=2\r\n"                               \
    "clab-ClientSessionId:8cd50800200c9a66abcd\r\n"     \
    "\r\n"                                              \
;


// DEPI setup message
char depi_setup_msg[] =
    "SETUP rtsp://192.0.2.2/ RTSP/1.0\r\n"              \
    "CSeq: 102\r\n"                                     \
    "Require: com.cablelabs.ermi\r\n"                   \
    "Transport: "                                       \
        "clab-DOCSIS/QAM"                               \
            ";unicast"                                  \
            ";bit_rate=38000000"                        \
            ";depi_mode=docsis_mpt"                     \
            ";qam_tsid=123,"                            \
        "clab-DOCSIS/QAM"                               \
            ";unicast"                                  \
            ";bit_rate=38000000"                        \
            ";depi_mode=docsis_mpt"                     \
            ";qam_tsid=456\r\n"                         \
    "\r\n"                                              \
;



