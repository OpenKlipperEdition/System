#
# this script only support x2000 platform
#
echo "1. take picture !!!!"
echo "2. preview picture to fb !!!"
echo "please Select a label for operation......For example 1"

for i in `seq -w 5 -1 0`
do
	if read -t 1  answer
	then
		break
	else
		echo -ne "\rCount down:$i s";
		# echo "Enter default Module"
		answer=1
	fi
done

case $answer in
"1")
	#configure kernel-menuconfig cim
	#cimutils -C -v cim -t grey -f 15.raw -l 0 -x 1920 -y 1080
	#configure kernel-menuconfig isp
	#cimutils -C -v isp -t nv12 -f 60.raw  -x 640 -y 480
	#cimutils -C -E helix -I /dev/video4 -t nv12  -f 50.jpg  -x 640 -y 480
	cimutils  -C -I /dev/video1 -I /dev/video4 -t nv12 -f 40.jpg -x 640 -y 480
	;;
"2")
	echo "test camera ... "
	m=`cat /sys/class/graphics/fb0/modes`
	m=${m#*:}
	m=${m%p*}

	w=${m%x*}
	h=${m#*x}

	let half_h=$h/2

	echo ${w}x${half_h} > /sys/devices/platform/ahb0/13050000.dpu/layer1/target_size
	echo ${w}x${half_h} > /sys/devices/platform/ahb0/13050000.dpu/layer1/src_size
	echo 0x${half_h} > /sys/devices/platform/ahb0/13050000.dpu/layer1/target_pos
	echo 6 > /sys/devices/platform/ahb0/13050000.dpu/layer1/src_fmt

	echo ${w}x${half_h} > /sys/devices/platform/ahb0/13050000.dpu/layer0/target_size
	echo ${w}x${half_h} > /sys/devices/platform/ahb0/13050000.dpu/layer0/src_size
	echo 0x0 > /sys/devices/platform/ahb0/13050000.dpu/layer0/target_pos
	echo 6 > /sys/devices/platform/ahb0/13050000.dpu/layer0/src_fmt

	echo 1 > /sys/devices/platform/ahb0/13050000.dpu/comp_update

	cimutils -P -x ${w} -y ${half_h} -t nv12 -I /dev/video5 -D /dev/fb0&
	sleep 1

	cimutils -P -x ${w} -y ${half_h} -t nv12 -I /dev/video8 -D /dev/fb1&
	sleep 15

	killall cimutils
	;;
*)
	echo "please check your input"
	;;
esac
