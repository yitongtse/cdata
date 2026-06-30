#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <mqueue.h>


#define MAX_MSG         10
#define MAX_MSG_SZ      64
#define MQ_NAME         "/test_mq"

int main (void)
{
    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_MSG_SZ + 1];

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = MAX_MSG_SZ;
    attr.mq_curmsgs = 0;

    mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq == -1) {
        perror("mq_open");
    }
}
