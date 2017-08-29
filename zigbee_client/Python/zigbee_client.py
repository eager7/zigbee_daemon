#!/usr/bin/env python
# -*- coding: utf-8 -*-
__author__ = 'chandler.pan'
from mLib.mDbg import *
import socket
import time

light_mode = 0
type = '{"type":'

E_SS_COMMAND_STATUS = 0x8000
E_SS_COMMAND_GET_MAC = 0x0010
E_SS_COMMAND_GET_MAC_RESPONSE = 0x8010
E_SS_COMMAND_GET_VERSION = 0x0011
E_SS_COMMAND_VERSION_LIST = 0x8011
E_SS_COMMAND_OPEN_NETWORK = 0x0012
E_SS_COMMAND_GET_CHANNEL = 0x0013
E_SS_COMMAND_GET_CHANNEL_RESPONSE = 0x8013
E_SS_COMMAND_GET_DEVICES_LIST_ALL = 0x0014
E_SS_COMMAND_GET_DEVICES_RESPONSE = 0x8014
E_SS_COMMAND_LEAVE_NETWORK = 0x0015
E_SS_COMMAND_SEARCH_DEVICE = 0x0016
E_SS_COMMAND_COORDINATOR_UPGRADE = 0x0017

E_SS_COMMAND_DOOR_LOCK_ADD_PASSWORD = 0x00F0
E_SS_COMMAND_DOOR_LOCK_DEL_PASSWORD = 0x00F1
E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD = 0x00F2
E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD_RESPONSE = 0x80F2
E_SS_COMMAND_DOOR_LOCK_GET_RECORD = 0x00F3
E_SS_COMMAND_DOOR_LOCK_GET_RECORD_RESPONSE = 0x80F3
E_SS_COMMAND_DOOR_LOCK_ALARM_REPORT = 0x00F4
E_SS_COMMAND_DOOR_LOCK_OPEN_REPORT = 0x00F5
E_SS_COMMAND_DOOR_LOCK_ADD_USER_REPORT = 0x00F6
E_SS_COMMAND_DOOR_LOCK_DEL_USER_REPORT = 0x00F7
E_SS_COMMAND_DOOR_LOCK_GET_USER = 0x00F8
E_SS_COMMAND_DOOR_LOCK_GET_USER_RESPONSE = 0x80F8
E_SS_COMMAND_SET_DOOR_LOCK_STATE = 0x00F9
E_SS_COMMAND_DISCOVERY = 0x00FF

def GatewayDiscovery():
    sock_discovery = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock_discovery.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock_discovery.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock_discovery.settimeout(1)
    while True:
        try:
            print "scanning gateway"
            sock_discovery.sendto('{"type":' + str(E_SS_COMMAND_DISCOVERY) + '}', ('255.255.255.255', 6789))
            (buf_gateway, address_gateway) = sock_discovery.recvfrom(2048)
            if len(buf_gateway):
                print "Revived from %s:%s" % (address_gateway, buf_gateway)
                buf_dic = eval(buf_gateway)
                port_gateway = buf_dic['port']
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

def SettingWifiNetwork(state):
    global sock_wifi
    #addr, port = GatewayDiscovery()
    
    try:
        print "SettingWifiNetwork"
	sock_wifi = socket.socket()
	sock_wifi.connect(("10.128.0.101", 7787))
    except Exception, e:
	mLog(W, e)
    else:
	mLog(D, "Connect With Server Successful")
	if state == 0:
		sock_wifi.send("{\"type\":17,\"ssid\":\"Tenda_25B200\",\"key\":\"12345678\"}")
	elif state == 1:
		sock_wifi.send("{\"type\":32785}")
		
def GetHostVersion():
    command = type + str(E_SS_COMMAND_GET_VERSION) + ',"sequence":0}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['type'] == E_SS_COMMAND_VERSION_LIST:
        version = msg['version']
        print "Host Version:"+version
    else:
        mLog(E, "Error Communication with server:"+msg['information'])
        print msg
def GetHostMac():
    command = type + str(E_SS_COMMAND_GET_MAC) + ',"sequence":0}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['type'] == E_SS_COMMAND_GET_MAC_RESPONSE:
        result = msg['mac']
        print "Host Mac:"+result
    else:
        mLog(E, "Error Communication with server:"+msg['information'])
        print msg
def GetZigbeeChannel():
    command = type + str(E_SS_COMMAND_GET_CHANNEL) + ',"sequence":0}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['type'] == E_SS_COMMAND_GET_CHANNEL_RESPONSE:
        result = msg['channel']
        print "Host Channel:"+str(result)
    else:
        mLog(E, "Error Communication with server:"+msg['information'])
        print msg
def SearchDevices():
    command = type + str(E_SS_COMMAND_SEARCH_DEVICE) + ',"sequence":0}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    print msg

def DoorLockAddPassword():
    id = raw_input("input password id:")
    available = raw_input("input password available:")
    time = "2017/01/01/12/00-2018/01/01/12/00"
    length = raw_input("input password length:")
    password = raw_input("input password data:")
    command = type+str(E_SS_COMMAND_DOOR_LOCK_ADD_PASSWORD)+',"sequence":0,"mac":0,"id":'+str(id)+\
              ',"available":'+str(available)+',"time":"'+time+'","length":'+str(length)+',"password":"'+password+'"}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    print msg
def DoorLockDelPassword():
    id = raw_input("input password id:")
    command = type+str(E_SS_COMMAND_DOOR_LOCK_DEL_PASSWORD)+',"sequence":0,"mac":0,"id":'+str(id)+'}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    print msg
