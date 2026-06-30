/*****************************************************************************
    File: pcap2ts.c

    Extract MPEG TS stream from a pcap file.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"

#define	TP_SIZE		188

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef enum {
    FALSE = 0,
    TRUE = 1
} boolean;


typedef struct {
    uint32 timestampHi;
    uint32 timestampLo;
    uint32 capturedLen;
    uint32 packetLen;
} pcap_hdr_t;


pcap_hdr_t pcapHdr;
uint8 pktBuf[2048];


uint32 switch_endian (uint32 x)
{
    return (x<<24) | ((x & 0xFF00) << 8) 
               | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24);
}


void print_hex (void* ptr, int nbytes)
{
    int i;
    unsigned char *temp = ptr;

    for (i=0; nbytes>0; nbytes--) {
        printf("%02x ", *temp++);
        if (++i==16) {
            i = 0;
            printf("\n");
        }
    }
    if (i) {
        printf("\n");
    }
}


void show_pcap_header (pcap_hdr_t* hdr)
{
    printf("Size: packet %d, captured %d\n", hdr->packetLen, hdr->capturedLen);
}


boolean check_sync (uint8* buf, int numTp)
{
    do {
        if (*buf != 0x47) {
            return FALSE;
        }
        buf += TP_SIZE;
    } while (--numTp == 0);
    return TRUE;
}


int main(int argc, char** argv)
{
    Param par;
    char inFile[256], outFile[256];
    FILE *inFp, *outFp;
    int nbytes;
    int pktCnt = 0;
    int skipCnt = 0;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "pcap2ts.par");
    getStringParam(&par, "Input IP/UDP/MPEG stream", inFile, "capture.pcap");
    getStringParam(&par, "Output filename", outFile, "capture.ts");
    endParam(&par);

    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
        printf("Error: Failed to open input file %s\n", inFile);
        exit (-1);
    }

    outFp = fopen(outFile, "w");
    if (outFp == NULL) {
        printf("Error: Failed to open output file %s\n", outFile);
        exit (-1);
    }

    // Skip pcap file header (24B)
    if (fseek(inFp, 24, SEEK_CUR) == -1) {
        printf("Error: Failed to skip file header\n");
        exit (-1);
    }

    while (1) {
        // Read pcap packet header
        nbytes = fread(&pcapHdr, 1, sizeof(pcap_hdr_t), inFp);
        if (nbytes == 0) {
           // EOF reached
           break;
        }

        if (nbytes != sizeof(pcap_hdr_t)) {
            printf("Error: Failed to skip frame header: %d\n", nbytes);
            exit (-1);
        }

        pcapHdr.packetLen = switch_endian(pcapHdr.packetLen) & 0xFFFF;
        pcapHdr.capturedLen = switch_endian(pcapHdr.capturedLen) & 0xFFFF;
//        printf("\nPkt %d: size %d", pktCnt, pcapHdr.packetLen);

        // Read packet content
        nbytes = fread(pktBuf, 1, pcapHdr.capturedLen, inFp);
        if (nbytes != pcapHdr.capturedLen) {
            printf("\nFailed to read from file: %d read\n", nbytes);
            return 0;
        }

        if (pcapHdr.packetLen != 1358) {
            printf("Packet %d: size %d, SKIP\n", pktCnt, pcapHdr.packetLen);
            skipCnt++;
            pktCnt++;
            continue;
        }

        fwrite(pktBuf + 42, 7, 188, outFp);

        // Check for SYNC
        if (!check_sync(pktBuf + 42, 7)) {
            printf("SYNC loss\n");
            exit (-1);
        }

        pktCnt++;
    }

    fclose(inFp);
    fclose(outFp);

    printf("\nSummary:\n");
    printf("Total packets: %d\n", pktCnt);
    printf("Skipped packets: %d\n", skipCnt);
    printf("Output packets: %d\n", pktCnt - skipCnt);
}
