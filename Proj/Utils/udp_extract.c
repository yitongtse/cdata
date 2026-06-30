/*
   Copyright (c) 2007 by Cisco Systems, Inc.
   All rights reserved.

   File: extract.c
   Converts the UDP payload from an ethereal capture file

   Usage: extract <in_file> <dest_ip> <dest_udp> <out_file>
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;
typedef int64_t         int64;
typedef uint64_t        uint64;


#define ETHEREAL_FILE_HDR_SIZE          24
#define ETHEREAL_FRM_HDR_SIZE           16

#define ARP_SIZE                        28


#define ETH_TYPE_IP                    0x0800
#define ETH_TYPE_ARP                   0x0806

// Ethernet header
typedef struct ether_hdr_ {
    uint8       dst_mac[6];
    uint8       src_mac[6];
    uint16      type_or_len;
} ether_hdr_t;


#define IP_PROT_UDP                     17

// IP Header
typedef struct ip_header_ {
    uint8 ver : 4;
    uint8 ihl : 4;
    uint8 tos;
    uint16 len;
    uint16 identification;
    uint16 flags : 3;
    uint16 frag_offset : 13;
    uint8 ttl;
    uint8 protocol;
    uint16 hdr_chksum;
    uint32 src_ip;
    uint32 dst_ip;
} ip_header_t;


// UPD Header
typedef struct udp_header_ {
    uint16 src_port;
    uint16 dst_port;
    uint16 length;
    uint16 chksum;
} udp_header_t;


// Global variables
//
FILE *in_fp, *out_fp;
uint32 dst_ip;
uint16 dst_udp;

uint8 buffer[1500];
int frm_cnt = 0;


void print_hex (void* ptr, int nbytes)
{
    int i;
    uint8 *temp = ptr;

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


void process_udp ()
{
    udp_header_t udp_hdr;
    int size;

    size = fread(&udp_hdr, sizeof(udp_header_t), 1, in_fp);
    if (size < 1) {
        printf("UDP: End of file reached.\n");
        exit (0);
    }

    printf("UDP: dst %d", udp_hdr.dst_port);

    if (udp_hdr.dst_port != dst_udp) {
        printf(", skip\n");
        fseek(in_fp, udp_hdr.length - sizeof(udp_header_t), SEEK_CUR);
        return;
    }

    // Copy UDP payload to output file
    printf(", copy\n");
    fread(buffer, udp_hdr.length - sizeof(udp_header_t), 1, in_fp);
    fwrite(buffer, udp_hdr.length - sizeof(udp_header_t), 1, out_fp);
}


void process_ip_packet ()
{
    char ip_addr[INET_ADDRSTRLEN+1];
    ip_header_t ip_hdr;
    int size;

    size = fread(&ip_hdr, sizeof(ip_header_t), 1, in_fp);
    if (size < 1) {
        printf("IP: End of file reached.\n");
        exit (0);
    }

    inet_ntop(AF_INET, &ip_hdr.dst_ip, ip_addr, INET_ADDRSTRLEN);
    printf("IP pkt: dst %s, prot %d, len %d",
            ip_addr, ip_hdr.protocol, ip_hdr.len);
    if ((ip_hdr.dst_ip == dst_ip) && (ip_hdr.protocol == IP_PROT_UDP)) {
        printf("\n");
        process_udp();

    } else {
        printf(", skip\n");
printf("Before IP skip: %d\n", ftell(in_fp));
        fseek(in_fp, ip_hdr.len - sizeof(ip_header_t), SEEK_CUR);
printf("After IP skip: %d\n", ftell(in_fp));
    }
}


void process_ether_frame ()
{
    ether_hdr_t ether_hdr;
    int size;

    size = fread(&ether_hdr, sizeof(ether_hdr_t), 1, in_fp);
    if (size < 1) {
        printf("Eth: End of file reached.\n");
        exit (0);
    }

    printf("Eth frame: type %04x\n", ether_hdr.type_or_len);
    print_hex(&ether_hdr, sizeof(ether_hdr_t));

    switch (ether_hdr.type_or_len) {
    case ETH_TYPE_IP:
        printf("IP\n");
        process_ip_packet();
        break;

    case ETH_TYPE_ARP:
        printf("ARP\n");
        fseek(in_fp, ARP_SIZE, SEEK_CUR);
        break;

    default:
        printf("\nUnknown ethernet type\n");
    }
}


int main (int argc, char** argv)
{
    if (argc != 5) {
        printf("Usage: %s <in_file> <dst_ip> <dst_udp> <out_file>\n", argv[0]);
        exit (-1);
    }

    in_fp = fopen(argv[1], "rb");
    if (in_fp == NULL) {
        fprintf(stderr, "Failed to open input file %s\n", argv[1]);
        exit (-1);
    }

    out_fp = fopen(argv[4], "wb");
    if (out_fp == NULL) {
        fprintf(stderr, "Failed to open output file %s\n", argv[4]);
        exit (-1);
    }

    if (!inet_pton(AF_INET, argv[2], &dst_ip)) {
        fprintf(stderr, "Illegal dest IP address %s\n", argv[2]);
        exit (-1);
    }

    dst_udp = atoi(argv[3]);


    // Skip Ethereal capture file header (24 bytes)
    if (fseek(in_fp, ETHEREAL_FILE_HDR_SIZE, SEEK_CUR) == -1) {
        fprintf(stderr, "Error: Failed to skip Ethereal file header\n");
        exit (-1);
    }

    while (1) {
        if (fseek(in_fp, ETHEREAL_FRM_HDR_SIZE, SEEK_CUR) == -1) {
            fprintf(stderr, "Error: Failed to skip Ethereal frame header\n");
            exit (-1);
        }

        printf("\n\nFrame %d: position: %d\n", ++frm_cnt, ftell(in_fp));
        process_ether_frame();
    }
}



