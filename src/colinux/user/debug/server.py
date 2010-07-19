#!/usr/bin/env python

from socket import *
import struct, time, sys

log = open(sys.argv[1], "a+b")
s = socket(AF_INET, SOCK_DGRAM, 0)
s.bind(("0.0.0.0", 63000))
while 1:
    data, address = s.recvfrom(0x100)
    if len(data) == 0:
        break
    log.write(data)
