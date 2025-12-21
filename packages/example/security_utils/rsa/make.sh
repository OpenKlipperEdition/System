#/bin/sh

rm build -rf;
mkdir build;
cd build;
cmake ../;
make;
scp rsa_nku user@192.168.4.21:~/out/zebra/
#scp rsa_nku user@192.168.4.102:~/wssong_hhh
