#!/bin/bash

repo sync -j32

if [ $1 != "latest" ]; then
	repo forall -c "git reset --hard $1"
fi

vendor=`find device/ -name vendorsetup.sh`
combos=`cat $vendor  | grep "\-eng" | grep -v "#" | awk '{print $2}'`
release_dir=release-images-`date +%F_%H_%M_%S`

echo  "pwd `pwd`"

source build/envsetup.sh

for c in $combos
do
echo $c

lunch $c

make -j

if [ $? == 0 ]; then

mkdir -p $release_dir/$c

cp out/product/$TARGET_PRODUCT-$TARGET_BUILD_VARIANT/image/* $release_dir/$c -rf
cd $release_dir
tar jcvf $c.tar.bz2 $c
cd -

fi

done
