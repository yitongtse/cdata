h62084
s 00006/00000/00141
d D 2.4 02/05/24 12:26:43 ytse 7 6
c Update
e
s 00003/00001/00138
d D 2.3 02/03/21 13:28:40 ytse 6 5
c Backup for NEW2
e
s 00001/00001/00138
d D 2.2 00/08/21 11:23:56 ytse 5 4
c Added support of Windows
e
s 00000/00000/00139
d D 2.1 00/08/21 11:04:28 ytse 4 3
c Before supporting Windows
e
s 00001/00001/00138
d D 1.3 99/11/05 15:39:15 yitong 3 2
c update
e
s 00003/00007/00136
d D 1.2 99/11/02 17:53:57 yitong 2 1
c Fix sign
e
s 00143/00000/00000
d D 1.1 99/10/29 15:58:12 yitong 1 0
c date and time created 99/10/29 15:58:12 by yitong
e
u
U
f e 0
t
T
I 1
#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "writer.h"


void initWriter(Writer *wtr, int (*emptyFunc)(), void *emptyPar)
{
    wtr->emptyFunc = emptyFunc;
    wtr->emptyPar = emptyPar;

    wtr->byteCnt = 0;
    wtr->errFlag = 0;
    wtr->echoFlag = 0;
    wtr->echoFp = NULL;
D 6
wtr->bitPos = 32;
E 6
I 6

wtr->bitPos = wtr->bitBuf.bitPos = 32;
wtr->bitBuf.buffer = 0;
E 6
}


D 5
static inline void advanceCurPtr(Writer *wtr)
E 5
I 5
static __inline void advanceCurPtr(Writer *wtr)
E 5
{
    if (wtr->bitBuf.bitPos==32) {
        *wtr->bitBuf.curPtr++ = wtr->bitBuf.buffer;
    }
    else {
        int temp = 32 - wtr->bitBuf.bitPos;
D 3
        *wtr->bitBuf.curPtr++ = (wtr->bitBuf.buffer << temp) >> temp;
E 3
I 3
        *wtr->bitBuf.curPtr++ |= (wtr->bitBuf.buffer << temp) >> temp;
E 3
    }
}


void flushWriter(Writer *wtr)
{
    advanceCurPtr(wtr);
    *wtr->bitBuf.curPtr--;
    wtr->errFlag = (*wtr->emptyFunc)(wtr->emptyPar);
}


void closeWriter(Writer *wtr)
{
    if (wtr->echoFlag && wtr->echoFp!=NULL)
        fclose(wtr->echoFp);
}


void putNextWord(Writer *wtr)
{
    advanceCurPtr(wtr);
    wtr->byteCnt += wtr->bitBuf.bitPos>>3;
    wtr->bitBuf.bitPos = 32;
    wtr->bitBuf.buffer = 0;

    if (wtr->bitBuf.curPtr >= wtr->bitBuf.endPtr)
        wtr->errFlag = (*wtr->emptyFunc)(wtr->emptyPar);

    wtr->bitPos = wtr->bitBuf.bitPos;
}


void byteAlignWriter(Writer *wtr)
{
D 2
/*
    int temp = wtr->bitBuf.bitPos & 7;
    if (temp)  wtr->bitBuf.bitPos -= 8 - temp;
*/
E 2
    wtr->bitPos &= 0x38;
}


/* Note: Next word write will be postpone as much as possible.
   Note: Assumes nbits <= 32, and value is 0 padded.
*/
D 2
void _putBits(Writer *wtr, int nbits, int value)
E 2
I 2
void _putBits(Writer *wtr, int nbits, uint value)
E 2
{
    while ((nbits -= wtr->bitPos) > 0) {
        wtr->bitBuf.buffer |= value >> nbits;
        putNextWord(wtr);
    }
    wtr->bitBuf.buffer |= value << (wtr->bitPos = -nbits);
}


void _putByte(Writer *wtr, int value)
{
    if (!wtr->bitPos)  putNextWord(wtr);
    wtr->bitBuf.buffer |= value << (wtr->bitPos-=8);
}


/* Note: if echoFp==NULL, simply set echoFlag and leave wtr->echoFp untouched
*/
void setWriterEcho(Writer *wtr, int echoFlag, FILE *echoFp)
{
    wtr->echoFlag = echoFlag;
    if (echoFp!=NULL) {
        if (wtr->echoFp != NULL)  fclose(wtr->echoFp);
        wtr->echoFp = echoFp;
    }
}


void echoWriterPos(Writer *wtr)
{
    int temp = 32 - wtr->bitPos;
    fprintf(wtr->echoFp, "\n%6d:%d  ", wtr->byteCnt+(temp>>3), temp%8);
}


D 2
void echoWriterBits(Writer *wtr, int nbits, int value, char* label)
E 2
I 2
void echoWriterBits(Writer *wtr, int nbits, uint value, char* label)
E 2
{
    fprintf(wtr->echoFp, "%30s  %2d  FLC: 0x%x  ", label, nbits, value);
}


void echoWriterByte(Writer *wtr, int value, char* label)
{
    fprintf(wtr->echoFp, "%30s  1*8  FLC: 0x%x", label, value);
}


void commentWriter(Writer *wtr, char *comment)
{ 
    if (wtr->echoFlag) 
        fprintf(wtr->echoFp, "%s", comment); 
}


I 7
int getWriterBitPos(Writer *wtr)
{
    return (wtr->byteCnt << 3) + (32 - wtr->bitPos);
}


E 7
D 2
void putBits(Writer *wtr, int nbits, int value, char *label)
E 2
I 2
void putBits(Writer *wtr, int nbits, uint value, char *label)
E 2
{
    if (wtr->echoFlag && label!=NULL)  echoWriterPos(wtr);
    _putBits(wtr, nbits, value);
    if (wtr->echoFlag && label!=NULL)  echoWriterBits(wtr, nbits, value, label);
}


void putByte(Writer *wtr, int value, char *label)
{
    if (wtr->echoFlag && label!=NULL)  echoWriterPos(wtr);
    _putByte(wtr, value);
    if (wtr->echoFlag && label!=NULL)  echoWriterByte(wtr, value, label);
}

E 1
