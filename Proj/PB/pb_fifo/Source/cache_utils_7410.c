// File : cache_utils_7410.c

#include "cache_utils_7410.h"
//#include "dinkusr.h"

// --------------------------------------------------------------------------
void show_cache_state(void) {
   register unsigned long spr_hid0;

   asm { mfspr spr_hid0,1008 }

   printf("\r\nInstruction cache is : ");
   if (spr_hid0 & HID0_ICE)
      printf("Enabled,");
   else
      printf("Disabled,");
   if (spr_hid0 & HID0_ILOCK)
      printf("Locked");
   else
      printf("Unlocked");

// -----------------------------
   printf("\r\nData cache is : ");
   if (spr_hid0 & HID0_DCE)
      printf("Enabled,");
   else
      printf("Disabled,");
   if (spr_hid0 & HID0_DLOCK)
      printf("Locked");
   else
      printf("Unlocked");

// -----------------------------
   printf("\r\nBranch Target Instruction Cache is : ");
   if (spr_hid0 & HID0_BTIC)
      printf("Enabled");
   else
      printf("Disabled");

// -----------------------------
   printf("\r\nBranch History Table is : ");
   if (spr_hid0 & HID0_BHT)
      printf("Enabled");
   else
      printf("Disabled");

// -----------------------------
   printf("\r\nStore Gathering is : ");
   if (spr_hid0 & HID0_SGE)
      printf("Enabled");
   else
      printf("Disabled");

// -----------------------------
   printf("\r\nSpeculative data cache and instruction cach is : ");
   if (spr_hid0 & HID0_SGE)
      printf("Disabled");
   else
      printf("Enabled");
}
// --------------------------------------------------------------------------
void prepare_L1_caches(void) {
   register unsigned long spr_read,spr_msr;

   asm { mfmsr spr_read }
   spr_msr = spr_read;                 // retain a copy 
   spr_read &= ~MSR_EE;
   asm { mtmsr spr_read }              // disable H/W interrupts

   asm { mfspr spr_read,1008 }         // read HID0 register
   if (spr_read & HID0_DCE) {          // If data cache is enabled
      // Check MPC7410UM/D Section 3.5.2 for details on Hardware Flush sequence
      asm { dssall }                   // Kill any outstanding data stream operations with dssall
//      asm { .long 0x7e00066c; }        // dssall is not recognized !
      asm { mfspr spr_read,1014 }      // Read MSSCR0
      spr_read |= MSSCR0_DL1HWF;
      __sync();                        // sync to finish any pending ld/st
      asm { mtspr 1014,spr_read }      // set L1-Data Cache Hardware Flush bit in Memory Sub-system Control register
      __sync();                        // sync to gurantee all data is written to BIU
      
      asm { mfspr spr_read,1008 }      // read HID0 register
      spr_read |= HID0_DCFI;
      asm { mtspr 1008,spr_read }      // Flash invalidate L1-data cache
      __isync();

      asm { mfspr spr_read,1008 }      // read HID0 register
      spr_read &= ~HID0_DCFI;;
      spr_read &= ~HID0_DCE;           // mask disable L1 data cache
      __sync();                        // make sure ld/st have completed
      asm { mtspr 1008,spr_read }
      __isync();
   }

   asm { mfspr spr_read,1008 }         // read HID0 register
   spr_read |= HID0_ICFI;
   spr_read |= HID0_ICE;               // Flash invalidate instruction cache
   __sync();
   asm { mtspr 1008,spr_read }
   __isync();

   asm { mfspr spr_read,1008 }         // read HID0 register
   spr_read &= ~HID0_ICFI;
   spr_read &= ~HID0_ICE;              // mask to disable L1 data cache
   __sync();                           // make sure ld/st have completed
   asm { mtspr 1008,spr_read }
   __isync();
   
   asm { mtmsr spr_msr }               // re-instate msr value, (restore external interrupt enable)
}

void enable_L1_caches(void) {
   register unsigned long spr_read;
   
   asm { mfspr spr_read,1008 }
   spr_read |= HID0_ICE;
   spr_read |= HID0_DCE;
   asm { mtspr 1008,spr_read }
}