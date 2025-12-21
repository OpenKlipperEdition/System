#!/usr/bin/env python
import json
import sys
import struct
import os
import shutil

def writeData(value,dstfd,srcName,fname,diskaddr,collects):
    magic = "RTOS"
    version = 1
    statinfo = os.stat(srcName)
    disksize = int(statinfo.st_size)
    disksize = (disksize + 511) // 512 * 512
    fdata = struct.pack('4si',magic.encode("utf-8"),version)
    fdata += struct.pack('ii',collects[0],collects[1])
    fdata += struct.pack("40s",fname.encode("utf-8"))
    fdata += struct.pack("I",int(value["ramaddr"],16))
    fdata += struct.pack("I",int(value["ramsize"],16))
    diskaddr += 2048
    fdata += struct.pack("q",diskaddr)
    fdata += struct.pack("I",int(disksize))
    fdata += struct.pack("I",int(value["stacksize"],16))
    pos = dstfd.tell()
    dstfd.write(fdata);
    dstfd.seek(pos+2048);
    pos = dstfd.tell()
    srcfd = open(srcName,'rb')
    while True:
        d = srcfd.read(4096)
        if len(d) == 0:
            break
        dstfd.write(d)
    dstfd.seek(pos + disksize)
    srcfd.close()
    return disksize + 2048

def parseSize(s):
    u = s[-1]
    d = int(s[0:-1])
    if(u == 'M'):
        d = d * 1024 * 1024
    if(u == 'G'):
        d = d * 1024 * 1024 * 1024
    if(u == 'K'):
        d = d * 1024
    return d

def main():
    jsonname = sys.argv[1]
    outname = sys.argv[2]
    address = parseSize(sys.argv[3])
    datas = json.load(open(jsonname))
    dstfd = open(outname, 'wb')
    for i in range(4,len(sys.argv)):
        inFileName = sys.argv[i]
        fns = os.path.basename(inFileName)
        fn = fns.split('.')
        fname = fn[0]
        collects = [i - 4,len(sys.argv) - 4]
        sectname = fn[-2]
        if sectname in datas:
            value = datas[sectname]
            address = address + writeData(value,dstfd,inFileName,fname,address,collects)
        else:
            print(fname + ":" + sectname + "isn't finded.")
            return -1
    dstfd.close()
    return 0
if __name__ == '__main__':
    ret = main()
    exit(ret)

# ./sec2bin.py section.json outname diskaddr infiles
