#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"
#include "infile.h"
#include "vld.h"


#define MAX_NUM_CODEWORD	512
#define TERMINAL		0x80000000
#define ILLEGAL_CODEWORD	0x0000FFFF


static int index[MAX_NUM_CODEWORD];
static int length[MAX_NUM_CODEWORD];
static uint codeword[MAX_NUM_CODEWORD];
static int node[MAX_NUM_CODEWORD*2];


static uint buildSubtree(	/* returns node entry for parent */
    int idx1, int idx2,		/* codeword table indices to work on */
    int leftBitPos,		/* leftmost bit position of examin. window */
    int nbits,			/* number of bits to examine */
    int *availNodeIdx		/* index of next available codeword */
)
{
    int entry;			/* entry in node table */
    uint pattern;		/* group pattern */
    int tmpt = 0;		/* template from 0 to 2^nbits - 1 */
    int max;			/* 2^nbits - 1 */
    int nShift;			/* shift amount for pattern being examined */
    uint bitMask;		/* mask of bits under examination */
    int idx;
    int nodeIdx;		/* current node index */
    int parentEntry;
    int totBits = 0;		/* total number of bits examined so far */
    int maxLength;		/* longest #bits to examine */


    /* Determine nbits to use, and node table size for this level */
    for (idx=idx1; idx<=idx2; idx++)
        if (length[idx]>totBits)  totBits = length[idx];

    maxLength = totBits - (31-leftBitPos);
    if (maxLength<nbits)  nbits = maxLength;
    else  totBits = (31-leftBitPos) + nbits;

    parentEntry = (nbits<<16) | *availNodeIdx;	/* return parent node value */
    max = (1<<nbits) - 1;
    nShift = leftBitPos - nbits + 1;
    bitMask = ((1<<nbits)-1) << nShift;
    nodeIdx = *availNodeIdx;
    *availNodeIdx += 1 << nbits;

    /* Process group by group.  Within each group all the codewords have
       the same pattern in the first nbits */
    idx = idx1;
    while (idx<=idx2) {
        /* Find next group pattern (codewords are right filled with 0's) */
        pattern = codeword[idx] & bitMask;

        /* Assign illegal codewords for skipped patterns */
        while ((tmpt<<nShift) < pattern) {
            node[nodeIdx++] = TERMINAL | (totBits<<16) | ILLEGAL_CODEWORD;
				/* Assume 0 length for illegal codeword */
            if (++tmpt > max)  return parentEntry;
        }

        if (length[idx]<=totBits) {
            /* The group can only contain one element in this case */
            if (idx<idx2 && ((codeword[idx]^codeword[idx+1])
                >>(nShift+totBits-length[idx])==0)) {
                printf("Error: codewords are not prefix free!\n");
                exit(-7834);
            }

            /* Assign the codeword for all matching templates */
            entry = TERMINAL | (length[idx]<<16) | index[idx];
				/* Assume 0 length for illegal codeword */
            while ((((pattern>>nShift)^tmpt) >> (totBits-length[idx])) == 0) {
                node[nodeIdx++] = entry;
                if (++tmpt>max)  return parentEntry;
            }
            idx++;		/* next codeword */
        }

        else {
            /* Find all elements in this group */
            idx1 = idx;
            while ((++idx<=idx2) && (codeword[idx]&bitMask)==pattern);
            node[nodeIdx++] = buildSubtree(idx1, idx-1, leftBitPos-nbits,
                                           nbits, availNodeIdx);

                /* Note: nbits here should be same as input, otherwise this
                         statement will not be reached.
		*/
            idx1 = idx;
            tmpt++;
        }
    }

    /* Assign illegal codewords for remaining skipped patterns */
    while (tmpt++ <= max) {
        node[nodeIdx++] = TERMINAL | (totBits<<16) | ILLEGAL_CODEWORD;
				/* Assume 0 length for illegal codeword */
    }
    return parentEntry;
}


