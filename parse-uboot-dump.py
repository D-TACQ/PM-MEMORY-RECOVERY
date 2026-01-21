#!/usr/bin/env python3
import re
import sys

infile = sys.argv[1]
outfile = sys.argv[2]

i = open(infile,"r")
o = open(outfile,"wb")

for line in i.readlines():
    line = line.strip()
    if re.match(r'^[0-9a-f]{8}:',line):
        line = line.split(":")
        if len(line) == 2:
            line = line[1]
            line = line.replace(" ","")[:32]
            data = bytearray.fromhex(line)

            if len(data) != 16:
                print(f'len(data) {len(data)}')

            # original data is little endian words, fix it
            for i in range(0, len(data), 2):
                data[i:i+2] = reversed(data[i:i+2])

            if len(data) != 16:
                print(f'len(data) {len(data)}')

            o.write(data)

