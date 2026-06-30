#! /usr/bin/python

pidset = set([])
file = open('log')

for line in file:
    tokens = line.split()
    if tokens[-2] == 'pid':
        pidset.add(int(line.split()[-1]))

print sorted(pidset)

