#!/bin/sh

major=`cat /proc/devices | grep gpio | awk '{print $1}'`
if [ ! -n "$major" ];then
        echo -e "can't get major"
        exit 1
fi 
echo -e "major:$major"
mknod /dev/gpio c $major 0