#!/bin/bash


win_url="ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip"
linux_url="ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz"



echo "=============Burner Urls:==========="
echo "Win: $win_url"
echo "Linux: $linux_url"
echo "==================================="

echo "Starts in 3 s"
echo 3
sleep 1
echo 2
sleep 1
echo 1
sleep 1

wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-ubuntu.tar.gz --ftp-user=ingenic_public --ftp-password=BFdg2f9B12
wget ftp://ftp.ingenic.com.cn/DevSupport/Tools/USBBurner/cloner-latest-windows.zip   --ftp-user=ingenic_public --ftp-password=BFdg2f9B12

