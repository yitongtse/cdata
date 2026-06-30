# Update QNX image
flash_erase /dev/fs0p0 0 500000 
flash_write /dev/fs0p0 /flash/mb_app.lc 0 

# Restore QNX image
flash_erase /dev/fs0p0 0 500000 
flash_write /dev/fs0p0 /flash/mb_app.lc.keep 0 

# Update QNX perm image
flash_erase /dev/fs0p0 500000 50000
flash_write /dev/fs0p0 /flash/mb_app.lc 500000 

# Clear QNX flash partition
flash_erase /dev/fs0p1 0 800000

# Show CPU loads
od -td4 /tmp/cpuload

# Start flash driver
devf-generic -s 0xfe000000,0x2000000

