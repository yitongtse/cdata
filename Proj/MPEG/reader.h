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

//    int    (*refillFunc)(void*);// points to routine to refill buffer
    int    (*refillFunc)();// points to routine to refill buffer
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
void flushBuffer(Reader *rdr);

/* Bit operations
*/
uint _getBits(Reader *rdr, int nbits);
void _skipBits(Reader *rdr, int nbits);
uint peekBits(Reader *rdr, int nbits);
void _editBits(Reader *rdr, int nbits, uint value);
uint getBits(Reader *rdr, int nbits, char *label);
void skipBits(Reader *rdr, int nbits, char *label);
void editBits(Reader *rdr, int nbits, uint value, char *label);

/* Byte operations
   Note: assumes bitBuf is byte aligned before calling
*/
int _getByte(Reader *rdr);
void _skipBytes(Reader *rdr, int nbytes);
void _editByte(Reader *rdr, uint value);
int getByte(Reader *rdr, char *label);
void skipBytes(Reader *rdr, int nbytes, char *label);
void editByte(Reader *rdr, uint value, char *label);

/* Debugging support
*/
void setReaderEcho(Reader *rdr, int echoFlag, FILE *echoFp);
void echoReaderPos(Reader *rdr);
void echoReaderBits(Reader *rdr, int nbits, uint value, char *label);
void echoReaderByte(Reader *rdr, int value, char *label);
void commentReader(Reader *rdr, char *comment);
int getReaderBitPos(Reader *rdr);	// position of the current bit in the source

#endif

