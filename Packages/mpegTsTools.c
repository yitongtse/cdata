/* @(#)mpegTsTools.c	1.2  09/27/99  @(#) */
#include   <stdio.h>
#include   <stdlib.h>
#include   <string.h>
#include   "mydefs.h"
#include   "utils.h"

void findTpPesLoc( Uint8 *pt8_tp, Uint32 resetFlag, Uint32 *pt_tpPyldLoc, Uint32 *pt_pesPyldLoc )
{
  static Uint32  pesHedrSize, pesHedrRead;

  Uint32 *pt32, tpPyldLoc, pesPyldLoc, hedrLgthLoc;


  /* if ask for reset */
  if ( resetFlag )
  {
    pesHedrSize = pesHedrRead = 0;
    return;
  }

  /* get the word pointer */
  pt32 = (Uint32 *) pt8_tp;

  /* find the TP payload location */
  /* if AF exist */
  if ( *pt32 & BIT5 )
    tpPyldLoc = 4 + 1 + extU( *(pt32 + 1), 0, 24 );
  else
    tpPyldLoc = 4;
  *pt_tpPyldLoc = tpPyldLoc;

  /* if TP carry the first byte of PES header */
  if ( *pt32 & BIT22 )
  {
    /* PES header will be at least 9 bytes long */
    pesHedrSize = 9;
    /* reset the PES header read counter */
    pesHedrRead = 0;
  }

  /* if there are PES header data left */
  if ( pesHedrSize > pesHedrRead )
  {
    /* if we are waiting for 'PES_header_data_length' */
    if ( pesHedrRead < 9 )
    {
      /* get the location (in byte) of 'PES_header_data_length' */
      hedrLgthLoc = tpPyldLoc + (8 - pesHedrRead);
      /* if 'PES_header_data_length' could be found in this TP */
      if ( hedrLgthLoc < 188 )
      {
	/* update the PES header size */
	pesHedrSize += extU( *(pt32 + (hedrLgthLoc >> 2)),
			     (hedrLgthLoc & 0x3) << 3, 24 );
      }
    }

    /* find the PES payload location */
    pesPyldLoc = tpPyldLoc + (pesHedrSize - pesHedrRead);
    pesPyldLoc = (pesPyldLoc > 188) ? 188 : pesPyldLoc;
    *pt_pesPyldLoc = pesPyldLoc;

    /* update the PES header read counter */
    pesHedrRead += (pesPyldLoc - tpPyldLoc);
  }
  else
  {
    *pt_pesPyldLoc = tpPyldLoc;
  }
}


#define    ADRS_HEAD     0xfffffffc
#define    ADRS_TAIL     0x00000003

void memByteCopy( Uint8 *dest_org, Uint32 size,	Uint8 *srce_org,
		  Uint32 ptBiasDest, Uint32 ptBiasSrce )
{
#if !LITTLE_ENDIAN
  Uint32   n;
#else
  Uint32   n, adrs;
  Uint8   *dest_new, *srce_new;
#endif

  for ( n = 0; n < size; ++n )
  {
#if !LITTLE_ENDIAN
    *dest_org = *srce_org;
#else
    adrs = (Uint32) dest_org - ptBiasDest;
    dest_new = (Uint8 *) (( adrs & ADRS_HEAD) +
			  (~adrs & ADRS_TAIL) + ptBiasDest);
    adrs = (Uint32) srce_org - ptBiasSrce;
    srce_new = (Uint8 *) (( adrs & ADRS_HEAD) +
			  (~adrs & ADRS_TAIL) + ptBiasSrce);
    *dest_new = *srce_new;
#endif
    dest_org++;
    srce_org++;
  }
}


void memByteFill( Uint8 *dest_org, Uint32 size,
		  Uint8 value, Uint32 pt32Bias )
{
#if !LITTLE_ENDIAN
  Uint32   n;
#else
  Uint32   n, adrs;
  Uint8   *dest_new;
#endif

  for ( n = 0; n < size; ++n )
  {
#if !LITTLE_ENDIAN
    *dest_org = value;
#else
    adrs = (Uint32) dest_org - pt32Bias;
    dest_new = (Uint8 *) (( adrs & ADRS_HEAD) +
			  (~adrs & ADRS_TAIL) + pt32Bias);
    *dest_new = value;
#endif
    dest_org++;
  }
}



#if LITTLE_ENDIAN
void wordReord( Uint32 *pt32, Uint32 wordNumb )
{
  Uint32   n, word;

  for ( n = 0; n < wordNumb; ++n )
  {
    word = pt32[ n ];
    pt32[ n ] =  (word << 24) |
                ((word <<  8) & 0x00ff0000) |
                ((word >>  8) & 0x0000ff00) |
                 (word >> 24);
  }
  return;
}
#endif