def DoorLockGetPassword():
    command = type + str(E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD) + ',"sequence":0,"mac":0,"id":255}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['type'] == E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD_RESPONSE:
        result = msg['password']
        print msg
        print result
    else:
        mLog(E, "Error Communication with server:"+msg['information'])
        print msg
def DoorLockGetRecord():
    command = type + str(E_SS_COMMAND_DOOR_LOCK_GET_RECORD) + ',"sequence":0,"mac":0,"id":255,number:5}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['type'] == E_SS_COMMAND_DOOR_LOCK_GET_RECORD_RESPONSE:
        result = msg['records']
        print result
    else:
        mLog(E, "Error Communication with server:"+msg['information'])
        print msg
def DoorLockGetUser():
    command = type + str(E_SS_COMMAND_DOOR_LOCK_GET_USER) + ',"sequence":0,"mac":0,"id":255}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['type'] == E_SS_COMMAND_DOOR_LOCK_GET_USER_RESPONSE:
        result = msg['users']
        print result
    else:
        mLog(E, "Error Communication with server:"+msg['information'])
        print msg
def DoorLockSetStatus():
    status = raw_input("input command(0=lock,1=unlock):")
    command = type+str(E_SS_COMMAND_SET_DOOR_LOCK_STATE)+',"sequence":0,"mac":0,"command":'+str(status)+'}'
    print command
    SocketSend(command)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    print msg

def GetDeviceLists():
    command_get_device_lists = type + str(E_SS_COMMAND_GET_DEVICES_LIST_ALL) + ',"sequence":0}'
    print command_get_device_lists
    SocketSend(command_get_device_lists)
    msg = SocketReceive()
    msg = eval(msg)  # covert to dict
    if msg['type'] == E_SS_COMMAND_GET_DEVICES_RESPONSE:
        global device_lists
        device_lists = msg['devices']
        for device in device_lists:
            print('name:%-30s online:%d id:0X%04X mac:0X%016X' %
            (device['name'], device['online'], device['id'], device['mac']))
    else:
        mLog(E, "Error Communication with server:"+msg['information'])
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
def SetDoorLockOpenClose(address, mode):
    command = '{"command": ' + str(0x0041) + '	,"sequence":0,"device_address":' + address + ',"operator":' + str(mode) + '}'
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
    command = type + str(E_SS_COMMAND_OPEN_NETWORK) + ',"sequence":0,"time":60}'
    SocketSend(command)
    msg = eval(SocketReceive())
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
def HandleDoorLockDevice(device_select):
    while True:
        print '''
            Choose your operator:
            1. set device open
            2. set device close
            3. reserve
            4. get device current location
            5. remove device from network
            other key will go back
        '''
        command = raw_input("input your command:")
        if command == '1':
            SetDoorLockOpenClose(str(device_select['device_mac_address']), 0)
        elif command == '2':
            SetDoorLockOpenClose(str(device_select['device_mac_address']), 1)
        elif command == '3':
            SetDoorLockOpenClose(str(device_select['device_mac_address']), 2)
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

    running = True
    while running:
        print'''
        Choose your operator:
	0. Search Host
        1. Get Host Version
        2. Get Host Mac
        3. Open Network
        4. Get Channel Number
        5. Get Devices List
        6. Search Devices
        11.Add Temporary Password
        12.Del Temporary Password
        13.Get Temporary Password
        14.Get Door Records
        15.Get Users
        16.Lock UnLock Door
	80.Setting Wifi
	81.Reset Network
        q. exit
        '''
        command = raw_input("input your command:")
        if  command == '0':
            mLog(D, 'Search Host')
            SocketInit()
        elif command == '1':
            mLog(D, 'Get Host Version')
            GetHostVersion()
        elif command == '2':
            mLog(D, 'select a device')
            GetHostMac()
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
                elif device_select['device_id'] == 0x000A:  # E_ZBD_DOOR_LOCK
                    print "E_ZBD_DOOR_LOCK"
                    HandleDoorLockDevice(device_select)
            except Exception, e:
                mLog(E, e)
        elif command == '3':
            mLog(D, 'Open Network 60 Seconds')
            HandlePermitjoin()
        elif command == '4':
            mLog(D, "Get Channel Number")
            GetZigbeeChannel()
        elif command == '5':
            mLog(D, "Get Devices List")
            GetDeviceLists()
        elif command == '6':
            mLog(D, "Search Devices")
            SearchDevices()
        elif command == '11':
            mLog(D, "Add Temporary Password")
            DoorLockAddPassword()
        elif command == '12':
            mLog(D, "DoorLockDelPassword")
            DoorLockDelPassword()
        elif command == '13':
            mLog(D, "Get Temporary Password")
            DoorLockGetPassword()
        elif command == '14':
            mLog(D, "Get Door Records")
            DoorLockGetRecord()
        elif command == '15':
            mLog(D, "Get Users")
            DoorLockGetUser()
        elif command == '16':
            mLog(D, "Lock UnLock Door")
            DoorLockSetStatus()
        elif command == '80':
            mLog(D, "Setting Wifi")
            SettingWifiNetwork(0)
        elif command == '81':
            mLog(D, "Reset Network")
            SettingWifiNetwork(1)
        elif command == 'q':
            mLog(W, 'exit program')
            running = False
        else:
            mLog(E, "Invalid param")


if __name__ == '__main__':
    main()
