// DPAA Frame Descriptor (FD)
//   See Sec 21.3.1.1
//
typedef struct {
    uint32 dd : 2;              // debug trace mode
    uint32 pid : 6;             // partition ID
    uint32 bpid : 8;            // buffer pool ID
    uint32 addr_hi : 16;        // frame address (upper 8 bits)
    uint32 addr_lo;             // frame address (lower 8 bits)
    uint32 format : 3;          // frame format
    uint32 offset : 9;          // frame offset
    uint32 length : 20;         // frame length
    uint32 status_cmd;          // frame status / command
} dpaa_frame_descriptor_t;


// DPAA Frame Enqueue Response
//   See Sec 21.3.8.1
//
typedef struct {
    uint8 verb;                 // command verb
    uint8 dca;                  // discrete consumption acknowledgment
    uint16 seqnum;              // order restoration sequence number
    uint32 orp;                 // order restoration point ID (within 24 bits)
    uint32 fqid;                // frame queue ID (within 24 bits)
    uint32 tag;                 // enqueue command tag
    dpaa_frame_descriptor_t fd; // frame descriptor
} dpaa_enqueue_command_t;


// DPAA Frame Dequeue Response
//   See Sec 21.3.8.2
//
typedef struct {
    uint8 verb;                 // command verb
    uint8 stat;                 // status
    uint16 seqnum;              // order restoration sequence number
    uint8 tok;                  // dequeue command token
    uint8 reserved[3];
    uint32 fqid;                // frame queue ID (should be within 24 bits)
    uint32 cntxtb;              // frame queue context B
    dpaa_frame_descriptor_t fd; // frame descriptor
} dpaa_dequeue_response_t;


// DPAA Buffer Descriptor
//   See Sec 22.3.3.5.4
//
typedef struct {
    uint8 verb;                 // command verb
    uint8 bpid;                 // buffer pool ID
    uint16 addr_hi;             // buffer address (upper 16 bits)
    uint32 addr_lo;             // buffer address (lower 32 bits)
} dpaa_buffer_t;



// Release buffers to a Portal
//
// Assumptions:
//   - will use PI write mode, cache-enabled for production notification
//   - will support freeing of up to 8 buffers with different BPIDs
//   - non-blocking call
//
// Parameters:
//   - portal: virtual address of the SW portal used to release buffers
//   - buf: array of buffers to be release
//   - num_buf: number of buffers to be release
//
// Return codes:
//   - EOK: successful
//   - EINVAL: bad input parameters
//   - EAGAIN: RCR is currently full, should retry later
//
int dpaa_release_buffer(uint32 *portal, dpaa_buffer_t *buf, int num_buf);


// Enqueue a frame to a Frame Queue
//
// Assumptions:
//   - will use PI write mode, cache-enabled for ENCR production notification
//   - non-blocking call
//
// Parameters:
//   - portal: virtual address of the SW portal used for enqueue
//   - enc: enqueue command.  The caller should set up fqid and fd.
//
// Return codes:
//   - EOK: successful
//   - EINVAL: bad input parameters
//   - EAGAIN: ENCR currently full, should retry later
//
int dpaa_enqueue_frame(uint32 *portal, dpaa_enqueue_command_t *enc);


// Dequeue a frame from a Portal
//
// Assumptions:
//   - will support dequeueing from portal only
//   - will support DQRR scheduled Push Mode only
//   - non-blocking call
//
// Parameters:
//   - portal: virtual address of the SW portal used for dequeue
//   - dqr: buffer for returning the output dequeue response
//
// Return codes:
//   - EOK: successful
//   - EINVAL: bad input parameters
//   - EAGAIN: No frame currently available
//
int dpaa_dequeue_frame(uint32 *portal, dpaa_dequeue_response_t *dqr);

