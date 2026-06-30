# Update TCC field upgradable QNX
#
flash_erase /dev/fs0p1 0 400000 
flash_write /dev/fs0p1 /flash/tcc_app.lc 0 


# Disable watchdog timer
#
mem_access w 0xa1000684 1 0x0

