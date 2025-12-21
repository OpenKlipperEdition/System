#!/bin/sh -e
TOPDIR=${PWD}/
vendor=${TOPDIR}/example
#mkdir -p ${TOPDIR}/example/images/nand
#cp ../../../output/images/rootfs.ubifs ${vendor}/images/nand/
#cp ../../../../kernel-x1800/arch/mips/boot/compressed/xImage ${vendor}/images/nand/
keyname=releasekey
configdir=${vendor}/config
imagedir=${vendor}/images
srcpath=""
dstpath=""
keydir=${vendor}/security
publickey=${keydir}/${keyname}.x509.pem
privatekey=${keydir}/${keyname}.pk8

outdir=${vendor}/output_server

#
# for server
#
python -m otapackage --otamode="fullpkg" --output=$outdir --imgpath=$imagedir --configpath=$configdir --publickey=$publickey --privatekey=$privatekey
#python -m otapackage --otamode="diffpkg" --srcpath=$srcpath --dstpath=$dstpath --output=$outdir --configpath=$configdir --publickey=$publickey --privatekey=$privatekey

cd $outdir
find ./ -type  f  -name "*" | sort  > filelist.txt;
