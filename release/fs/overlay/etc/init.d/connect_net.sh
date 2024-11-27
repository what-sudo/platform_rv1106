#!/bin/sh

ENABLE_ETH=1
ENABLE_WLAN=0

function interface_up()
{
    ifconfig eth0 down >& /dev/null
    ifconfig wlan0 down >& /dev/null

    if [ ${ENABLE_ETH} = 1 ]; then
        ifconfig eth0 up
    fi
    if [ ${ENABLE_WLAN} = 1 ]; then
        insmod /lib/modules/ATBM606x_wifi_sdio.ko
    fi
}

function connect_net()
{
    changes_old=0
    changes_cur=0

    killall udhcpc >& /dev/null
    killall wpa_supplicant >& /dev/null

    if [ ${ENABLE_WLAN} = 1 ]; then
        wlan_path='/sys/class/net/wlan0/carrier'
        if [ -f $wlan_path ]; then
            ifconfig wlan0 up
            wpa_supplicant -D nl80211 -iwlan0 -c /etc/wpa_supplicant.conf -B &
            udhcpc -i wlan0 -R &
        fi
    fi

    while :
    do
        if [ ${ENABLE_ETH} = 1 ]; then
            changes_cur=`cat /sys/class/net/eth0/carrier_changes`
            if [ $changes_cur != $changes_old ]; then
                changes_old=$changes_cur
                eth0_sta=`cat /sys/class/net/eth0/carrier`
                if [ $eth0_sta = 1 ]; then
                    udhcpc -i eth0 -R &
                else
                    udhcpc_pid=`ps | grep "udhcpc -i eth0" | grep -v "grep" | awk '{print $1}'`
                    if [ $udhcpc_pid ]; then
                        echo "kill udhcpc, run command \"kill $udhcpc_pid\""
                        kill $udhcpc_pid
                    else
                        echo "'udhcpc -i eth0' not runner"
                    fi
                fi
            fi
        fi
        sleep 3
    done
}

network_init()
{
	ethaddr1=`ifconfig -a | grep "eth.*HWaddr" | awk '{print $5}'`
	if [ -f /configs/ethaddr.txt ]; then
		ethaddr2=`cat /configs/ethaddr.txt`
		if [ $ethaddr1 == $ethaddr2 ]; then
			echo "eth HWaddr cfg ok"
		else
			ifconfig eth0 down
			ifconfig eth0 hw ether $ethaddr2
		fi
	else
		echo $ethaddr1 > /configs/ethaddr.txt
	fi
}

echo ">>> Start connect_net"
echo ""
network_init
interface_up
connect_net
