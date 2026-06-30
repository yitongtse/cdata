/* @(#)utils.h	1.1  07/15/98  @(#) */
/*
 *  ======== utils.h ========
 *
 *
 */
#ifndef	_UTILS_H_
#define	_UTILS_H_

#define extU(reg, csta, cstb) ( ( (reg) << (csta) ) >> (cstb) )
#define setU(reg, csta, cstb) ( (reg) |= ( ((1<<((cstb)-(csta)+1)) - 1) << (csta) ) )
#define clrU(reg, csta, cstb) ( (reg) &= ~( ((1<<((cstb)-(csta)+1)) - 1) << (csta) ) )

extern  void myPrintf( Uint32 cases, char * fmt, ...);

#endif /* _UTILS_H_ */
