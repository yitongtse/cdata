#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int slot = 3;
int port = 1;
int chan;
int prog;
int ses_id;


int main(int argc, char** argv)
{
    int i, j;

    for (chan = 1; chan <= 12; chan++) {
        printf("int qam %d/%d.%d\n", slot, port, chan);

        for (prog = 1; prog < 10; prog++) {
            printf("cable video ip 192.168.0.10 multicast SSM_%03d prog %d\n",
                   ses_id++, prog);
        }
    }
    return 0;
}
