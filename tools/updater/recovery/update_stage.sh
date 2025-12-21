#!/bin/sh
echo "in update_stages:param:$1,$2"
glob=`cat /tmp/glob`
case $1 in
    "UPDATING" )
        p=$(expr $2 / 10)
		let p*=10
        echo "+++ UPDATING $p"
		if [ x$glob != x$p ];then
			echo $p > /tmp/glob
		fi
        ;;
    "UPDATE_SUCCESS" )
        echo "+++ UPDATE_SUCCESS"
        ;;
    "UPDATE_TIMEOUT" )
        echo "+++ UPDATE_TIMEOUT"
        ;;
    "UPDATE_FAILURE" )
        echo "+++ UPDATE_FAILURE"
        ;;
    "UPDATE_START" )
        echo "+++ UPDATE_START"
        ;;
    "NETWORK_CONNECT_SUCCESS" )
        echo "+++ NETWORK_CONNECT_SUCCESS"
        ;;
    "NETWORK_CONNECT_FAILURE" )
        echo "+++ NETWORK_CONNECT_FAILURE"
        ;;
    "UPDATE_NET_CONFIG" )
        echo "+++ UPDATE_NET_CONFIG"
        ;;
    *)
        echo "unknown update stages!"
        ;;
esac
