# SUP setup
test ipc port create TestSUP 1

# QNX setup
ipc_test 1 &

# Open SUP test port for reliable non-blocking send
ipc_ctrl 1 open TestSUP 2

# Send messages
ipc_ctrl 1 send 4000 1200 1


------------------------------

show cable video message stat



