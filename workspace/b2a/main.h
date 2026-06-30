typedef unsigned char       Uint8;
typedef unsigned short      Uint16;
typedef unsigned int        Uint32;
typedef          char       Int8;
typedef          short      Int16;
typedef          int        Int32;
typedef unsigned long       Uint64;
typedef          long       Int64;

#ifdef __MAIN_FUNCTION__
#define EXTERN 
#else
#define EXTERN extern
#endif

EXTERN void exithere ( Int8 *, Uint32, Int8 * );






