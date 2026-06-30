#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"


void initReader(Reader *rdr, int (*refillFunc)(), void *refillPar)
{
    rdr->refillFunc = refillFunc;
    rdr->refillPar = refillPar;

    rdr->byteCnt = 0;
    rdr->errFlag = 0;
    rdr->peekFlag = 0;
    rdr->echoFlag = 0;
    rdr->echoFp = NULL;
rdr->bitPos = 0;
}


void closeReader(Reader *rdr)
{
    if (rdr->echoFlag && rdr->echoFp!=NULL)
        fclose(rdr->echoFp);
}


static __inline void advanceCurPtr(Reader *rdr)
{
    rdr->bitBuf.bitPos = 32;
    if (++rdr->bitBuf.curPtr >= rdr->bitBuf.endPtr)
        rdr->errFlag = (*rdr->refillFunc)(rdr->refillPar);
}


void flushReader(Reader *rdr)
{
    rdr->byteCnt += (rdr->bitBuf.endPtr - rdr->bitBuf.curPtr - 1) << 2;
    rdr->bitBuf.curPtr = rdr->bitBuf.endPtr - 1;
    rdr->bitPos = 0;
}


void flushBuffer(Reader *rdr)
{
    if (rdr->bitBuf.bitPos==32) { 
        *rdr->bitBuf.curPtr = rdr->bitBuf.buffer;
    } 
    else { 
        int temp = 32 - rdr->bitBuf.bitPos; 
        *rdr->bitBuf.curPtr |= (rdr->bitBuf.buffer << temp) >> temp; 
    } 
}


void copyReader(Reader *srcRdr, Reader *dstRdr)
{
    dstRdr->bitBuf.curPtr = srcRdr->bitBuf.curPtr;
    dstRdr->bitBuf.bitPos = srcRdr->bitPos;
    if (!dstRdr->bitBuf.bitPos) {
        dstRdr->bitBuf.curPtr++;
        dstRdr->bitBuf.bitPos = 32;
    }
    else { 
        // Left fill 1st word of payload with 0
        *srcRdr->bitBuf.curPtr &= 0xFFFFFFFF >> (32 - srcRdr->bitPos); 
    } 

    // Note: the following is added to solve a bug in ipmpeg2dec.c
    //       Need to see if this breaks anything else!
    dstRdr->bitBuf.endPtr = srcRdr->bitBuf.endPtr;
}


void getNextWord(Reader *rdr)
{
    rdr->byteCnt += rdr->bitBuf.bitPos>>3;

    if (rdr->peekFlag) {
        rdr->bitBuf.buffer = rdr->peekBuf;
        rdr->peekFlag = 0;
    }
    else {
        advanceCurPtr(rdr);
        rdr->bitBuf.buffer = *rdr->bitBuf.curPtr;
    }

    rdr->bitPos = rdr->bitBuf.bitPos;
}


/* Note: nbits must be from 0 to 8.
*/
uint peekBits(Reader *rdr, int nbits)
{
    uint value;
    int temp = rdr->bitPos - nbits;
    if (temp >= 0) {
        value = rdr->bitBuf.buffer >> temp;
    }
    else {
        if (!rdr->peekFlag) {
            advanceCurPtr(rdr);
            rdr->peekBuf = *rdr->bitBuf.curPtr;
            rdr->peekFlag = 1;
        }
        value = (rdr->bitBuf.buffer << (-temp))
                | (rdr->peekBuf >> (rdr->bitBuf.bitPos+temp));
    }
    temp = 32 - nbits;
    return (value<<temp) >> temp;
}


void byteAlignReader(Reader *rdr)
{
    rdr->bitPos &= 0x38;
}


/* Note: Next word read will be postpone as much as possible.
   Note: This version only support nbits <= 32.
*/
uint _getBits(Reader *rdr, int nbits)
{
    uint value = 0;
    int remainBits = nbits;

    while ((remainBits-=rdr->bitPos) > 0) {
        value |= rdr->bitBuf.buffer << remainBits;
        getNextWord(rdr);
    }
    value |= rdr->bitBuf.buffer >> (rdr->bitPos = -remainBits);
    remainBits = 32 - nbits;
    return (value << remainBits) >> remainBits;    /* clipping */
}


