Problem du Jour: GF lost valuable data when a box crashed mid-shot. They are desperate to recover it.

Peter left SB to figure if it's possible to recover the memory from Linux after a reboot. Answer: it's not, there's a pattern in the memory. Where was the pattern introduced:

1.  Early boot FSBL or u-boot : OK, bad luck. 
2. Early kernel: dump from u-boot.

So #1 fails, let's try #2.


```
acq1102_057> more /proc/driver/acq400/0/buffers
Buffers
 ix,      va,        pa,     len,state
000,ed800000,0x2d800000,0x100000,0
001,eda00000,0x2da00000,0x100000,0
002,edb00000,0x2db00000,0x100000,0
003,edc00000,0x2dc00000,0x100000,0
004,edd00000,0x2dd00000,0x100000,0
```

u-boot works doesn't use VM, so it works with PA, not VA.

have a looksee at the data before we go down:

```
acq1102_057> cat /dev/acq400.0.hb/000 | hexdump | head
0000000 ffeb ffef ffe6 fff1 0001 fffd ffff ffcd
0000010 1001 e5fa c525 e0af ffeb ffef ffe6 fff1
0000020 0001 fffd ffff ffce 1002 e5fa c525 e0af
0000030 ffeb fff0 ffe6 fff2 0002 fffe ffff ffce
0000040 1003 e5fa c527 e0af ffeb fff0 ffe6 fff2
```

```
acq1102_057> cat /dev/acq400.0.hb/000 | wc -c
1048576

acq1102_057> cat /dev/acq400.0.hb/000 | sha1sum 
59578e1e854b7dddf9173898b99276c1803c74dd  -
acq1102_057> cat /dev/acq400.0.hb/000 | sha1sum 
59578e1e854b7dddf9173898b99276c1803c74dd  -
acq1102_057> cat /dev/acq400.0.hb/000 | sha1sum 
59578e1e854b7dddf9173898b99276c1803c74dd  -
acq1102_057> cat /dev/acq400.0.hb/000 | sha1sum 
59578e1e854b7dddf9173898b99276c1803c74dd  -
acq1102_057> sleep 10
acq1102_057> cat /dev/acq400.0.hb/000 | sha1sum 
59578e1e854b7dddf9173898b99276c1803c74dd  -
```

OK, ALL GOOD. Reboot and dump. 

```
acq2006-uboot> md.w 0x2d800000
2d800000: ffe9 ffee ffe5 fff2 0002 0000 0002 ffd1    ................
2d800010: 7801 ee89 53f3 e93c ffe9 ffee ffe6 fff2    .x...S<.........
2d800020: 0002 ffff 0002 ffd1 7802 ee89 53f4 e93c    .........x...S<.
2d800030: ffe9 ffee ffe6 fff2 0002 fffe 0002 ffd0    ................
2d800040: 7803 ee89 53f5 e93c ffe9 ffee ffe6 fff2    .x...S<.........
2d800050: 0002 ffff 0002 ffd1 7804 ee89 53f6 e93c    .........x...S<.
2d800060: ffe9 ffef ffe6 fff3 0002 ffff 0002 ffd1    ................
2d800070: 7805 ee89 53f7 e93c ffea ffef ffe6 fff2    .x...S<.........

```

OK, looks promising. HOW to offload?.

```
C-Kermit> log session acq1102_057_hb00.hexdump
C-Kermit> c
acq2006-uboot> md.w 0x2d800000 0x80000
2d800000: ffe9 ffee ffe5 fff2 0002 0000 0002 ffd1    ................
2d800010: 7801 ee89 53f3 e93c ffe9 ffee ffe6 fff2    .x...S<.........
2d800020: 0002 ffff 0002 ffd1 7802 ee89 53f4 e93c    .........x...S<.
2d800030: ffe9 ffee ffe6 fff2 0002 fffe 0002 ffd0    ................
2d800040: 7803 ee89 53f5 e93c ffe9 ffee ffe6 fff2    .x...S<.........

```
7 minutes later..

```
-rw-r--r-- 1 pgm pgm 4588695 Jan 21 09:51 acq1102_057_hb00.hexdump
```

