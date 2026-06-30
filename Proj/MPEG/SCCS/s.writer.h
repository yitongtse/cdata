h40991
s 00001/00001/00091
d D 2.3 02/05/24 12:26:40 ytse 5 4
c Update
e
s 00004/00000/00088
d D 2.2 02/03/21 13:28:40 ytse 4 3
c Backup for NEW2
e
s 00000/00000/00088
d D 2.1 00/08/21 11:04:28 ytse 3 2
c Before supporting Windows
e
s 00003/00003/00085
d D 1.2 99/11/02 17:53:57 yitong 2 1
c Fix sign
e
s 00088/00000/00000
d D 1.1 99/10/29 15:58:16 yitong 1 0
c date and time created 99/10/29 15:58:16 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef WRITER_H
#define WRITER_H


/* Implementation notes:

I 4
   The bitpos in Writer is used in the bit/byte writing functions.
   The bitPos in bitBuf is to be set by the empty function, and will
   be copied into the bitPos in Writer when putNextWord() is called.

E 4
   Note for the empty function
     The empty function is called by putNextWord() and flushWriter().
     When called by putNextWord(), endPtr will indicate the end of data.
     However, when called by flushWriter(), the end of data should be
     indicated by curPtr and bitPos instead.  Therefore, to work in both
     cases, emptyFunc() should use the curPtr and bitPos to determine
     the end of data.  The function should use other means to determine
     the beginning of data.  When bitPos is not 32, the remain part of
     the current word is 0 padded.  It is the function's responsibility
     to right it with other value if necessary.

     The function should return 0 if successful, and EOF if failed.
     When successful, it should set up the curPtr, endPtr and bitPos
     of bitBuf.
     curPtr should points to the word containing the beginning of data.
     bitPos should indicate the first available bit of buffer for writing.
     (bitPos = 32 means buffer is empty.  bitPos = 0 means buffer is full.
      bitPos should be > 0)
     In case bitPos < 32, it is emptyFunc()'s responsibility to ensure
     the buffer is RIGHT filled with 0.

   Issues:
     When the data buffer does not start at word boundary, bitPos<32.
     In this case, the data in buffer before the start bit position will
     be corrupted.  Therefore, putNextWord() needs to make sure only
     the data portion of buffer is written in the buffer in this case.
     The starting bit position is indicated in bitBuf.bitPos, while the
     actual bit being written is indicated by bitPos.
*/


typedef struct
{
    BitBuf bitBuf;
    int    bitPos;

    int    (*emptyFunc)(void*); // points to routine to empty buffer
    void   *emptyPar;		// parameter to be passed to emptyFunc

    int    errFlag;
 
    /* Debugging support */
    int    echoFlag;
    FILE   *echoFp;
    int    byteCnt;
}
Writer;


/* Set up
*/
void initWriter(Writer *wtr, int (*refill)(), void *par);
void flushWriter(Writer *wtr);
void closeWriter(Writer *wtr);

/* Supporting functions
*/
void byteAlignWriter(Writer *wtr);
void putNextWord(Writer *wtr);

/* Bit operations
*/
D 2
void _putBits(Writer *wtr, int nbits, int value);
void putBits(Writer *wtr, int nbits, int value, char *label);
E 2
I 2
void _putBits(Writer *wtr, int nbits, uint value);
void putBits(Writer *wtr, int nbits, uint value, char *label);
E 2

/* Byte operations
   Note: assumes bitBuf is byte aligned before calling
*/
void _putByte(Writer *wtr, int value);
void putByte(Writer *wtr, int value, char *label);

/* Debugging support
*/
void setWriterEcho(Writer *wtr, int echoFlag, FILE *echoFp);
void echoWriterPos(Writer *wtr);
D 2
void echoWriterBits(Writer *wtr, int nbits, int value, char *label);
E 2
I 2
void echoWriterBits(Writer *wtr, int nbits, uint value, char *label);
E 2
void echoWriterByte(Writer *wtr, int value, char *label);
void commentWriter(Writer *wtr, char *comment);
I 5
int getWriterBitPos(Writer *wtr);
E 5

D 5

E 5
#endif

E 1
