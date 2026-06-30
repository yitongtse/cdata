#ifndef CACHE_UTILS_7410_H
#define CACHE_UTILS_7410_H

#define HID0		0x3f0      // HID0 Special Purpose Register #
#define HID0_EMCP         (unsigned long) 0x80000000
#define HID0_EBA          (unsigned long) 0x20000000
#define HID0_EBD          (unsigned long) 0x10000000
#define HID0_BCLK         (unsigned long) 0x08000000
#define HID0_ECLK         (unsigned long) 0x02000000
#define HID0_PAR          (unsigned long) 0x01000000
#define HID0_DOZE         (unsigned long) 0x00800000
#define HID0_NAP          (unsigned long) 0x00400000
#define HID0_SLEEP        (unsigned long) 0x00200000
#define HID0_DPM          (unsigned long) 0x00100000
#define HID0_RISEG        (unsigned long) 0x00080000
#define HID0_EIEC         (unsigned long) 0x00040000
#define HID0_NHR          (unsigned long) 0x00010000
#define HID0_ICE          (unsigned long) 0x00008000
#define HID0_DCE          (unsigned long) 0x00004000
#define HID0_ILOCK        (unsigned long) 0x00002000
#define HID0_DLOCK        (unsigned long) 0x00001000
#define HID0_ICFI         (unsigned long) 0x00000800
#define HID0_DCFI         (unsigned long) 0x00000400
#define HID0_SPD          (unsigned long) 0x00000200
#define HID0_IFTT         (unsigned long) 0x00000100
#define HID0_SGE          (unsigned long) 0x00000080
#define HID0_DCFA         (unsigned long) 0x00000040
#define HID0_BTIC         (unsigned long) 0x00000020
#define HID0_BHT          (unsigned long) 0x00000004
#define HID0_NOPDST       (unsigned long) 0x00000002
#define HID0_NOPPTI       (unsigned long) 0x00000001

#define MSR_EE            (unsigned long) 0x00008000

#define MSSCR0_DL1HWF     (unsigned long) 0x00800000

// L2 Control Register
#define L2CR         1017   

void show_cache_state(void);
void prepare_L1_caches(void);
void enable_L1_caches(void);

#endif

