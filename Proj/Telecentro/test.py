#! /usr/bin/python

import sys

file = open(sys.argv[1])
prev_br = 0

for line in file:
    tokens = line.split()
    br = int(line.split()[-1])
    diff = abs(br - prev_br)
    if diff > 10000:
        print line.rstrip()
    prev_br = br

