// Filename: huffman.c
//
// Description: This program reads the frequency data from the input file
//              in.dat, and construct the huffman code for it.
//

#include <stdio.h>


#define MAX_LINE_WIDTH  80
#define MAX_INT		0x7FFFFFFF
#define HUFF_TABLE_SIZE	1024


typedef struct HuffNode {
    int freq;		        // frequency of occurence
    int used;			// whether the node has been used
    int leftIdx;		// index of left node
    int rightIdx;		// index of right node
    unsigned int code;		// huffman code (left aligned)
    int len;			// number of bits in huffman code
} HuffNode;


HuffNode huffNode[HUFF_TABLE_SIZE];


// Print binary huffman code
//
void printHuffCode(unsigned int code, int len)
{
    while (len>0) {
        printf("%d", (code >> --len) & 1);
    }
}

// Find the two least frequent nodes to merge.
//
void mergeLeastFreqPair(int nentry)
{
    int i;
    int freq;
    int minFreq1 = MAX_INT;
    int minFreq2 = MAX_INT;
    int minIdx1, minIdx2;

    // Find the two least frequent nodes
    for (i=0; i<nentry; i++) {
        if (huffNode[i].used) {
            continue;
        }

        freq = huffNode[i].freq;
        if (freq < minFreq1) {
            // Move least entry to second least entry
            minFreq2 = minFreq1;
            minIdx2 = minIdx1;

            // Replace least entry
            minFreq1 = freq;
            minIdx1 = i;

        } else if (freq < minFreq2) {
            // Replace second least entry
            minFreq2 = freq;
            minIdx2 = i;
        }
    }

    // Generate new node
    huffNode[nentry].freq = minFreq1 + minFreq2;
    huffNode[nentry].used = 0;
    huffNode[nentry].leftIdx = minIdx1;
    huffNode[nentry].rightIdx = minIdx2;
    huffNode[nentry].code = 0;
    huffNode[nentry].len = 0;

    // Mark the merged nodes being used
    huffNode[minIdx1].used = 1;
    huffNode[minIdx2].used = 1;
}


// Assign huffman code recursively
//
void assignCode(idx)
{
    int idx2;
    int len = huffNode[idx].len;
    unsigned int code = huffNode[idx].code;

    // Transverse the left subtree
    idx2 = huffNode[idx].leftIdx;
    huffNode[idx2].len = len + 1;
    huffNode[idx2].code = code;
    if (huffNode[idx2].leftIdx != -1) {
        assignCode(idx2);
    }

    // Transverse the right subtree
    idx2 = huffNode[idx].rightIdx;
    huffNode[idx2].len = len + 1;
    huffNode[idx2].code = code | (1 << (31 - len));
    if (huffNode[idx2].leftIdx != -1) {
        assignCode(idx2);
    }
}


int main (int argc, char** argv)
{
    int i;
    char* s;
    int ncode = 0;
    char line[MAX_LINE_WIDTH];
    FILE *fp;

    // Assume input is in the file "in.dat"
    fp = fopen("in.dat", "r");

    // Read frequency data from input file
    while (1) {
        s = fgets(line, MAX_LINE_WIDTH, fp);
        if (s == NULL)  break;

        sscanf(line, "%d", &huffNode[ncode].freq);
        huffNode[ncode].used = 0;
        huffNode[ncode].leftIdx = -1;
        huffNode[ncode].rightIdx = -1;
        huffNode[ncode].code = 0;
        huffNode[ncode].len = 0;
        ncode++;
    }

    // Build the code tree
    for (i=0; i<ncode-1; i++) {
        mergeLeastFreqPair(ncode+i);
    }

    // Assign Huffman code
    assignCode(ncode * 2 - 2);

    // Output
    printf("\nFreq \tLen \tCode");
    for (i=0; i<ncode; i++) {
        printf("\n%d \t%d \t",
               huffNode[i].freq, huffNode[i].len, huffNode[i].code);
        printHuffCode((huffNode[i].code >> (32 - huffNode[i].len)),
                      huffNode[i].len);
    }
    printf("\n\n");

    return 0;
}