void buildVld(Vld* vld, char* vldTable, int nbits)
{
    char* ptr;
    int i, j;
    int ncw;            /* no. of codewords */
    int bitPos;         /* position of current bit */
    int minIdx, minIndex, minCw, minLen;
    int topEntry;

    /* Read in VLD table */
    ptr = vldTable;
    ncw = 0;
    bitPos = 31;
    codeword[0] = 0;

    for (i=strlen(vldTable); i>0; i--)
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
                index[ncw] = ncw;
                length[ncw++] = 31 - bitPos;
                bitPos = 31;
                codeword[ncw] = 0;
                break;
 
             default:
                printf("Invalid char %x encountered in VLC table!\n", *(ptr-1));
        }
 
    /* Sort table entry according to codeword pattern (left justified) */
    for (i=0; i<ncw-1; i++) {
        minIdx = i;
        minCw = codeword[i];
        for (j=i+1; j<ncw; j++)
            if (codeword[j]<minCw) {
                minIdx = j;
                minCw = codeword[j];
            }

        /* Swapping */
        minIndex = index[minIdx];
        minLen = length[minIdx];

        index[minIdx] = index[i];
        codeword[minIdx] = codeword[i];
        length[minIdx] = length[i];

        index[i] = minIndex;
        codeword[i] = minCw;
        length[i] = minLen;
    }

    /* Build decoder tree */
    vld->numNodes = 0;
    topEntry = buildSubtree(0, ncw-1, 31, nbits, &vld->numNodes);

    /* Copy decoder tree to structure */
    vld->node = (int*)malloc(vld->numNodes*sizeof(int));
    for (i=0; i<vld->numNodes; i++)
        vld->node[i] = node[i];
    vld->node[0] |= (topEntry<<5) & 0x03E00000;  /* set #bits for 1st lookup */
}


void loadVld(FILE *fp, Vld *vld)
{
    fread(&vld->numNodes, sizeof(int), 1, fp);
    vld->node = (int*)malloc(vld->numNodes*sizeof(int));
    fread(vld->node, sizeof(int), vld->numNodes, fp);
}
 
 
void saveVld(FILE *fp, Vld *vld)
{
    fwrite(&vld->numNodes, sizeof(int), 1, fp);
    fwrite(vld->node, sizeof(int), vld->numNodes, fp);
}
 

void printVld(Vld *vld)
{
    int i;
    printf("\nFirst lookup: %d bits\n", getBitField(vld->node[0],25,5));
    printf("Node     Entry     Terminal   nbits    value/next node\n");
    for (i=0; i<vld->numNodes; i++)
        printf("%03x    %08x        %d       %2d          0x%04x\n",
               i, vld->node[i], getBitField(vld->node[i],31,1),
               getBitField(vld->node[i],20,5), getBitField(vld->node[i],15,16));
}


int _getVld(Reader *in, Vld *vld)
{
    int totBits = 0;
    int* vldNode = vld->node;
    int nbits = getBitField(*vldNode, 25, 5);
    uint entry = vldNode[peekBits(in, nbits)];

    while (!(entry & 0x80000000)) {
        _skipBits(in, nbits);
        totBits += nbits;
        nbits = getBitField(entry, 20, 5);
        entry = vldNode[(entry & 0xFFFF) + peekBits(in, nbits)];
    }
    _skipBits(in, getBitField(entry,20,5)-totBits);
    return (entry & 0xFFFF);
}
 

int getVld(Reader *in, Vld *vld, char* label)
{
    int codeIdx, nbits, cwLen;
    uint entry;
    int totBits = 0;
    uint codeword = 0;
    int* vldNode = vld->node;

    if (in->echoFlag && label!=NULL)
        echoReaderPos(in);

    nbits = getBitField(*vldNode, 25, 5);
    entry = vldNode[peekBits(in, nbits)];

    while (!(entry & 0x80000000)) {
        codeword = (codeword << nbits) | _getBits(in, nbits);
        totBits += nbits;
        nbits = getBitField(entry, 20, 5);
        entry = vldNode[(entry & 0xFFFF) + peekBits(in, nbits)];
    }
    cwLen = getBitField(entry, 20, 5);
    nbits = cwLen - totBits;
    codeword = (codeword << nbits) | _getBits(in, nbits);
    codeIdx = entry & 0xFFFF;

    if (in->echoFlag && label!=NULL)
        fprintf(in->echoFp, "%30s  %2d  VLC: 0x%x -> 0x%x  ",
                label, cwLen, codeword, codeIdx);

    return (codeIdx);
}
 
