#ifndef WRITER_H
#define WRITER_H


/* Implementation notes:

   The bitpos in Writer is used in the bit/byte writing functions.
   The bitPos in bitBuf is to be set by the empty function, and will
   be copied into the bitPos in Writer when putNextWord() is called.

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
void _putBits(Writer *wtr, int nbits, uint value);
void putBits(Writer *wtr, int nbits, uint value, char *label);

/* Byte operations
   Note: assumes bitBuf is byte aligned before calling
*/
void _putByte(Writer *wtr, int value);
void putByte(Writer *wtr, int value, char *label);

/* Debugging support
*/
void setWriterEcho(Writer *wtr, int echoFlag, FILE *echoFp);
void echoWriterPos(Writer *wtr);
void echoWriterBits(Writer *wtr, int nbits, uint value, char *label);
void echoWriterByte(Writer *wtr, int value, char *label);
void commentWriter(Writer *wtr, char *comment);
int getWriterBitPos(Writer *wtr);

#endif

