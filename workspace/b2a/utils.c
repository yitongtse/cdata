#include <stdio.h>
#include <stdlib.h>
#include "main.h"

void exithere ( Int8 * place, Uint32 line, Int8 * message )
{
  printf( "EXIT in %s(line %d): \n --- %s\n", place, line, message );
  exit ( 1 );
}
 
