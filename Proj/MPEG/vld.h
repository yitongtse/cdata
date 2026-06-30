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

