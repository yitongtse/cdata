h56676
s 00001/00001/00093
d D 2.4 02/05/24 12:26:27 ytse 7 6
c Update
e
s 00001/00000/00093
d D 2.3 02/02/19 11:12:49 ytse 6 5
c Added getBitPos() which returns current bit position in the source
e
s 00002/00001/00091
d D 2.2 00/08/21 11:23:55 ytse 5 4
c Added support of Windows
e
s 00000/00000/00092
d D 2.1 00/08/21 11:04:25 ytse 4 3
c Before supporting Windows
e
s 00006/00001/00086
d D 1.3 99/11/05 15:38:27 yitong 3 2
c Added editing feature
e
s 00001/00001/00086
d D 1.2 99/11/02 17:53:57 yitong 2 1
c Fix sign
e
s 00087/00000/00000
d D 1.1 99/10/29 15:58:15 yitong 1 0
c date and time created 99/10/29 15:58:15 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef READER_H
#define READER_H

#include "bitbuf.h"


/*  Implementation notes:

    The bitPos in Reader is used in the bit/byte reading functions.
    The bitPos in bitBuf is to be set by the refill function, and
    will be copied into the bitPos in Reader when getNextWord() is called.

    Conventions for the refill function:
    - The function should return 0 if successful, and EOF if failed.
    - When successful, it should set up the curPtr, endPtr and bitPos of bitBuf.
    - curPtr should be word-aligned, and points to the word containing
      the first bit of data.  The first bit of data may not be word- or
      even byte-aligned.
    - endPtr should be word-aligned, and points to the word immediately after
      the last bit of data.  This means the end of the data buffer must be
      word-aligned.
    - bitPos should indicate the first bit of data in *curPtr.
      bitPos = 32 means the MSB is the first available bit.
      bitPos should not be 0.
      In case bitPos<32, then *curPtr should be left filled by 0s.
*/


typedef struct
{
    BitBuf bitBuf;
    int    bitPos;              // current bit position

D 5
    int    (*refillFunc)(void*);// points to routine to refill buffer
E 5
I 5
//    int    (*refillFunc)(void*);// points to routine to refill buffer
    int    (*refillFunc)();// points to routine to refill buffer
E 5
    void   *refillPar;		// parameter to be passed to refillFunc

    uint   peekBuf;             // contains the next byte of payload
                                // in its most significant byte
    int    peekFlag;            // whether peekBuf content is updated
    int    errFlag;

    /* Debugging support */
    int    echoFlag;
    FILE   *echoFp;
    int    byteCnt;
}
Reader;


/* Set up
*/
void initReader(Reader *rdr, int (*refill)(), void *par);
void closeReader(Reader *rdr);

/* Supporting functions
*/
void flushReader(Reader *rdr);
void copyReader(Reader *srcRdr, Reader *dstRdr);
void byteAlignReader(Reader *rdr);
void getNextWord(Reader *rdr);
I 3
void flushBuffer(Reader *rdr);
E 3

/* Bit operations
*/
uint _getBits(Reader *rdr, int nbits);
void _skipBits(Reader *rdr, int nbits);
uint peekBits(Reader *rdr, int nbits);
I 3
void _editBits(Reader *rdr, int nbits, uint value);
E 3
uint getBits(Reader *rdr, int nbits, char *label);
void skipBits(Reader *rdr, int nbits, char *label);
I 3
void editBits(Reader *rdr, int nbits, uint value, char *label);
E 3

/* Byte operations
   Note: assumes bitBuf is byte aligned before calling
*/
int _getByte(Reader *rdr);
I 3
void _skipBytes(Reader *rdr, int nbytes);
void _editByte(Reader *rdr, uint value);
E 3
int getByte(Reader *rdr, char *label);
void skipBytes(Reader *rdr, int nbytes, char *label);
I 3
void editByte(Reader *rdr, uint value, char *label);
E 3

/* Debugging support
*/
void setReaderEcho(Reader *rdr, int echoFlag, FILE *echoFp);
void echoReaderPos(Reader *rdr);
D 2
void echoReaderBits(Reader *rdr, int nbits, int value, char *label);
E 2
I 2
void echoReaderBits(Reader *rdr, int nbits, uint value, char *label);
E 2
void echoReaderByte(Reader *rdr, int value, char *label);
void commentReader(Reader *rdr, char *comment);
I 6
D 7
int getBitPos(Reader *rdr);	// position of the current bit in the source
E 7
I 7
int getReaderBitPos(Reader *rdr);	// position of the current bit in the source
E 7
E 6
D 3

E 3

#endif

E 1