Use vi to trim old commands off the top (if present). 
How to decode:

https://github.com/nmatt0/firmwaretools/blob/master/parse-uboot-dump.py

```
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ time ./parse-uboot-dump.py acq1102_057_hb00.hexdump acq1102_057_hb00.bin

real	0m0.223s
```
```
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ ls -l
total 5508
-rw-r--r-- 1 pgm pgm 1043408 Jan 21 09:53 acq1102_057_hb00.bin

```

Hmm, we're a little bit short here, worse:

```
Worse, original data was little endian words, and now it's a little-endian-mess:

(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ hexdump acq1102_057_hb00.bin |head
0000000 e9ff eeff e5ff f2ff 0200 0000 0200 d1ff
0000010 0178 89ee f353 3ce9 e9ff eeff e6ff f2ff
0000020 0200 ffff 0200 d1ff 0278 89ee f453 3ce9
0000030 e9ff eeff e6ff f2ff 0200 feff 0200 d0ff
```
OK, we can fix that:

```
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ cat ./parse-uboot-dump.py
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

            # original data is little endian words, fix it
            for i in range(0, len(data), 2):
                data[i:i+2] = reversed(data[i:i+2])

            o.write(data)

```


```
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ time ./parse-uboot-dump.py acq1102_057_hb00.hexdump acq1102_057_hb00.bin

real	0m0.223s

```
Head looks OK:
```
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ hexdump acq1102_057_hb00.bin | head
0000000 ffe9 ffee ffe5 fff2 0002 0000 0002 ffd1
0000010 7801 ee89 53f3 e93c ffe9 ffee ffe6 fff2
0000020 0002 ffff 0002 ffd1 7802 ee89 53f4 e93c
0000030 ffe9 ffee ffe6 fff2 0002 fffe 0002 ffd0
0000040 7803 ee89 53f5 e93c ffe9 ffee ffe6 fff2
0000050 0002 ffff 0002 ffd1 7804 ee89 53f6 e93c
```

But we're ~5000 short:

```
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ ls -l
total 5508
-rw-r--r-- 1 pgm pgm 1043152 Jan 21 09:59 acq1102_057_hb00.bin

python
>>> 0x100000 - 1043152
5424
```

Hmm, so the sha1 is never going to match. Where's the hole?. Presumably some bytes got dropped in transit.  Hmm, but it didn't:

```
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ wc -l acq1102_057_hb00.hexdump
65536 acq1102_057_hb00.hexdump

```


=> exactly 65536 lines!  So the python extract is faulty.   Personally, I don't trust python with binary data. Maybe one of you guys can spot the fault. Maybe it's stripping leading zeros or something really stupid!  I printed len(data) before and after, it's ALWAYS 16..

OK, let's try a C++ program. Here it is, with lost line validation!

```
https://github.com/D-TACQ/PM-MEMORY-RECOVERY


(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ ./parse-uboot-dump acq1102_057_hb00.hexdump acq1102_057_hb00.bin
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ ls -l
total 5528
-rw-r--r-- 1 pgm pgm 1048576 Jan 21 11:05 acq1102_057_hb00.bin
-rw-r--r-- 1 pgm pgm 4587520 Jan 21 10:16 acq1102_057_hb00.hexdump
-rwxr-xr-x 1 pgm pgm   12984 Jan 21 11:05 parse-uboot-dump
-rw-r--r-- 1 pgm pgm    1296 Jan 21 10:54 parse-uboot-dump.cpp
-rwxr-xr-x 1 pgm pgm     745 Jan 21 10:13 parse-uboot-dump.py
(base) pgm@hoy6:~/SANDBOX/GF-MEMORY-RECOVERY$ sha1sum acq1102_057_hb00.bin
e88c3e02d45d743f4bea3119fcd767b90622f963  acq1102_057_hb00.bin

```

The .bin length is correct, the top and tail look plausible, but the sha1 is different.  
Not sure where the exact difference is.  I'd same the u-boot method is good, but something in the boot 
eg probing the DRAM for size info, made some small change to the data. Maybe the user is able to work around a couple of duff samples..


