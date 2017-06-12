#!/usr/bin/env python
# -*- coding: utf-8 -*-
__author__ = 'chandler.pan'
from mLib.mDbg import *
import socket
import time

light_mode = 0


def GatewayDiscovery():
    sock_discovery = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock_discovery.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock_discovery.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock_discovery.settimeout(1)
    while True:
        try:
            print "scanning gateway"
            sock_discovery.sendto('{"command":' + str(0xFFFF) + '}', ('255.255.255.255', 6789))
            (buf_gateway, address_gateway) = sock_discovery.recvfrom(2048)
            if len(buf_gateway):
                print "Revived from %s:%s" % (address_gateway, buf_gateway)
                buf_dic = eval(buf_gateway)
                port_gateway = buf_dic['description']['port']
                return address_gateway[0], port_gateway
        except Exception, e:
            mLog(W, e)
def SocketInit():
    global sock_zigbee
    addr, port = GatewayDiscovery()
    while True:
        try:
            print "Connect with Gateway"
            sock_zigbee = socket.socket()
            sock_zigbee.connect((addr, port))
        except Exception, e:
            mLog(W, e)
        else:
            mLog(D, "Connect With Server Successful")
            break
        time.sleep(2)
def SocketReceive():
    try:
        return sock_zigbee.recv(1024)
    except Exception, e:
        mLog(E, e)
def SocketSend(msg):
    try:
        sock_zigbee.send(msg)
    except Exception, e:
        mLog(E, e)


def GetDeviceLists():
    command_get_device_lists = '{"command":	' + str(0x0011) + ',"sequence":0}'
    SocketSend(command_get_device_lists)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['status'] == 0:
        global device_lists
        device_lists = msg['description']
        # for device in device_lists:
            # print('name:%-30s online:%d id:0X%04X mac:0X%016X' %
            # (device['device_name'], device['device_online'], device['device_id'], device['device_mac_address']))
    else:
        mLog(E, "Error Communication with server")
        print msg
def SetDeviceOnOff(address, mode):
    command = '{"command":' + str(0x0020) + ', "sequence":0, "device_address":' + address + ',"group_id":0, "mode":' + str(mode) + '}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "SetDeviceOnOff Success")
    else:
        mLog(E, "Error Communication with server")
        print msg
def GetDeviceLightLevel(address):
        command = '{"command":' + str(0x0024) + ',"sequence":0,"device_address":' + address + '}'
        SocketSend(command)
        msg = eval(SocketReceive())
        if msg['status'] == 0:
            mLog(N, "GetDeviceLightLevel Success")
            print msg
        else:
            mLog(E, "Error Communication with server")
            print msg
def SetDeviceLightLevel(address):
    while True:
        level = raw_input("Input your level value(q exit):")
        if level == 'q':
            break
        try:
            if int(level) > 255:
                print 'the value is invalid, input again'
                continue
            command = '{"command":' + str(0x0021) + ',"sequence":0,"device_address":' + address + ',' \
                      '"group_id":0,"light_level":'+str(level)+'}'
            print command
            SocketSend(command)
            msg = eval(SocketReceive())
            if msg['status'] == 0:
                mLog(N, "GetDeviceLightRGBValue Success")
                print msg
            else:
                mLog(E, "Error Communication with server")
                print msg
        except Exception,e:
            mLog(E,e)
def GetDeviceLightState(address):
    command = '{"command":' + str(0x0023) + ',"sequence":0,"device_address":' + address + '}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "GetDeviceLightState Success")
        print msg
    else:
        mLog(E, "Error Communication with server")
        print msg
def SetDeviceOpenCloseStop(address, mode):
    command = '{"command": ' + str(0x0040) + '	,"sequence":0,"device_address":' + address + ',"operator":' + str(mode) + '}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "SetDeviceOnOff Success")
    else:
        mLog(E, "Error Communication with server")
        print msg
