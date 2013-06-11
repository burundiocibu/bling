#!/usr/bin/env python

from struct import unpack

fh = open('/dev/random', 'rb')


for i in range(300):
    print "{",
    addr = unpack("4B", fh.read(4))
    for j in range(len(addr)):
        print "0x{:02x}".format(addr[j]),
        if j<3: print ", ",
    print "}}, // slave {:d}".format(i)
