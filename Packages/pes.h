/* @(#)pes.h	1.1  07/15/98  @(#) */
/*
 *  ======== pes.h ========
 *
 *
 */
#ifndef	_PES_H_
#define	_PES_H_

#define	PES_HEADER_MINIMUM_SIZE	    9
#define	PES_MAX_HEADER_SIZE	    280

extern	Uint32	pesGetDTS ( Uint8 * pesBuf );

#endif	/* _PES_H_ */
