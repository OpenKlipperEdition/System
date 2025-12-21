#!/usr/bin/env python
from __future__ import print_function
import sys
import os
def main():
    func = []
    for i in range(1,len(sys.argv)):
        fn = sys.argv[i]
        with open(fn,"rt") as fp:
            for f in fp.readlines():
                s = f.strip("\n")
                s = f.strip()
                if s not in func:
                    func.append(s)
            fp.close()

    for f in func:
        print("%s" % (f))
if __name__ == '__main__':
    main()