def GetDeviceRgbValue(address):
    command = '{"command":' + str(0x0025) + ',"sequence":0,"device_address":' + address + '}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "GetDeviceLightState Success")
        print msg
    else:
        mLog(E, "Error Communication with server")
        print msg
def SetDeviceRgbValue(address):
    while True:
        rgb = raw_input("Input your RGB value(q exit):")
        if rgb == 'q':
            break
        try:
            rgb = rgb.split(',')
            command = '{"command":' + str(0x0022) + ',"sequence":0,"device_address":' + address + \
                      ',"group_id":0,"rgb_value":{"R":'+rgb[0]+',"G":'+rgb[1]+',"B":'+rgb[2]+'}}'
            print command
            SocketSend(command)
            msg = eval(SocketReceive())
            if msg['status'] == 0:
                mLog(N, "GetDeviceLightRGBValue Success")
                print msg
            else:
                mLog(E, "Error Communication with server")
                print msg
        except Exception,e:
            mLog(E,e)
def GetDeviceSensorValue(address, sensor):
    command = '{"command":' + str(0x0030) + ',"sequence":0,"device_address":' + address + ',"sensor_type":' + str(sensor) + '}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "SetDeviceOnOff Success")
        print msg
    else:
        mLog(E, "Error Communication with server")
        print msg
def HandlePermitjoin():
    command = '{"command":' + str(0x0001) + ',"sequence":0,"time":60}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "HandlePermitjoin Success")
        print msg
    else:
        mLog(E, "Error Communication with server")
        print msg
def RemoveDeviceNetwork(address):
    command = '{"command":' + str(0x0002) + ',"sequence":0,"device_address":' + address + ',"rejoin":0,"remove_children":1}'                                                                                        '"remove_children":1}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "GetDeviceLightState Success")
        print msg
    else:
        mLog(E, "Error Communication with server")
        print msg
def GetNetworkChannel():
    command = '{"command":' + str(0x0003) + ',"sequence":0}'
    SocketSend(command)
    msg = eval(SocketReceive())
    if msg['status'] == 0:
        mLog(N, "GetDevice Channel Success")
        print msg
    else:
        mLog(E, "Error Communication with server")
        print msg

def HandleDimmerLight(device_select):
    while True:
        print '''
            Choose your operator:
            1. set device on
            2. set device off
            3. get device light state
            4. set device light level
            5. get device light level
            6. remove device from network
            other key will go back
        '''
        operator = raw_input("input your command:")
        if operator == '1':
            mLog(D, 'on device')
            SetDeviceOnOff(str(device_select['device_mac_address']), 1)
        elif operator == '2':
            mLog(D, 'off device')
            SetDeviceOnOff(str(device_select['device_mac_address']), 0)
        elif operator == '3':
            mLog(D, 'get device light state')
            GetDeviceLightState(str(device_select['device_mac_address']))
        elif operator == '4':
            mLog(D, 'set device light level')
            SetDeviceLightLevel(str(device_select['device_mac_address']))
        elif operator == '5':
            mLog(D, 'set device light level')
            GetDeviceLightLevel(str(device_select['device_mac_address']))
        elif operator == '6':
            print "remove device from network"
            RemoveDeviceNetwork(str(device_select['device_mac_address']))
        else:
            mLog(W, 'back forward')
            break
