#!/bin/sh

sta_mt7628() {
#       detect_ralink_wifi mt7628 mt7628
        if [ -e /etc/config/wireless ];then
         cat <<EOF
config wifi-device      mt7628sta
        option type     mt7628sta
        option vendor   ralink
        option ifname   rai0

config wifi-iface
        option device   mt7628sta
        option ifname   rai0
        option mode     sta
EOF
        fi
}

ap_mt7628() {
#       detect_ralink_wifi mt7628 mt7628
		ssid=mt7628-`ifconfig eth0 | grep HWaddr | cut -c 51- | sed 's/://g'`
        if [ -e /etc/config/wireless ];then
         cat <<EOF
config wifi-device      mt7628
        option type     mt7628
        option vendor   ralink
        option band     2.4G
        option channel  0
        option auotch   2

config wifi-iface
        option device   mt7628
        option ifname   ra0
        option network  lan
        option mode     ap
        option ssid     $ssid
        option encryption none
        #option encryption psk2
        #option key      12345678

EOF
        fi
}

dhcp_ip() {
	for i in {1..5}
	do
		ssid=`iwconfig rai0 | grep ESSID | awk '{print $4}'`
		if [ ssid != "ESSID:" ];then
		udhcpc -i rai0
		break
		sleep 1s
		fi
	done
}

case $1 in
sta)
        sta_mt7628 > /etc/config/wireless;;
ap)
        ap_mt7628 > /etc/config/wireless;;
dhcp)
		dhcp_ip;;
Exit)
        exit 0  ;;
esac