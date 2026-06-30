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

wtr->bitPos = wtr->bitBuf.bitPos = 32;
wtr->bitBuf.buffer = 0;
}


static __inline void advanceCurPtr(Writer *wtr)
{
    if (wtr->bitBuf.bitPos==32) {
        *wtr->bitBuf.curPtr++ = wtr->bitBuf.buffer;
    }
    else {
        int temp = 32 - wtr->bitBuf.bitPos;
        *wtr->bitBuf.curPtr++ |= (wtr->bitBuf.buffer << temp) >> temp;
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
    wtr->bitPos &= 0x38;
}


/* Note: Next word write will be postpone as much as possible.
   Note: Assumes nbits <= 32, and value is 0 padded.
*/
void _putBits(Writer *wtr, int nbits, uint value)
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


void echoWriterBits(Writer *wtr, int nbits, uint value, char* label)
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


int getWriterBitPos(Writer *wtr)
{
    return (wtr->byteCnt << 3) + (32 - wtr->bitPos);
}


void putBits(Writer *wtr, int nbits, uint value, char *label)
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

