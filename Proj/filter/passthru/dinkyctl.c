#include <devctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/io-net.h>
#include "dinky.h"

char *devname = "/dev/io-net/en_en0";
unsigned dinkyflag = DROP_FLAG_ONWIRE;


int main(int argc, char **argv)
{
	int i, fd;
	unsigned cmd;
	struct dinky_rate rate;
	char *cp;
	
	for (i = 1; i < argc; i++) {
		
		if (strnicmp(argv[i], "dev=", 4) == 0) {
			if (*(cp = &argv[i][4]) != NULL) {
				devname = cp;
				do {
					cp++;
				} while (*cp && (*cp != ' ' || *cp != '\t'));
				*cp = NULL;
			}
			continue;
		}

		if (strnicmp(argv[i], "stat", 4) == 0) {
			struct dinky_stat stat;

			if ((fd = open(devname, O_RDONLY)) == -1) {
				fprintf(stderr, "Can't open device \"%s\" to read (%d).\n",
						devname, errno);
				return -1;
			}
			
			if (devctl(fd, DCMD_DINKY_GETSTAT, &stat, sizeof(stat), NULL) != EOK) {
				perror("devctl(DEMD_DINKY_GETSTAT)");
				return -1;
			}
			
			printf("Dinky Status:\n");
			printf("Inbound  packets: drop rate (%d/%d), total packets %d, dropped %d\n",
				   stat.irate.drop, stat.irate.total, 
				   stat.irate.pkt_all, stat.irate.pkt_dropped);
			printf("Outbound packets: drop rate (%d/%d), total packets %d, dropped %d\n",
				   stat.orate.drop, stat.orate.total, 
				   stat.orate.pkt_all, stat.orate.pkt_dropped);
			printf("Dinky flag: 0x%08x ", stat.dropflag);
			if (stat.dropflag) {
				printf("(");
				if (stat.dropflag & DROP_FLAG_ARP)    printf("ARP ");
				if (stat.dropflag & DROP_FLAG_IP)     printf("IP ");
				if (stat.dropflag & DROP_FLAG_TCP)    printf("TCP ");
				if (stat.dropflag & DROP_FLAG_UDP)    printf("UDP ");
				if (stat.dropflag & DROP_FLAG_QNET)   printf("QNET ");
				if (stat.dropflag & DROP_FLAG_ONWIRE) printf("ONWIRE");
				printf(")");
			}
			printf("\n");

			close(fd);
			continue;
		}
		
		if (strnicmp(argv[i], "flag=", 5) == 0) {
			cp = &argv[i][5];
			if (strnicmp(cp, "arp",  3) == 0) dinkyflag |= DROP_FLAG_ARP;
			if (strnicmp(cp, "ip",   2) == 0) dinkyflag |= DROP_FLAG_IP;
			if (strnicmp(cp, "tcp",  3) == 0) dinkyflag |= DROP_FLAG_TCP;
			if (strnicmp(cp, "udp",  3) == 0) dinkyflag |= DROP_FLAG_UDP;
			if (strnicmp(cp, "qnet", 4) == 0) dinkyflag |= DROP_FLAG_QNET;
			
			if ((fd = open(devname, O_WRONLY)) == -1) {
				fprintf(stderr, "Can't open device \"%s\" to write (%d).\n",
						devname, errno);
				return -1;
			}
			
			if (devctl(fd, DCMD_DINKY_SETFLAG, &dinkyflag, sizeof(dinkyflag), NULL) != EOK) {
				perror("devctl(DCMD_DINKY_SETFLAG)");
				return -1;
			}
			
			close(fd);
			continue;
		}

		cmd = 0;
		if (strnicmp(argv[i], "irate=", 6) == 0) {
			cp = &argv[i][6];
			cmd = DCMD_DINKY_SETIRATE;
		} else if (strnicmp(argv[i], "orate=", 6) == 0) {
			cp = &argv[i][6];
			cmd = DCMD_DINKY_SETORATE;
		} else if (strnicmp(argv[i], "rate=", 5) == 0) {
			cp = &argv[i][5];
			cmd = DCMD_DINKY_SETRATE;
		} else {
			fprintf(stderr, "Unknow option: \"%s\", ignored.\n", argv[i]);
			continue;
		}

		rate.total = rate.drop = 0;
		while (*cp && *cp != ',' && *cp != ' ' && *cp != '\t' && *cp != '/') {
			rate.drop = rate.drop * 10 + (*cp - '0');
			cp++;
		}
		if (*cp) cp++;
		while (*cp && *cp != ',' && *cp != ' ' && *cp != '\t') {
			rate.total = rate.total * 10 + (*cp - '0');
			cp++;
		}
			
		if ((fd = open(devname, O_WRONLY)) == -1) {
			fprintf(stderr, "Can't open device \"%s\" to write (%d).\n",
					devname, errno);
			return -1;
		}
			
		if (devctl(fd, cmd, &rate, sizeof(rate), NULL) != EOK) {
			perror("devctl(SET[IO]RATE)");
			return -1;
		}

		close(fd);
	}
	
	return 0;
}
  
