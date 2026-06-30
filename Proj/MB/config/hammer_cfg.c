#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int qd;                 // qam domain
int qb;                 // qam block
int slot;
int port;
int chan;
int qam;
int prog;
int udp;
int bitrate = 300000;
struct in_addr src_ip;
struct in_addr src_ip_shared;

int repeat = 10;

char filename[64];
char ip_addr[20];
FILE *fp;


void generate_script (void)
{
    for (qam=1; qam<=24; qam++) {
        src_ip_shared.s_addr = inet_addr("232.0.0.1");

#if 1
        // Delete all sessions in a qam channel
        for (prog=1; prog<=10; prog++) {
            fprintf(fp, "deletesession %02d%02d\n", qam, prog);
        }
        fprintf(fp, "\n");
#endif

        // Create all sessions in a qam channel
        for (prog=1; prog<=10; prog++, src_ip.s_addr+=(1<<24)) {

#if 0
            if (prog <= 2) {
                fprintf(fp, "createsession %02d%02d 1 %02d 1 %d %d 1 " \
                        "%s 1 2.2.2.2 0.0.0.0 0.0.0.0 0\n",
                        qam, prog, prog, qam, bitrate,
                        inet_ntoa(src_ip_shared));
                src_ip_shared.s_addr += 1 << 24;

            } else {
#endif
                fprintf(fp, "createsession %02d%02d 1 %02d 1 %d %d 1 " \
                        "%s 1 2.2.2.2 0.0.0.0 0.0.0.0 0\n",
                        qam, prog, prog, qam, bitrate, inet_ntoa(src_ip));
//            }
        }
        fprintf(fp, "\n");
    }
}


int main(int argc, char** argv)
{
    int i;
    int j;
    int port, chan;

    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        exit (-1);
    }

    for (i=1; i<=8; i++) {
        sprintf(filename, "script.dc%d", i);
        fp = fopen(filename, "w");
        if (fp == NULL) {
            printf("Failed to open output file %s\n", filename);
            return;
        }

        sprintf(ip_addr, "232.0.%d.1", i);
        src_ip.s_addr = inet_addr(ip_addr);

        for (j=0; j<repeat; j++) {
            generate_script();
        }

        fclose(fp);
    }
}