def HandleDimmerColorLight(device_select):
    while True:
        print '''
            Choose your operator:
            1. set device on
            2. set device off
            3. get device light state
            4. set device light level
            5. get device light level
            6. set device color
            7. get device color value
            8. remove device from network
            other key will go back
        '''
        operator = raw_input("input your command:")
        if operator == '1':
            mLog(D, 'on device')
            SetDeviceOnOff(str(device_select['device_mac_address']), 1)
        elif operator == '2':
            mLog(D, 'off device')
            SetDeviceOnOff(str(device_select['device_mac_address']), 0)
        elif operator == '3':
            mLog(D, 'get device light state')
            GetDeviceLightState(str(device_select['device_mac_address']))
        elif operator == '4':
            mLog(D, 'set device light level')
            SetDeviceLightLevel(str(device_select['device_mac_address']))
        elif operator == '5':
            mLog(D, 'set device light level')
            GetDeviceLightLevel(str(device_select['device_mac_address']))
        elif operator == '6':
            mLog(D, 'set device color')
            SetDeviceRgbValue(str(device_select['device_mac_address']))
        elif operator == '7':
            mLog(D, 'get device color')
            GetDeviceRgbValue(str(device_select['device_mac_address']))
        elif operator == '8':
            print "remove device from network"
            RemoveDeviceNetwork(str(device_select['device_mac_address']))
        else:
            mLog(W, 'back forward')
            break
def HandleClosuresDevice(device_select):
    while True:
        print '''
            Choose your operator:
            1. set device open
            2. set device close
            3. set device stop
            4. get device current location
            5. remove device from network
            other key will go back
        '''
        command = raw_input("input your command:")
        if command == '1':
            SetDeviceOpenCloseStop(str(device_select['device_mac_address']), 0)
        elif command == '2':
            SetDeviceOpenCloseStop(str(device_select['device_mac_address']), 1)
        elif command == '3':
            SetDeviceOpenCloseStop(str(device_select['device_mac_address']), 2)
        elif command == '4':
            print "this func is no finished"
        elif command == '5':
            RemoveDeviceNetwork(str(device_select['device_mac_address']))
        else:
            mLog(W, 'back forward')
            break
def HandleLightSensor(device_select):
    while True:
        print '''
            Choose your operator:
            1. get device's illuminance
            2. remove device from network
            other key will go back
        '''
        command = raw_input("input your command:")
        if command == '1':
            GetDeviceSensorValue(str(device_select['device_mac_address']), 0x0400)
        elif command == '2':
            RemoveDeviceNetwork(str(device_select['device_mac_address']))
        else:
            mLog(W, 'back forward')
            break

def main():
    mLog(D, "zigbee client socket test program")
    SocketInit()
    running = True
    while running:
        GetDeviceLists()
        print'''
        Choose your operator:
        1. open network
        2. select device
        3. get channel
        q. exit
        '''
        command = raw_input("input your command:")
        if command == '1':
            mLog(D, 'open network 60 second')
            HandlePermitjoin()
        elif command == '2':
            mLog(D, 'select a device')
            for i in range(len(device_lists)):
                print i, device_lists[i]
            try:
                device_num = int(raw_input("input device's number:"))
                device_select = device_lists[device_num]
                if device_select['device_id'] == 0x0840:  # E_ZBD_COORDINATOR
                    print "select coordinator"
                elif device_select['device_id'] == 0x0101:  # E_ZBD_DIMMER_LIGHT
                    print "E_ZBD_DIMMER_LIGHT"
                    HandleDimmerLight(device_select)
                elif device_select['device_id'] == 0x0106:  # E_ZBD_LIGHT_SENSOR
                    print "E_ZBD_LIGHT_SENSOR"
                    HandleLightSensor(device_select)
                elif device_select['device_id'] == 0x0102:  # E_ZBD_COLOUR_DIMMER_LIGHT
                    print "E_ZBD_COLOUR_DIMMER_LIGHT"
                    HandleDimmerColorLight(device_select)
                elif device_select['device_id'] == 0x0202:  # E_ZBD_WINDOW_COVERING_DEVICE
                    print "E_ZBD_WINDOW_COVERING_DEVICE"
                    HandleClosuresDevice(device_select)
            except Exception, e:
                mLog(E, e)
        elif command == '3':
            GetNetworkChannel()
        elif command == 'q':
            mLog(W, 'exit program')
            running = False
        else:
            mLog(E, "Invalid param")


if __name__ == '__main__':
    main()
