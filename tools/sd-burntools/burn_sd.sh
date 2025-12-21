#!/bin/bash

if [ $# -lt 3 ]; then
#	echo "Usage:	pcburn <byte offset(MB)> <src file> <dst device, such as /dev/sdb>"
	echo "Usage:	pcburn <secotr offset> <src file> <dst device, such as /dev/sdb>"
	exit
fi

BURNOFFSET=$1
FILENAME=$2
TARGET=$3

FILESIZE=$(stat -c%s $FILENAME)
let FILESIZE=FILESIZE+511
let BURNCOUNT=FILESIZE/512

# MegaBytes -> Sector Num
#let BURNOFFSET=BURNOFFSET*1024*1024/512

echo $BURNOFFSET
echo $FILESIZE
echo $BURNCOUNT

dd if=$FILENAME of=$TARGET seek=$BURNOFFSET bs=512 count=$BURNCOUNT conv=notrunc
sync
sync
echo "finish!!!!!!!!!!!!!!!!!!!!"
