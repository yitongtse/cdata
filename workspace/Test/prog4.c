#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>


typedef struct {
    const char *ptr;    // read pointer
    int len;            // remaining length in buffer
    int err;            // error code
} msg_parser_t;


#define MSG_PARSER_CHECK(psr,data_len)  \
    if ((psr)->err)  return;            \
    (psr)->len -= (data_len);           \
    if ((psr)->len < 0) {               \
        (psr)->err = EOF;               \
        return;                         \
    }


int msg_parser_err (msg_parser_t *psr, int data_len)
{
    if (psr->err)  return 1;
    (psr)->len -= (data_len);
    if ((psr)->len < 0) {
        (psr)->err = EOF;
        return 1;
    }
    return 0;
}


// Get a byte
static inline
void get_byte (msg_parser_t *psr, unsigned char *data)
{
    MSG_PARSER_CHECK(psr, 1)
//    if (msg_parser_err(psr, 1))  return;
    *data = *(psr->ptr)++;
}


// Get a short
static inline
void get_short (msg_parser_t *psr, unsigned short *data)
{
    MSG_PARSER_CHECK(psr, 2)
//    if (msg_parser_err(psr, 2))  return;
    *data = *(psr->ptr)++ << 8;
    *data |= *(psr->ptr)++;
}


int main()
{
    char msg_buf[5] = "Hello";
    msg_parser_t psr = {msg_buf, sizeof(msg_buf), 0};
    unsigned char data1;
    unsigned short data2;

    get_byte(&psr, &data1);
    get_short(&psr, &data2);

    printf("data1: %x\n", data1);
    printf("data2: %x\n", data2);
    printf("rc: %d\n", psr.err);

    get_byte(&psr, &data1);
    get_short(&psr, &data2);

    printf("data1: %x\n", data1);
    printf("data2: %x\n", data2);
    printf("rc: %d\n", psr.err);

    return 0;
}
