h17279
s 00000/00000/00039
d D 2.1 00/08/21 11:04:27 ytse 2 1
c Before supporting Windows
e
s 00039/00000/00000
d D 1.1 99/10/29 15:58:16 yitong 1 0
c date and time created 99/10/29 15:58:16 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef VLD_H
#define VLD_H


/*
  Node content:

    bits            Internal node          Terminal node
  -----------------------------------------------------------
     0-15         next node offset         decoded value
    16-20       #bits for next lookup     codeword length
      31                 0                       1

  Bits 21-25 of the first node contains the #bits for the first lookup.
  Invalid codeword pattern is indicated by decoded value==-1 (i.e., all ones)
*/


/* Variable Length Decoder
*/
typedef struct Vld
{
    int *node;		/* decoder tree node table */
    int numNodes;	/* number of nodes */
}
Vld;


void buildVld(Vld *vld, char *vldTable, int nbits);
void loadVld(FILE *fp, Vld *vld);       /* load in binary format */
void saveVld(FILE *fp, Vld *vld);       /* save in binary format */
void printVld(Vld *vld);

int _getVld(Reader *in, Vld *vld);
int getVld(Reader *in, Vld *vld, char *label);


#endif

E 1
