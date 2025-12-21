#!/usr/bin/env python
from __future__ import print_function
import sys
import json
def help():
    print("%s ldscripts ldsldsfile mapfile0 mapfile1")

def genMap(fn,funcs):
    with open(fn, "r") as mapfd:
        for line in mapfd.readlines():
            s = line.split()
            for f in funcs:
                if f.startswith(s[2]):
                    d = int("0x" + s[0],16)
                    d = d & 0xffffffff
                    print ("%s = 0x%08x;" % (s[2],d))
        mapfd.close()
def genRamAddr(fn,sectname):
    datas = json.load(open(fn))
    if sectname in datas:
        value = datas[sectname]
        print ("sram_start = %s;" % (value['ramaddr']))
        print ("sram_size = %s;" % (value['ramsize']))
    return -1
def outLds(fn):
    with open(fn, "r") as mapfd:
        for line in mapfd.readlines():
            print(line,end='')
def main():
    funcs = []
    with open(sys.argv[1], "r") as mapfd:
        for line in mapfd.readlines():
            funcs.append(line)

    genMap(sys.argv[3],funcs)
    genRamAddr(sys.argv[4],sys.argv[5])
    outLds(sys.argv[2])
if __name__ == '__main__':
    main()
