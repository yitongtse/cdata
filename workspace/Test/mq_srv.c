#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <mqueue.h>


#define MAX_MSG         8
#define MSG_BUF_SZ      1023
#define MQ_NAME         "/mymq"


int main (void)
{
    struct mq_attr attr, attr2;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = MSG_BUF_SZ;
    attr.mq_curmsgs = 0;

    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq == -1) {
        perror("mq_open");
    }

    int done = 0;
    char buffer[MSG_BUF_SZ + 1];

    while (!done) {
        int nbytes = mq_receive(mq, buffer, MSG_BUF_SZ, NULL);
        if (nbytes > 0) { 
            printf("MSG: %s\n", buffer);
        } else {
            printf("nbytes = 0\n");
            sleep(1);
        }
        if (!strcmp(buffer, "exit")) {
            done = 1;
        }
    }
}
