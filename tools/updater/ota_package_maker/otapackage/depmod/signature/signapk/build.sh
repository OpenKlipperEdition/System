##!/bin/sh
tar -xvf bouncycastle.tar.xz
mkdir classes
javac -cp .:bouncycastle/bouncycastle-bcpkix-host.jar:bouncycastle/bouncycastle-host.jar:commons-compress-1.18.jar  SignZip.java -d classes
#jar xvf bouncycastle/bouncycastle-bcpkix-host.jar -C class
cd classes
jar xvf ../bouncycastle/bouncycastle-bcpkix-host.jar
jar xvf ../bouncycastle/bouncycastle-host.jar
jar xvf ../commons-compress-1.18.jar
cd -
jar cvfm signapk.jar SignZip.mf -C classes .
rm classes -rf
tar -cJvf bouncycastle.tar.xz bouncycastle
rm bouncycastle -rf
