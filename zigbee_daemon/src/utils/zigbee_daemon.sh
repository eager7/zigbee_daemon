#! /bin/sh
###########################################################################
#
# MODULE:             IOTC-zigbee - init shell script
#
# REVISION:           $Revision: 1.0 $
#
# DATED:              $Date: 2016-12-23 11:16:28 +0000 $
#
# AUTHOR:             PCT
#
###########################################################################
#
# Copyright panchangtao@gmail.com 2016. All rights reserved
#
###########################################################################


PATH=/sbin:/usr/sbin:/bin:/usr/bin

. /lib/init/vars.sh
. /lib/lsb/init-functions

do_start() {
    ln -s lock /var/run/zigbee_daemon.lock || exit 0
	if [ -x /usr/local/bin/zigbee-daemon ]; then
		sudo /usr/local/bin/zigbee-daemon -v 7 -B 1000000 -s /dev/ttyUSB0 -D /etc/zigbee_daemon/ZigbeeDaemon.DB
		ES=$?
		[ "$VERBOSE" != no ] && log_end_msg $ES
		return $ES
	fi
}

case "$1" in
    start)
	do_start
        ;;
    restart|reload|force-reload)
        ps -aux | grep zigbee-daemon | grep D | awk '{print $2}'  | sudo xargs kill -9
		rm -rf /var/run/zigbee_daemon.lock
		do_start
        ;;
    stop)
		ps -aux | grep zigbee-daemon | grep D | awk '{print $2}'  | sudo xargs kill -9
		rm -rf /var/run/zigbee_daemon.lock
        ;;
    status)
		ps -aux | grep zigbee-daemon | grep D
        ;;
    *)
        echo "Usage: $0 start|stop" >&2
        exit 3
        ;;
esac
