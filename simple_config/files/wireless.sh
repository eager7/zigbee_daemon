#!/bin/sh

sta_mt7628() {
#       detect_ralink_wifi mt7628 mt7628
		ssid=$1
		key=$2
		iwlist rai0 scanning > /dev/null
		iwpriv rai0 get_site_survey | grep $ssid > /dev/null
		if [ $? != 0 ];then
			echo "there is no this ap:$ssid"
			exit 2
		fi
		iwpriv rai0 get_site_survey | grep $ssid | awk '{print $4}' | grep NONE > /dev/null
		if [ $? == 0 ];then
			encrypt='NONE'
		fi
		iwpriv rai0 get_site_survey | grep $ssid | awk '{print $4}' | grep WPA2PSK > /dev/null
		if [ $? == 0 ];then
			encrypt='psk2'
		fi
		iwpriv rai0 get_site_survey | grep $ssid | awk '{print $4}' | grep WPAPSK > /dev/null
		if [ $? == 0 ];then
			encrypt='psk'
		fi
		
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
        option encryption $encrypt                             
        option ssid     $ssid                                  
        option key              $key
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
		ifconfig ra0 down
		rmmod mt7628
		insmod /lib/module/mt7628sta.ko
		if [ $? == 0 ];then
			ifconfig rai0 up
			sta_mt7628 $2 $3 > /etc/config/wireless
			wifi
		fi
		dhcp_ip
		;;
ap)
		ifconfig rai0 down
		rmmod mt7628sta
		insmod /lib/module/mt7628.ko
		if [ $? == 0 ];then
			ap_mt7628 > /etc/config/wireless
			ifconfig ra0 up
			wifi
		fi
		;;
dhcp)
		dhcp_ip;;
Exit)
        exit 0  ;;
esac
