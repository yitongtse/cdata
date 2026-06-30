h13253
s 00004/00000/00278
d D 2.6 03/03/24 11:21:25 ytse 11 10
c Fixed a bug in copyReader(): forgot to copy the endPtr also.
e
s 00001/00001/00277
d D 2.5 02/05/24 12:26:44 ytse 10 9
c Update
e
s 00001/00000/00277
d D 2.4 02/03/21 13:28:40 ytse 9 8
c Backup for NEW2
e
s 00006/00000/00271
d D 2.3 02/02/19 11:12:48 ytse 8 7
c Added getBitPos() which returns current bit position in the source
e
s 00003/00002/00268
d D 2.2 00/08/21 11:23:55 ytse 7 6
c Added support of Windows
e
s 00000/00000/00270
d D 2.1 00/08/21 11:04:25 ytse 6 5
c Before supporting Windows
e
s 00002/00001/00268
d D 1.5 99/11/05 16:36:42 yitong 5 4
c Fix bug in _editBits()
e
s 00001/00001/00268
d D 1.4 99/11/05 16:07:11 yitong 4 3
c Fix typo in flushReader()
e
s 00052/00000/00217
d D 1.3 99/11/05 15:38:27 yitong 3 2
c Added editing feature
e
s 00001/00001/00216
d D 1.2 99/11/02 17:53:57 yitong 2 1
c Fix sign
e
s 00217/00000/00000
d D 1.1 99/10/29 15:58:11 yitong 1 0
c date and time created 99/10/29 15:58:11 by yitong
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
I 9
rdr->bitPos = 0;
E 9
}


void closeReader(Reader *rdr)
{
    if (rdr->echoFlag && rdr->echoFp!=NULL)
        fclose(rdr->echoFp);
}


D 7
static inline void advanceCurPtr(Reader *rdr)
E 7
I 7
static __inline void advanceCurPtr(Reader *rdr)
E 7
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


I 3
void flushBuffer(Reader *rdr)
{
    if (rdr->bitBuf.bitPos==32) { 
        *rdr->bitBuf.curPtr = rdr->bitBuf.buffer;
    } 
    else { 
        int temp = 32 - rdr->bitBuf.bitPos; 
D 4
        *rdr->bitBuf.curPtr != (rdr->bitBuf.buffer << temp) >> temp; 
E 4
I 4
        *rdr->bitBuf.curPtr |= (rdr->bitBuf.buffer << temp) >> temp; 
E 4
    } 
}


E 3
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
I 11

    // Note: the following is added to solve a bug in ipmpeg2dec.c
    //       Need to see if this breaks anything else!
    dstRdr->bitBuf.endPtr = srcRdr->bitBuf.endPtr;
E 11
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


I 3
void _editBits(Reader *rdr, int nbits, uint value)
{
    while ((nbits -= rdr->bitPos) > 0) {
D 5
        setBitField(&rdr->bitBuf.buffer, rdr->bitPos, 0, (value>>nbits)); 
E 5
I 5
        setBitField(&rdr->bitBuf.buffer, rdr->bitPos-1, rdr->bitPos,
                    (value>>nbits)); 
E 5
        flushBuffer(rdr);
        getNextWord(rdr); 
    }   
    nbits = -nbits; 
    setBitField(&rdr->bitBuf.buffer, rdr->bitPos-1, rdr->bitPos-nbits, value);
    rdr->bitPos = nbits; 
}
 

E 3
int _getByte(Reader *rdr)
{
D 7
    if (!rdr->bitPos)  getNextWord(rdr);
E 7
I 7
    if (!rdr->bitPos)  
		getNextWord(rdr);
E 7
    return (rdr->bitBuf.buffer >> (rdr->bitPos-=8)) & 0xFF;
}


/* Note: this version is only good for small number of skipped bytes */
void _skipBytes(Reader *rdr, int nbytes)
{
    _skipBits(rdr, nbytes<<3);
}


I 3
void _editByte(Reader *rdr, uint value)
{
    if (!rdr->bitPos) {
        flushBuffer(rdr);
        getNextWord(rdr);
    }
    setBitField(&rdr->bitBuf.buffer, rdr->bitPos-1, 8, value);
    rdr->bitPos -= 8;
}


E 3
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


D 2
void echoReaderBits(Reader *rdr, int nbits, int value, char* label)
E 2
I 2
void echoReaderBits(Reader *rdr, int nbits, uint value, char* label)
E 2
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


I 8
D 10
int getBitPos(Reader *rdr)
E 10
I 10
int getReaderBitPos(Reader *rdr)
E 10
{
    return (rdr->byteCnt << 3) + (32 - rdr->bitPos);
}


E 8
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


I 3
void editBits(Reader *rdr, int nbits, uint value, char *label)
{
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    _editBits(rdr, nbits, value);
    if (rdr->echoFlag && label!=NULL)  echoReaderBits(rdr, nbits, value, label);
}


E 3
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
I 3
}


void editByte(Reader *rdr, uint value, char *label)
{
    if (rdr->echoFlag && label!=NULL)  echoReaderPos(rdr);
    _editByte(rdr, value);
    if (rdr->echoFlag && label!=NULL)  echoReaderByte(rdr, value, label);
E 3
}

E 1
