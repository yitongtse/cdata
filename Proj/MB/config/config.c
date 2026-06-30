#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int qd;                 // qam domain
int qb;                 // qam block


char* get_qam_block (int n)
{
    switch (n) {
        case 1: return "1-6";
        case 2: return "7-12";
        default: return "ERROR";
    }
}


// Basic video setup
//
void basic_video_setup (int slot)
{
    int port, chan;

    for (port = 1; port <= 12; port++) {
        printf("int qam %d/%d\n", slot, port);
        printf("no cable downstream rf-shutdown\n");
        printf("cable downstream stacking 4\n");
        printf("cable downstream modulation 256\n");
        printf("cable downstream annex B\n");

        for (chan = 1; chan <= 4; chan++) {
            printf("int qam %d/%d.%d\n", slot, port, chan);
            printf("no cable downstream rf-shutdown\n");
            printf("cable downstream modulation 256\n");
            printf("cable mode video local\n");
        }
    }
}


// Config multicast labels
void mcast_label_config (void)
{
    int i;
    for (i=1; i<=240; i++) {
        printf("ssm SSM1.%d source 1.1.1.1 group 232.1.0.%d g6/13\n", i, i);
    }
    for (i=1; i<=240; i++) {
        printf("ssm SSM2.%d source 1.1.1.2 group 232.2.0.%d g6/14\n", i, i);
    }
}


void mcast_session_config (void)
{
    int slot = 6, port, chan, qb;
    int i, n;

    qb = 1;
    n = 1;
    for (port = 1; port <= 12; port++) {
        if (n > 240) {
            n = 1;
            qb++;
        }

        printf("int qam %d/%d\n", slot, port);

        for (chan = 1; chan <= 4; chan++) {
            printf("int qam %d/%d.%d\n", slot, port, chan);

            for (i = 1; i <= 10; i++, n++) {
                printf("cable video multicast SSM%d.%d prog %d\n", qb, n, i);
            }
        }
    }
}


int main(int argc, char** argv)
{
    basic_video_setup(6);

//    mcast_label_config();
//    mcast_session_config();

    return 0;
}
