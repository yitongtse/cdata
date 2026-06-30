#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define NUM_PORTS       8
#define PORT_DENSITY    32

#if 1
// Annex B
#define START_FREQ      91000000
#define FREQ_INC        6000000

#else

// Annex A
#define START_FREQ      98000000
#define FREQ_INC        8000000
#endif

int tsid = 1;


// Basic video setup
//
void basic_video_setup (int slot)
{
    int port, chan;
    int freq;
    int lbg;
    int lqam = 7;

    for (port = 1; port <= NUM_PORTS; port++) {
        printf("int qam %d/%d\n", slot, port);
        printf("cable downstream max-carriers %d\n", PORT_DENSITY);
        printf("y\n");
        printf("cable downstream start-freq %d\n", START_FREQ - FREQ_INC/2);
//        printf("cable downstream freq-profile default-freq-profile\n");

        freq = START_FREQ;

        for (chan = 1; chan <= PORT_DENSITY; chan++, freq+=FREQ_INC) {
            printf("int qam %d/%d.%d\n", slot, port, chan);
            printf("cable downstream tsid %d\n", tsid++);
            printf("cable downstream lqam-group %d\n", ++lqam / 8);
            printf("cable downstream rf-profile my-rf-profile\n");
            printf("cable downstream frequency %d\n", freq);

// Video config:
            printf("cable mode video local\n");

// DEPI config:
//            lbg = (port <= NUM_PORTS/2)? 1 : 2;
//            printf("cable mode depi local lbg %d\n", lbg);
//            printf("cable depi dest-ip 10.10.10.1\n");

            printf("no cable downstream rf-shutdown\n");
//            freq += 6000000;
        }
    }
}


int main(int argc, char** argv)
{
    basic_video_setup(3);

    return 0;
}
