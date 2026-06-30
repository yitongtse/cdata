/* @(#)mydefs.h	1.3  09/27/99  @(#) */
/*
 *  ======== mydefs.h ========
 *
 */
#ifndef _H_MYDEFS_
#define _H_MYDEFS_

#ifndef NULL
#define NULL        ((void *)0)
#endif  /* NULL */

#define __INLINE static __inline__

/* definition for bit position */
#define BIT0        0x1
#define BIT1        0x2
#define BIT2        0x4
#define BIT3        0x8
#define BIT4        0x10
#define BIT5        0x20
#define BIT6        0x40
#define BIT7        0x80
#define BIT8        0x100
#define BIT9        0x200
#define BIT10       0x400
#define BIT11       0x800
#define BIT12       0x1000
#define BIT13       0x2000
#define BIT14       0x4000
#define BIT15       0x8000
#define BIT16       0x10000
#define BIT17       0x20000
#define BIT18       0x40000
#define BIT19       0x80000
#define BIT20       0x100000
#define BIT21       0x200000
#define BIT22       0x400000
#define BIT23       0x800000
#define BIT24       0x1000000
#define BIT25       0x2000000
#define BIT26       0x4000000
#define BIT27       0x8000000
#define BIT28       0x10000000
#define BIT29       0x20000000
#define BIT30       0x40000000
#define BIT31       0x80000000

/* Define Generic Types with Specific Sizes */
typedef unsigned char       Uint8;
typedef unsigned short      Uint16;
typedef unsigned int        Uint32;
typedef          char       Int8;
typedef          short      Int16;
typedef          int        Int32;

typedef void    (*VoidFuncPtr)( void );
typedef Int32   (*IntFuncPtr) ( void );

typedef enum Boolean {
    FALSE,
    TRUE
} Boolean;

typedef	enum {
    VB_OK		    = 0,
    MEMORY_ALLOC_FAIL	    = -1000,
    BAD_PARAMETER	    = -1000 - 1,
    MODULE_NOT_INITIALIZED  = -1000 - 2,
    MODULE_NOT_IMPLEMENTED  = -1000 - 3
} Status;

enum {
    SHOW_ES_HEADER = 1,
    SHOW_TP_HEADER = 2,
    SHOW_BUF_INFO  = 4,
    SHOW_AUDIO_BUF = 8,
    VERBOSE        = 0x8000,
};

#endif  /* _H_MYDEFS_ */
