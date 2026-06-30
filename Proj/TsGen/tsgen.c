#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "param.h"


#define TP_SIZE                 188

typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;
typedef int64_t         int64;
typedef uint64_t        uint64;


// TP header
//
typedef struct tp_header_ {
    uint32 sync : 8;
    uint32 transport_err : 1;
    uint32 payload_unit_start : 1;
    uint32 priority : 1;
    uint32 pid : 13;
    uint32 scrambling_ctrl : 2;
    uint32 af_ctrl : 2;
    uint32 cc : 4;
} tp_header_t;


char filename[128];
int tp_cnt;
int pid;

uint32 tp_buf[TP_SIZE/4];



int main (int argc, char** argv)
{
    param_t par;
    int fd;
    tp_header_t *tp_hdr = (tp_header_t*)tp_buf;
    int i;

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "tsgen.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "Video Input Simulation");
    param_get_string(&par, "Output filename", filename, "test.m2t");
    param_get_int(&par, "Number of TPs to generate", &tp_cnt, 10000);
    param_get_int(&par, "PID", &pid, 100);
    param_end(&par);

    // Open content file
    fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd == -1) {
        printf("Failed to open output file %s: %s\n",
               filename, strerror(errno));
        exit(-1);
    }

    // Initialize TP buffer and header
    memset(tp_buf, 0, TP_SIZE);
    tp_hdr->sync = 0x47;
    tp_hdr->pid = pid;
    tp_hdr->af_ctrl = 1;


    for (i=0; i<tp_cnt; i++) {
        tp_buf[1] = i;
        write(fd, tp_buf, TP_SIZE);
        tp_hdr->cc++;
    }
}