void _skipBits(Reader *rdr, int nbits)
{
    while ((nbits -= rdr->bitPos) > 0)  getNextWord(rdr);
    rdr->bitPos = -nbits;
}


void _editBits(Reader *rdr, int nbits, uint value)
{
    while ((nbits -= rdr->bitPos) > 0) {
        setBitField(&rdr->bitBuf.buffer, rdr->bitPos-1, rdr->bitPos,
                    (value>>nbits)); 
        flushBuffer(rdr);
        getNextWord(rdr); 
    }   
    nbits = -nbits; 
    setBitField(&rdr->bitBuf.buffer, rdr->bitPos-1, rdr->bitPos-nbits, value);
    rdr->bitPos = nbits; 
}
 

int _getByte(Reader *rdr)
{
    if (!rdr->bitPos)  
		getNextWord(rdr);
    return (rdr->bitBuf.buffer >> (rdr->bitPos-=8)) & 0xFF;
}


/* Note: this version is only good for small number of skipped bytes */
void _skipBytes(Reader *rdr, int nbytes)
{
    _skipBits(rdr, nbytes<<3);
}


void _editByte(Reader *rdr, uint value)
{
    if (!rdr->bitPos) {
        flushBuffer(rdr);
        getNextWord(rdr);
    }
    setBitField(&rdr->bitBuf.buffer, rdr->bitPos-1, 8, value);
    rdr->bitPos -= 8;
}


/* Note: if echoFp==NULL, simply set echoFlag and leave rdr->echoFp untouched
*/
void setReaderEcho(Reader *rdr, int echoFlag, FILE *echoFp)
{
    rdr->echoFlag = echoFlag;
    if (echoFp!=NULL) {
        if (rdr->echoFp != NULL)  fclose(rdr->echoFp);
        rdr->echoFp = echoFp;
    }
}


void echoReaderPos(Reader *rdr)
{
    int temp = 32 - rdr->bitPos;
    fprintf(rdr->echoFp, "\n%6d:%d  ", rdr->byteCnt+(temp>>3), temp%8);
}


void echoReaderBits(Reader *rdr, int nbits, uint value, char* label)
{
    fprintf(rdr->echoFp, "%30s  %2d  FLC: 0x%x  ", label, nbits, value);
}


void echoReaderByte(Reader *rdr, int value, char* label)
{
    fprintf(rdr->echoFp, "%30s  1*8  FLC: 0x%x", label, value);
}


void commentReader(Reader *rdr, char *comment)
{
    if (rdr->echoFlag)
        fprintf(rdr->echoFp, "%s", comment);
}


int getReaderBitPos(Reader *rdr)
{
    return (rdr->byteCnt << 3) + (32 - rdr->bitPos);
}


uint getBits(Reader *rdr, int nbits, char *label)
{
    uint value;
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    value = _getBits(rdr, nbits);
    if (rdr->echoFlag && label!=NULL)  echoReaderBits(rdr, nbits, value, label);
    return value;
}


void skipBits(Reader *rdr, int nbits, char *label)
{
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    _skipBits(rdr, nbits);
    if (rdr->echoFlag && label!=NULL)
        fprintf(rdr->echoFp, "%30s  %d  [skipped]", label, nbits);
}


void editBits(Reader *rdr, int nbits, uint value, char *label)
{
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    _editBits(rdr, nbits, value);
    if (rdr->echoFlag && label!=NULL)  echoReaderBits(rdr, nbits, value, label);
}


int getByte(Reader *rdr, char *label)
{
    int value;
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    value = _getByte(rdr);
    if (rdr->echoFlag && label!=NULL)  echoReaderByte(rdr, value, label);
    return value;
}


void skipBytes(Reader *rdr, int nbytes, char *label)
{
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    _skipBytes(rdr, nbytes);
    if (rdr->echoFlag && label!=NULL)
        fprintf(rdr->echoFp, "%30s  %d*8  [skipped]", label, nbytes);
}


void editByte(Reader *rdr, uint value, char *label)
{
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    _editByte(rdr, value);
    if (rdr->echoFlag && label!=NULL)  echoReaderByte(rdr, value, label);
}

