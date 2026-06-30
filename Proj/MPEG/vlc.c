#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "bitbuf.h"
#include "writer.h"
#include "vlc.h"


#define MAXNUMCODEWORD 512


void loadVlc(Vlc* vlc, char* vlcTable)
{
    char* ptr;
    int i;
    int ncw;		/* no. of codewords */
    int bitPos;		/* position of current bit */
    int length[MAXNUMCODEWORD];
    uint codeword[MAXNUMCODEWORD];

    ncw = 0;
    bitPos = 31;
    codeword[0] = 0;
    ptr = vlcTable;

    for (i=strlen(vlcTable); i>0; i--)
        switch (*ptr++) {
            case '1':
                codeword[ncw] |= 1 << bitPos;
					/* Note: No break here! */
            case '0':
                bitPos--;
					/* Note: No break here! */
            case ' ':
                break;

            case '\n': 
                length[ncw++] = 31 - bitPos;
                codeword[ncw] = 0;
                bitPos = 31;
                break;

             default:
                printf("Invalid char %x encountered in VLC table!\n", *(ptr-1));
        }

    vlc->size = ncw;
    vlc->length = malloc(ncw*sizeof(int));
    vlc->codeword = malloc(ncw*sizeof(int));
    for (i=0; i<ncw; i++) {
        vlc->length[i] = length[i];
        vlc->codeword[i] = codeword[i] >> (32-length[i]);  /* right-aligned */
    }
}


void _putVlc(Writer *out, Vlc *vlc, int index)
{
    _putBits(out, vlc->length[index], vlc->codeword[index]);
}


void putVlc(Writer* out, Vlc* vlc, int index, char* label)
{
    if (out->echoFlag && label!=NULL) {
        echoWriterPos(out);
        fprintf(out->echoFp, "%30s  %2d  VLC: 0x%x <- 0x%x  ",
                label, vlc->length[index], vlc->codeword[index], index);
    }
    _putVlc(out, vlc, index);
}


/* Diagnostics */
void printVlc(Vlc* vlc)
{
    int i;
    printf("\nVLC Dump:\n");
    printf("  VLC table size: %d\n", vlc->size);
    printf("  Index:  Length  Codeword\n");
    for (i=0; i<vlc->size; i++)
        printf("    %3d:   %2d     %04x\n",
               i, vlc->length[i], vlc->codeword[i]);
    printf("\n");
}



/* Sample program

char vlcTable[] =
  "1\n" \
  "01\n" \
  "001\n" \
  "0001 1\n" \
  "0001 0\n" \
  "0000 1\n" \
  "0000 01\n";


void main()
{
    Writer out;
    OutFile outFile;
    Vlc vlc;
    int i;

    loadVlc(&vlc, vlcTable);

    for (i=0; i<vlc.size; i++)
        printf("Code[%d]:\t%d\t%x\n", i, vlc.length[i], vlc.codeword[i]);

    openOutFile(&outFile, &out, "test.dat", 64);
    setWriterEcho(&out, 1, stdout);
    putVlc(&out, &vlc, 0, "VLC0");
    putVlc(&out, &vlc, 1, "VLC1");
    putVlc(&out, &vlc, 2, "VLC2");
    putVlc(&out, &vlc, 3, "VLC3");
    putVlc(&out, &vlc, 4, "VLC3");
    putVlc(&out, &vlc, 5, "VLC3");
    putVlc(&out, &vlc, 6, "VLC3");
    closeOutFile(&outFile);
}

*/
