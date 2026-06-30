# Update QNX image
flash_erase /dev/fs0p0 0 500000 
flash_write /dev/fs0p0 /flash/mv_app.lc 0 

# Update QNX perm image
flash_erase /dev/fs0p0 500000 500000
flash_write /dev/fs0p0 /flash/mv_app.lc 500000 

# Update Cobia image
flash_erase /dev/fs0p2 740000 200000
flash_write /dev/fs0p2 /flash/mv_cob.lc 740000

# Update Yellowfin image
flash_erase /dev/fs0p2 940000 280000
flash_write /dev/fs0p2 /flash/mv_yel.lc 940000

# Update Greatwhite image
flash_erase /dev/fs0p2 C00000 80000
flash_write /dev/fs0p2 /flash/mv_gwt.lc C00000

# Clear QNX flash partition
flash_erase /dev/fs0p1 0 800000

# Show CPU loads
od -td4 /tmp/cpuload

# Start flash driver
devf-generic -s 0xfe000000,0x2000000

# Configure the video qam channel
mv_msg_test -qam -q_id 0 -mode v -pwr 390 -freq 423000000
mv_msg_test -qam -q_id 0 -mode v -pwr 390 -frmt 256 -freq 723000000

# Copy QNX image to LC flash
copy tftp://223.255.254.254/ytse/mv_app.lc linecard-7-flash:

# Copy FPGA image to LC flash
copy tftp://223.255.254.254/ytse/fpga/mv_cob.lc linecard-7-flash:
copy tftp://223.255.254.254/ytse/fpga/mv_yel.lc linecard-7-flash:

# Copy linecard coredump to TFTP
copy linecard-7-flash:/mv_video.core tftp://223.255.254.254/ytse



