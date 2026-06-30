/* binary to ascii utility function    */
/* Author: Shan Zhu    Date: 11/14/02  */


#include <stdio.h>
#include <stdlib.h>

#define __MAIN_FUNCTION__
#include "main.h"

#define ITEM_PER_LINE 4

Int32 main( Uint32 argc, Uint8 *argv[] ) {

  Uint8 infileName[128], outfileName[128];
  FILE *infilePtr, *outfilePtr;
  Uint32 i;
  Uint32 word;

  if (argc != 3) {
  help:
    printf( "Usage: b2a <input binary file> <output interger file>\n" );
    printf( "-- Changing binary file into a interger file as an array.\n" );
    exit(1);
  }

  sprintf( infileName, "%s", argv[1] );
  sprintf( outfileName, "%s", argv[2] );

  if ( (infilePtr=fopen(infileName, "rb")) == NULL ) {
    exithere( __FILE__, __LINE__, "can not open inputfile" );
  }

  if ( (outfilePtr=fopen(outfileName, "w")) == NULL ) {
    exithere( __FILE__, __LINE__, "can not open outputfile" );
  }

  fprintf( outfilePtr, "/* integer data of binary */\n" );
  fprintf( outfilePtr, "\n\n" );

  i=0;
  while ( 1 ) {
    if ( fread( &word, sizeof(Uint32), 1, infilePtr ) ) {
      fprintf( outfilePtr, "0x%08x, ", word );
      i++;
    } else {
      printf( "\n" );
      break;
    }

    if ( (i%ITEM_PER_LINE) == 0 ) {
      fprintf( outfilePtr, "\n" );
    }
  }

  fclose( infilePtr );
  fclose( outfilePtr );

  printf( "\n Done! \n" );

  return 1;
}

