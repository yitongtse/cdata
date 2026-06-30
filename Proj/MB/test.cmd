# Evil test

video add in 0 udp 0 192.168.25.10 49152-49391 
video add in 240 udp 1 192.168.30.10 49152-49391 

video add out 0 0-239 0-23 1-10
video add out 240 240-479 24-47 1-10

video add out 480 0-239 0-23 11-30
video add out 960 0-239 24-46 11-30


# Clean up
video delete out 0-1439
video delete in 0-1439

on -Xaps=Critical aps show -lv -d 100 &

