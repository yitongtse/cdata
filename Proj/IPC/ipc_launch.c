/*
    Copyright (c) 2006-2007 by Cisco Systems, Inc.
    All rights reserved.

    ipc_launch.c - IPC Launch Program for Cat4k
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <hw/inout.h>
#include <sys/neutrino.h>

#include INTERNAL_INC(lc/include_private/common.h)
#include INTERNAL_INC(lc/include_private/util.h)


#define EOBC_MAC_ADDR_BASE_STR	"02000000"

#define IONET_MODULE_PATH	"/proc/boot/"
#define IONET_DEVICE_PATH	"/dev/io-net/"


int main(int argc, char **argv, char **arge)
{
    int fd;
    int err;
    int slot_num;
    char mac_addr[13];
    char buffer[256];
    char *io_net_args[2];

    slot_num = get_slot_id();
    printf("Slot number %d\n", slot_num);

    sprintf(mac_addr, EOBC_MAC_ADDR_BASE_STR "%02x" "00", slot_num);

    io_net_args[0]="io-net";
    io_net_args[1]=NULL;

    printf("Starting io-net ...\n");
    err = spawnvpe(P_NOWAITO, "io-net", io_net_args, arge);
    if (err == -1) {
        perror("spawning io-net");
        return EXIT_FAILURE;
    }
  
    printf("Mounting npm-tcpip ...\n");
    do {
        delay(10);
        err = mount(IONET_MODULE_PATH "npm-tcpip.so", "/",
                    _MFLAG_OCB | _MOUNT_IMPLIED, "io-net", NULL, 0);
    } while (err == -1);

    printf("Waiting for " IONET_DEVICE_PATH "ip0 ...\n");
    while ((fd = open(IONET_DEVICE_PATH "ip0", O_RDONLY)) == -1) {
        delay(10);
    }
    close(fd);

    printf("Mounting devn-mpc85xx-mossbeach ...\n");
    err = mount(IONET_MODULE_PATH "devn-mpc85xx-mossbeach.so", "/",
                _MFLAG_OCB | _MOUNT_IMPLIED, "io-net",
                buffer, (strlen(buffer) + 1));
    if (err == -1) {
        perror("Mounting devn-mpc85xx-mossbeach");
        return EXIT_FAILURE;
    }
    printf("Mount option: %s\n", buffer);
    printf("Mount devn-mpc85xx-mossbeach.so OK\n");

    printf("Waiting for " IONET_DEVICE_PATH "en0 ...\n");
    while ((fd = open(IONET_DEVICE_PATH "en0", O_RDONLY)) == -1) {
        delay(10);
    }
    close(fd);

    printf("Mounting npm_cipc ...\n");
    err = mount(IONET_MODULE_PATH "npm_cipc.so", "/",
                _MFLAG_OCB | _MOUNT_IMPLIED, "io-net", NULL, 0);
    if (err == -1) {
        perror("Mounting npm_cipc");
        return EXIT_FAILURE;
    }

    printf("Waiting for " IONET_DEVICE_PATH "cipc0 ...\n");
    while ((fd = open(IONET_DEVICE_PATH "cipc0", O_RDONLY)) == -1) {
        delay(10);
    }
    close(fd);

    printf("Mounting ncm_cipc_en ...\n");
    err = mount(IONET_MODULE_PATH "ncm_cipc_en.so", "/",
                _MFLAG_OCB | _MOUNT_IMPLIED, "io-net", NULL, 0);
    if (err == -1) {
        perror("mounting npm_cipc_en");
        return EXIT_FAILURE;
    }

    sprintf(buffer, "ifconfig en1 inet 2.9.34.%d", slot_num + 40);
    err = system(buffer);
    if (err < 0) {
        perror("ifconfig failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

