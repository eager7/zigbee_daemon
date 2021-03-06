/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          Serial interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2016-12-02 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2016. All rights reserved
 *
 ***************************************************************************/
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/signal.h>
#include <errno.h>
#include <string.h>
#include <zigbee_node.h>
#include <door_lock.h>
#include "door_lock_controller.h"
#include "zigbee_devices.h"
#include "zigbee_control_bridge.h"
#include "zigbee_socket.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_ZCB (verbosity >= 7)

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;
extern tsDeviceIDMap asDeviceIDMap[];
/** Coordinator */
uint64          u64PanID    = CONFIG_DEFAULT_PAN_ID;
teChannel       eChannel    = E_CHANNEL_DEFAULT;
teStartMode     eStartMode  = CONFIG_DEFAULT_START_MODE;

/** APS Ack enabled by default */
int bZCB_EnableAPSAck  = 1;

//uint16 au16ProfileZLL =         E_ZB_PROFILEID_ZLL;
static uint16 au16ProfileHA =   E_ZB_PROFILEID_HA;
static uint16 au16Cluster[] = {
                                E_ZB_CLUSTERID_ONOFF,                   /*Light*/
                                E_ZB_CLUSTERID_DOOR_LOCK,
                              };

tsZigbeeNodes sControlBridge;
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Local    Functions                                            ***/
/****************************************************************************/
/**
 * 配置协调器的参数
 * */
static teZbStatus eZCB_SetDeviceType(teModuleMode eModuleMode)
{
    uint8 u8ModuleMode = eModuleMode;
    DBG_vPrintln(DBG_ZCB, "Writing Module: Set Device Type: %d\n", eModuleMode);
    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_SET_DEVICE_TYPE, sizeof(uint8), &u8ModuleMode, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

static teZbStatus eZCB_SetChannelMask(uint32 u32ChannelMask)
{
    DBG_vPrintln(DBG_ZCB, "Setting channel mask: 0x%08X", u32ChannelMask);
    u32ChannelMask = htonl(u32ChannelMask);
    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_SET_CHANNEL_MASK, sizeof(uint32), &u32ChannelMask, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

static teZbStatus eZCB_SetExtendedPANID(uint64 u64PanID)
{
    u64PanID = htobe64(u64PanID);
    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_SET_EXT_PAN_ID, sizeof(uint64), &u64PanID, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

static teZbStatus eZCB_StartNetwork(void)
{
    DBG_vPrintln(DBG_ZCB, "Start network \n");
    CHECK_RESULT (eSL_SendMessage(E_SL_MSG_START_NETWORK, 0, NULL, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

static void eZCB_ConfigureControlBridge(void)
{
#define CONFIGURATION_INTERVAL 500000
    usleep(CONFIGURATION_INTERVAL);
    /* Set up configuration */
    switch (eStartMode)
    {
        case(E_START_COORDINATOR):
            DBG_vPrintln(DBG_ZCB, "Starting control bridge as HA coordinator");
            eZCB_SetDeviceType(E_MODE_COORDINATOR);usleep(CONFIGURATION_INTERVAL);
            eZCB_SetChannelMask(eChannel);      usleep(CONFIGURATION_INTERVAL);
            eZCB_SetExtendedPANID(u64PanID);    usleep(CONFIGURATION_INTERVAL);
            eZCB_StartNetwork();                usleep(CONFIGURATION_INTERVAL);
            break;

        case (E_START_ROUTER):
            DBG_vPrintln(DBG_ZCB, "Starting control bridge as HA compatible router");
            eZCB_SetDeviceType(E_MODE_HA_COMPATABILITY);usleep(CONFIGURATION_INTERVAL);
            eZCB_SetChannelMask(eChannel);      usleep(CONFIGURATION_INTERVAL);
            eZCB_SetExtendedPANID(u64PanID);    usleep(CONFIGURATION_INTERVAL);
            eZCB_StartNetwork();                usleep(CONFIGURATION_INTERVAL);
            break;

        case (E_START_TOUCHLINK):
            DBG_vPrintln(DBG_ZCB, "Starting control bridge as ZLL router");
            eZCB_SetDeviceType(E_MODE_ROUTER);  usleep(CONFIGURATION_INTERVAL);
            eZCB_SetChannelMask(eChannel);      usleep(CONFIGURATION_INTERVAL);
            eZCB_SetExtendedPANID(u64PanID);    usleep(CONFIGURATION_INTERVAL);
            eZCB_StartNetwork();                usleep(CONFIGURATION_INTERVAL);
            break;

        default:
            ERR_vPrintln(T_TRUE,  "Unknown module mode\n");
    }
}

/**
 * 初始化一个设备节点，根据设备ID对设备进行不同的初始化，初始化函数列表在数组asDeviceIDMap中
 * */
static void vZCB_InitZigbeeNodeInfo(tsZigbeeNodes *psZigbeeNode, uint16 u16DeviceID)
{
    DBG_vPrintln(DBG_ZCB, "************vZCB_InitZigbeeNodeInfo\n");
    tsDeviceIDMap *psDeviceIDMap = asDeviceIDMap;
    eLockLock(&psZigbeeNode->mutex);
    psZigbeeNode->sNode.u16DeviceID = u16DeviceID; //update device id
    DBG_vPrintln(DBG_ZCB, "Init Device 0x%04x\n",psZigbeeNode->sNode.u16DeviceID);
    while (((psDeviceIDMap->u16ZigbeeDeviceID != 0) && (psDeviceIDMap->prInitializeRoutine != NULL)))
    {
        if (psDeviceIDMap->u16ZigbeeDeviceID == psZigbeeNode->sNode.u16DeviceID) {
            DBG_vPrintln(DBG_ZCB, "Found Zigbee device type for Zigbee Device type 0x%04X\n", psDeviceIDMap->u16ZigbeeDeviceID);
            psDeviceIDMap->prInitializeRoutine(psZigbeeNode);
        }
        psDeviceIDMap++;
    }
    eLockunLock(&psZigbeeNode->mutex);
}

/**
 * 将在路由表中发现的设备和设备发送Device Announce发现的设备加入到设备链表中，在此函数中我们
 * 仅知道设备的网络地址和MAC地址，并不知道设备的DeviceID，它可以提供哪些服务，这些服务又位于
 * 哪个端点等，因此需要发送匹配描述符来查询这些信息，终端节点因为休眠的原因无法获取这些信息，所以
 * 会直接初始化，不会去和此设备进行交互，只能等此设备上报数据。
 * */
static void vZCB_AddNodeIntoNetwork(uint16 u16ShortAddress, uint64 u64IEEEAddress, uint8 u8MacCapability)
{
    tsZigbeeNodes *psZigbeeNodeTemp = NULL;
    psZigbeeNodeTemp = psZigbeeFindNodeByShortAddress(u16ShortAddress);
    
    if ((NULL != psZigbeeNodeTemp)&&(0 != psZigbeeNodeTemp->sNode.u16DeviceID) ){//New Nodes
        DBG_vPrintln(DBG_ZCB, "The Node:0x%04x already in the network\n", psZigbeeNodeTemp->sNode.u16ShortAddress);
        return;
    }

    eZigbeeAddNode(u16ShortAddress, u64IEEEAddress, 0x0000, u8MacCapability, &psZigbeeNodeTemp);
    if(u8MacCapability & E_ZB_MAC_CAPABILITY_FFD){ //router, we need get its' device id
        if(0 == psZigbeeNodeTemp->sNode.u16DeviceID){ //unfinished node
            DBG_vPrintln(DBG_ZCB, "eZCB_MatchDescriptorRequest\n");
            usleep(1000);
            if(eZCB_MatchDescriptorRequest(u16ShortAddress, au16ProfileHA,sizeof(au16Cluster) / sizeof(uint16), au16Cluster, 0, NULL, NULL) != E_ZB_OK)
            {
                ERR_vPrintln(DBG_ZCB, "Error sending match descriptor request\n");
            }
        }
    } else { //enddevice, no need device id
        vZCB_InitZigbeeNodeInfo(psZigbeeNodeTemp, E_ZBD_END_DEVICE_DEVICE);
    }
}
/**
 * 处理设备节点的cluster列表，列表中的cluster就是设备支持的cluster，在控制
 * 网络中的节点时需要检查设备节点支不支持此cluster，如果不支持需要在程序中添加。
 * */
static void vZCB_HandleNodeClusterList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8003]ZCB_HandleNodeClusterList\n");
    int iPosition;
    int iCluster = 0;
    struct _tsClusterList {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    au16ClusterList[255];
    } PACKED *psClusterList = (struct _tsClusterList *)pvMessage;
    
    psClusterList->u16ProfileID = ntohs(psClusterList->u16ProfileID);
    DBG_vPrintln(DBG_ZCB, "Cluster list for endpoint %d, profile ID 0x%4X\n",
                psClusterList->u8Endpoint, 
                psClusterList->u16ProfileID);
    
    eLockLock(&sControlBridge.mutex); //lock Coordinator Node
    if (eZigbeeNodeAddEndpoint(&sControlBridge.sNode, psClusterList->u8Endpoint, psClusterList->u16ProfileID, NULL) != E_ZB_OK) {
        goto done;
    }
    iPosition = sizeof(uint8) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbeeNodeAddCluster(&sControlBridge.sNode, psClusterList->u8Endpoint,
                                  ntohs(psClusterList->au16ClusterList[iCluster])) != E_ZB_OK) {
            goto done;
        }
        iPosition += sizeof(uint16);
        iCluster++;
    }
done:
    eLockunLock(&sControlBridge.mutex);
}
/**
 * 处理设备的attribute列表，列表中的attribute是设备支持的，并且挂靠在具体的cluster下，
 * 在读取或者写入attribute时需要检查节点是否支持，不支持将无法操作
 * */
static void vZCB_HandleNodeClusterAttributeList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8004]ZCB_HandleNodeClusterAttributeList\n");
    int iPosition;
    int iAttribute = 0;
    struct _tsClusterAttributeList {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16ClusterID;
        uint16    au16AttributeList[255];
    } PACKED *psClusterAttributeList = (struct _tsClusterAttributeList *)pvMessage;
    
    psClusterAttributeList->u16ProfileID = ntohs(psClusterAttributeList->u16ProfileID);
    psClusterAttributeList->u16ClusterID = ntohs(psClusterAttributeList->u16ClusterID);
    
    DBG_vPrintln(DBG_ZCB, "Cluster attribute list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n",
                psClusterAttributeList->u8Endpoint, 
                psClusterAttributeList->u16ClusterID,
                psClusterAttributeList->u16ProfileID);
    
    eLockLock(&sControlBridge.mutex);
    iPosition = sizeof(uint8) + sizeof(uint16) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbeeNodeAddAttribute(&sControlBridge.sNode, psClusterAttributeList->u8Endpoint,
                                    psClusterAttributeList->u16ClusterID,
                                    ntohs(psClusterAttributeList->au16AttributeList[iAttribute])) != E_ZB_OK) {
            goto done;
        }
        iPosition += sizeof(uint16);
        iAttribute++;
    }
    
done:
    eLockunLock(&sControlBridge.mutex);
}
/**
 * 处理协调器的命令列表，将命令添加到链表中，在使用前需要先查询
 * */
static void vZCB_HandleNodeCommandIDList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8005]ZCB_HandleNodeCommandIDList\n");
    int iPosition;
    int iCommand = 0;
    struct _tsCommandIDList {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16ClusterID;
        uint8     au8CommandList[255];
    } PACKED *psCommandIDList = (struct _tsCommandIDList *)pvMessage;
    
    psCommandIDList->u16ProfileID = ntohs(psCommandIDList->u16ProfileID);
    psCommandIDList->u16ClusterID = ntohs(psCommandIDList->u16ClusterID);
    
    DBG_vPrintln(DBG_ZCB, "Command ID list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n",
                psCommandIDList->u8Endpoint, 
                psCommandIDList->u16ClusterID,
                psCommandIDList->u16ProfileID);
    
    eLockLock(&sControlBridge.mutex);
    
    iPosition = sizeof(uint8) + sizeof(uint16) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbeeNodeAddCommand(&sControlBridge.sNode,
                                  psCommandIDList->u8Endpoint,
                                  psCommandIDList->u16ClusterID,
                                  psCommandIDList->au8CommandList[iCommand]) != E_ZB_OK) {
            goto done;
        }
        iPosition += sizeof(uint8);
        iCommand++;
    }    

done:
    eLockunLock(&sControlBridge.mutex);
}

static void vZCB_HandleRestartProvisioned(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8006]ZCB_HandleRestartProvisioned\n");
    const char *pcStatus = NULL;
    
    struct _tsWarmRestart
    {
        uint8     u8Status;
    } PACKED *psWarmRestart = (struct _tsWarmRestart *)pvMessage;

    switch (psWarmRestart->u8Status)
    {
        #define STATUS(a, b) case(a): pcStatus = b; break
        STATUS(0, "STARTUP");
        STATUS(1, "WAIT_START");
        STATUS(2, "NFN_START");
        STATUS(3, "DISCOVERY");
        STATUS(4, "NETWORK_INIT");
        STATUS(5, "RESCAN");
        STATUS(6, "RUNNING");
        #undef STATUS
        default: pcStatus = "Unknown";
    }
    WAR_vPrintln(T_TRUE,  "Control bridge restarted, status %d (%s)\n", psWarmRestart->u8Status, pcStatus);
    return;
}
/**
 * 在协调器重启后设置协调器的参数，如信道，MAC地址，设备类型等，最后启动网络
 * */
static void vZCB_HandleRestartFactoryNew(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8007]ZCB_HandleRestartFactoryNew\n");
    const char *pcStatus = NULL;
    
    struct _tsWarmRestart {
        uint8     u8Status;
    } PACKED *psWarmRestart = (struct _tsWarmRestart *)pvMessage;

    switch (psWarmRestart->u8Status)
    {
        #define STATUS(a, b) case(a): pcStatus = b; break
        STATUS(0, "STARTUP");
        STATUS(1, "WAIT_START");
        STATUS(2, "NFN_START");
        STATUS(3, "DISCOVERY");
        STATUS(4, "NETWORK_INIT");
        STATUS(5, "RESCAN");
        STATUS(6, "RUNNING");
        #undef STATUS
        default: pcStatus = "Unknown";
    }
    WAR_vPrintln(T_TRUE,  "Control bridge factory new restart, status %d (%s)", psWarmRestart->u8Status, pcStatus);
    
    eZCB_ConfigureControlBridge();
    return;
}
/**
 * 处理协调器入网
 * */
static void vZCB_HandleNetworkJoined(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8024]ZCB_HandleNetworkJoined\n");
    struct _tsNetworkJoinedFormed {
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint64    u64IEEEAddress;
        uint8     u8Channel;
    } PACKED *psMessage = (struct _tsNetworkJoinedFormed *)pvMessage;

    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);

    DBG_vPrintln(DBG_ZCB, "Network %s on channel %d. Control bridge address 0x%04X (0x%016llX)\n",
                        psMessage->u8Status == 0 ? "joined" : "formed",
                        psMessage->u8Channel, psMessage->u16ShortAddress,
                        (unsigned long long int)psMessage->u64IEEEAddress);

    /** Control bridge joined the network - initialise its data in the network structure */
    eLockLock(&sControlBridge.mutex);

    sControlBridge.sNode.u16DeviceID     = E_ZBD_COORDINATOR;
    sControlBridge.sNode.u16ShortAddress = psMessage->u16ShortAddress;
    sControlBridge.sNode.u64IEEEAddress  = psMessage->u64IEEEAddress;
    sControlBridge.sNode.u8MacCapability = E_ZB_MAC_CAPABILITY_ALT_PAN_COORD|E_ZB_MAC_CAPABILITY_FFD|E_ZB_MAC_CAPABILITY_POWERED|E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE;

    DBG_vPrintln(DBG_ZCB, "Node Joined 0x%04X (0x%016llX)\n",
                        sControlBridge.sNode.u16ShortAddress, 
                        (unsigned long long int)sControlBridge.sNode.u64IEEEAddress);

    vZigbeePrintNode(&sControlBridge.sNode);
    asDeviceIDMap[0].prInitializeRoutine(&sControlBridge);
    eLockunLock(&sControlBridge.mutex);
}

/**
 * 路由器或者终端设备在加入网络时都会发送一个DeviceAnnounce的事件出来，里面包含了网络地址，
 * MAC地址以及设备类型，我们根据此信息初始化设备，需要注意的是，终端设备在每次重启时都会发送这个事件，
 * 但是路由器只会在入网时发送。
 * */
static void vZCB_HandleDeviceAnnounce(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x004D]vZCB_HandleDeviceAnnounce\n");
    struct _tsDeviceAnnounce {
        uint16    u16ShortAddress;
        uint64    u64IEEEAddress;
        uint8     u8MacCapability;
    } PACKED *psMessage = (struct _tsDeviceAnnounce *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    
    DBG_vPrintln(DBG_ZCB, "Device Joined, Address 0x%04X (0x%016llX). Mac Capability Mask 0x%02X\n",
                psMessage->u16ShortAddress,(unsigned long long int)psMessage->u64IEEEAddress,psMessage->u8MacCapability);

    tsZigbeeNodes *psZigbeeNodeTemp = NULL;
    psZigbeeNodeTemp = psZigbeeFindNodeByShortAddress(psMessage->u16ShortAddress);

    if ((NULL != psZigbeeNodeTemp)&&(0 != psZigbeeNodeTemp->sNode.u16DeviceID) ){//New Nodes
        DBG_vPrintln(DBG_ZCB, "The Node:0x%04x already in the network\n", psZigbeeNodeTemp->sNode.u16ShortAddress);
        eZigbeeSqliteUpdateDeviceOnline(psMessage->u64IEEEAddress, 1);
        return;
    }
    eZigbeeAddNode(psMessage->u16ShortAddress,
                   psMessage->u64IEEEAddress,
                   0x0000,
                   psMessage->u8MacCapability,
                   &psZigbeeNodeTemp);
    snprintf(psZigbeeNodeTemp->sNode.auDeviceName,
             sizeof(psZigbeeNodeTemp->sNode.auDeviceName), "%s-%04X", "Device", psZigbeeNodeTemp->sNode.u16ShortAddress);
    eZigbeeSqliteAddNewDevice(psZigbeeNodeTemp->sNode.u64IEEEAddress, psZigbeeNodeTemp->sNode.u16ShortAddress,
                              0x0000, psZigbeeNodeTemp->sNode.auDeviceName, psZigbeeNodeTemp->sNode.u8MacCapability, NULL);
    //eZCB_DeviceRecognition(psZigbeeNodeTemp->sNode.u64IEEEAddress);
    return;
}

/** 
 * 此事件是发送匹配描述符的响应处理函数，可以通过匹配描述符获取到支持cluster的端点号，然后通过端点号获取更多信息。
 * 当系统断电重启后，路由设备并不会主动发送匹配描述符响应，因此需要我们在主线程中定时去获取协调器的路由表，并根据
 * 里面的路由器节点去发送匹配描述符来查询端点，使设备重新进入网络链表。
 * */
static void vZCB_HandleMatchDescriptorResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8046]ZCB_HandleMatchDescriptorResponse\n");
    struct _tMatchDescriptorResponse {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint8     u8NumEndpoints;
        uint8     au8Endpoints[255];
    } PACKED *psMatchDescriptorResponse = (struct _tMatchDescriptorResponse *)pvMessage;

    psMatchDescriptorResponse->u16ShortAddress  = ntohs(psMatchDescriptorResponse->u16ShortAddress);
    if (psMatchDescriptorResponse->u8NumEndpoints) /* if endpoint's number is 0, this is a invaild device */{
        tsZigbeeNodes *psZigbeeNode = psZigbeeFindNodeByShortAddress(psMatchDescriptorResponse->u16ShortAddress);
        if((NULL == psZigbeeNode) || (psZigbeeNode->sNode.u16DeviceID != 0)){
            ERR_vPrintln(T_TRUE, "Can't find this node in the network!\n");
            return ;
        }
        eLockLock(&psZigbeeNode->mutex);
        int i = 0;
        for (i = 0; i < psMatchDescriptorResponse->u8NumEndpoints; i++) {
            /* Add an endpoint to the device for each response in the match descriptor response */
            eZigbeeNodeAddEndpoint(&psZigbeeNode->sNode, psMatchDescriptorResponse->au8Endpoints[i], 0, NULL);
        }
        eLockunLock(&psZigbeeNode->mutex);

        for (i = 0; i < psZigbeeNode->sNode.u32NumEndpoints; i++)/* get profile id, device id, input clusters */{
            if (psZigbeeNode->sNode.pasEndpoints[i].u16ProfileID == 0) {
                usleep(500);
                if (eZCB_SimpleDescriptorRequest(&psZigbeeNode->sNode, psZigbeeNode->sNode.pasEndpoints[i].u8Endpoint) != E_ZB_OK){
                    ERR_vPrintln(T_TRUE, "Failed to read endpoint simple descriptor - requeue\n");
                    //eZigbeeRemoveNode(psZigbeeNode);
                    return ;
                }
            }
        }
    }
}
/**
 * 简单描述符的响应处理函数，我们可以通过简单描述符获取到设备ID，从而知道是一个什么类型的设备，然后根据设备ID
 * 初始化设备节点。
 * */
static void vZCB_HandleSimpleDescriptorResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8043]vZCB_HandleSimpleDescriptorResponse\n");
    struct _tSimpleDescriptorResponse {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint8     u8Length;
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16DeviceID;
        struct {
          uint8   u8DeviceVersion :4;
          uint8   u8Reserved :4;
        }PACKED sBitField;
        tsZDClusterList sInputClusters;
    } PACKED *psSimpleDescriptorResponse = (struct _tSimpleDescriptorResponse*)pvMessage;
    
    psSimpleDescriptorResponse->u8Length        = (uint8)ntohs(psSimpleDescriptorResponse->u8Length);
    psSimpleDescriptorResponse->u16DeviceID     = ntohs(psSimpleDescriptorResponse->u16DeviceID);
    psSimpleDescriptorResponse->u16ProfileID    = ntohs(psSimpleDescriptorResponse->u16ProfileID);
    psSimpleDescriptorResponse->u16ShortAddress = ntohs(psSimpleDescriptorResponse->u16ShortAddress);
    DBG_vPrintln(DBG_ZCB, "Get Simple Desciptor response for Endpoint %d to 0x%04X\n",
        psSimpleDescriptorResponse->u8Endpoint, psSimpleDescriptorResponse->u16ShortAddress);

    tsZigbeeNodes *psZigbeeNode = psZigbeeFindNodeByShortAddress(psSimpleDescriptorResponse->u16ShortAddress);
    if((NULL == psZigbeeNode) || (psZigbeeNode->sNode.u16DeviceID != 0)){
        ERR_vPrintln(T_TRUE, "Can't find this node in the network!\n");
        return ;
    }
    eLockLock(&psZigbeeNode->mutex);
    if (eZigbeeNodeAddEndpoint(&psZigbeeNode->sNode,
                               psSimpleDescriptorResponse->u8Endpoint, ntohs(psSimpleDescriptorResponse->u16ProfileID),
                               NULL) != E_ZB_OK) {
        ERR_vPrintln(T_TRUE, "eZigbeeNodeAddEndpoint error\n");
        //eZigbeeRemoveNode(psZigbeeNode);
        return ;
    }

    int i = 0;
    for (i = 0; i < psSimpleDescriptorResponse->sInputClusters.u8ClusterCount; i++){
        uint16 u16ClusterID = ntohs(psSimpleDescriptorResponse->sInputClusters.au16Clusters[i]);
        if (eZigbeeNodeAddCluster(&psZigbeeNode->sNode, psSimpleDescriptorResponse->u8Endpoint, u16ClusterID) != E_ZB_OK){
            ERR_vPrintln(T_TRUE, "eZigbeeNodeAddCluster error\n");
            //eZigbeeRemoveNode(psZigbeeNode);
            return ;
        }
    }
    eLockunLock(&psZigbeeNode->mutex);
    vZCB_InitZigbeeNodeInfo(psZigbeeNode, psSimpleDescriptorResponse->u16DeviceID);
}
/**
 * 将一个设备节点踢出网络
 * */
static void vZCB_HandleDeviceLeave(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x004D]vZCB_HandleDeviceLeave\n");
    struct _tsLeaveIndication {
        uint64    u64IEEEAddress;
        uint8     u8Rejoin;
    } PACKED *psMessage = (struct _tsLeaveIndication *)pvMessage;
    
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);

    tsZigbeeNodes *psZigbeeNode = psZigbeeFindNodeByIEEEAddress(psMessage->u64IEEEAddress);
    if(NULL == psZigbeeNode){
        ERR_vPrintln(T_TRUE, "Can't find this node in the network!\n");
        return;
    }
    eZigbeeSqliteUpdateDeviceOnline(psZigbeeNode->sNode.u64IEEEAddress, 0);

    if(psZigbeeNode) eZigbeeRemoveNode(psZigbeeNode);
    
    return;
}

static void vZCB_HandleAlarm(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x0011]vZCB_HandleAlarm\n");
    struct _tsAlarm {
        uint8     u8Sequence;
        uint8     u8SrcEndpoint;
        uint16    u16ClusterID;
        uint16    u16ShortAddress;
        uint8     u8AlarmCode;
        uint16    u16AlarmCluster;
    } PACKED *psMessage = (struct _tsAlarm *)pvMessage;

    psMessage->u16ClusterID   = ntohs(psMessage->u16ClusterID);
    psMessage->u16ShortAddress   = ntohs(psMessage->u16ShortAddress);
    psMessage->u16AlarmCluster   = ntohs(psMessage->u16AlarmCluster);
    DBG_vPrintln( DBG_ZCB, "Alarm from 0x%04X.\n",psMessage->u16ShortAddress);
    tsZigbeeNodes *psZigbeeNode = psZigbeeFindNodeByShortAddress(psMessage->u16ShortAddress);
    if(NULL == psZigbeeNode){
        ERR_vPrintln(T_TRUE, "Can't find this node in the network!\n");
        return;
    }
    //TODO:将报警信息存入数据库并通知云端
    if(psMessage->u8AlarmCode == E_RECORD_TYPE_LOCAL_OPEN_THREATE){
        eSocketDoorAlarmReport(1);
        eZigbeeSqliteAddDoorLockRecord(E_RECORD_TYPE_LOCAL_OPEN_THREATE, 0, (uint32)time((time_t*)NULL),NULL);
    } else if(psMessage->u8AlarmCode == E_RECORD_TYPE_LOCAL_OPEN_VIOLENCE){
        eSocketDoorAlarmReport(0);
        eZigbeeSqliteAddDoorLockRecord(E_RECORD_TYPE_LOCAL_OPEN_VIOLENCE, 0, (uint32)time((time_t*)NULL),NULL);
    }
    return;
}
/**
 * 处理节点主动上报的属性
 * */
static void vZCB_HandleAttributeReport(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x8102]vZCB_HandleAttributeReport\n");
    struct _tsAttributeReport {
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8AttributeStatus;
        uint8     u8Type;
        uint16    u16SizeOfAttributesInBytes;
        tuZcbAttributeData uData;
    } PACKED *psMessage = (struct _tsAttributeReport *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);
    psMessage->u16AttributeID   = ntohs(psMessage->u16AttributeID);
   // psMessage->eType   = ntohs(psMessage->eType);

    DBG_vPrintln( DBG_ZCB, "Attribute report from 0x%04X - Endpoint %d, cluster 0x%04X, attribute 0x%04X, status %d, type 0x%x, len %d.\n",
                psMessage->u16ShortAddress,
                psMessage->u8Endpoint,
                psMessage->u16ClusterID,
                psMessage->u16AttributeID,
                psMessage->u8AttributeStatus,
                psMessage->u8Type,
                psMessage->u16SizeOfAttributesInBytes
            );

    tsZigbeeNodes *psZigbeeNode = psZigbeeFindNodeByShortAddress(psMessage->u16ShortAddress);
    if(NULL == psZigbeeNode){
        WAR_vPrintln(T_TRUE, "Can't find this node in network.\n");
        return;
    }
    tuZcbAttributeData uAttributeData;
    switch(psMessage->u8Type)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            uAttributeData.u8Data = psMessage->uData.u8Data;
            break;

        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            INF_vPrintln(DBG_ZCB, "data %d\n", ntohs(psMessage->uData.u16Data));
            uAttributeData.u16Data = ntohs(psMessage->uData.u16Data);
            break;

        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            uAttributeData.u32Data = ntohl(psMessage->uData.u32Data);
            break;

        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            uAttributeData.u64Data = be64toh(psMessage->uData.u64Data);
            break;

        default:
            ERR_vPrintln(T_TRUE,  "Unknown attribute data type (%d) received from node 0x%04X", psMessage->u8Type, psMessage->u16ShortAddress);
            break;
    }

    eLockLock(&psZigbeeNode->mutex);
    switch(psMessage->u16ClusterID){
        case E_ZB_CLUSTERID_POWER:
            INF_vPrintln(DBG_ZCB, "update door lock power to %d\n", uAttributeData.u16Data);
            psZigbeeNode->sNode.sAttributeValue.u16Battery = uAttributeData.u16Data;
            eSocketPowerConfigurationReport((uint8)(psZigbeeNode->sNode.sAttributeValue.u16Battery/10));

            break;
        default:
            break;
    }
    eLockunLock(&psZigbeeNode->mutex);

    return ;
}

static void vZCB_HandleDoorLockSetUser(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x00F5]vZCB_HandleDoorLockSetUser\n");
    struct _tsSetDoorLockUser {
        uint8     u8SequenceNo;
        uint8     u8SrcEndpoint;
        uint16    u16ClusterID;
        uint8     u8AddressMode;
        uint16    u16ShortAddress;
        uint8     u8UserType;
        uint8     u8UserID;
        uint8     u8UserPermStatus;
        uint8     u8Command;
    } PACKED *psMessage = (struct _tsSetDoorLockUser *)pvMessage;

    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);

    DBG_vPrintln( DBG_ZCB, "Set user request from 0x%04X - Endpoint %d, user id %d, command %d.\n",
                  psMessage->u16ShortAddress,
                  psMessage->u8SrcEndpoint,
                  psMessage->u8UserID,
                  psMessage->u8Command);

    //tsZigbeeNodes *psZigbeeNode = psZigbeeFindNodeByShortAddress(psMessage->u16ShortAddress);
    //if(NULL == psZigbeeNode){
    //    WAR_vPrintln(T_TRUE, "Can't find this node in network.\n");
    //    return;
    //}

    if(E_CLD_DOOR_LOCK_CMD_SET_USER_STATUS == psMessage->u8Command){
        eSocketDoorUserDelReport(psMessage->u8UserID);
        eZigbeeSqliteDelDoorLockUser(psMessage->u8UserID);
    } else if(E_CLD_DOOR_LOCK_CMD_SET_USER_TYPE == psMessage->u8Command){
        eSocketDoorUserAddReport(psMessage->u8UserID, psMessage->u8UserType, psMessage->u8UserPermStatus);
        eZigbeeSqliteAddDoorLockUser(psMessage->u8UserID, psMessage->u8UserType, psMessage->u8UserPermStatus, "DoorLock");
    }

    return ;
}

static void vZCB_HandleDoorLockStateReport(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x00F6]vZCB_HandleDoorLockStateReport\n");
    struct _tsDoorStateReport {
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;
        uint8     u8SrcEndpoint;
        uint8     u8UserType;
        uint8     u8UserID;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8AttributeStatus;
        teZCL_ZCLAttributeType     eType;
        uint16    u16SizeOfAttributesInBytes;
        tuZcbAttributeData uData;
    } PACKED *psMessage = (struct _tsDoorStateReport *)pvMessage;

    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);
    psMessage->u16AttributeID   = ntohs(psMessage->u16AttributeID);

    DBG_vPrintln( DBG_ZCB, "Door lock state report from 0x%04X - Endpoint %d, user id %d.\n",
                  psMessage->u16ShortAddress,
                  psMessage->u8SrcEndpoint,
                  psMessage->u8UserID);

    eZigbeeSqliteAddDoorLockRecord((teDoorLockUserType) psMessage->u8UserType, psMessage->u8UserID,
                                   (uint32) time((time_t *) NULL), NULL);
    eSocketDoorLockReport(psMessage->u8UserID, (uint8)((psMessage->u8UserType == E_RECORD_TYPE_LOCAL_OPEN_NON_NORMAL)?1:0));
    sleep(1);
    eZCB_DoorLockDeviceOperator(&sControlBridge.sNode, E_CLD_DOOR_LOCK_DEVICE_CMD_LOCK);

    return ;
}

static void vZCB_HandleDoorLockOpenRequest(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintln(DBG_ZCB, "************[0x00F2]vZCB_HandleDoorLockOpenRequest\n");
    struct _tsDoorOpenRequest {
        uint8     u8SequenceNo;
        uint8     u8SrcEndpoint;
        uint16    u16ClusterID;
        uint8     u8AddressMode;
        uint16    u16ShortAddress;
        uint8     u8UserType;
        uint8     u8UserID;
        uint8     u8Command;
        uint8     u8PasswordID;
        uint8     u8PasswordLen;
    } PACKED *psMessage = (struct _tsDoorOpenRequest *)pvMessage;
    uint8 auPassword[DOOR_LOCK_PASSWORD_LEN] = {0};

    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);
    memcpy(auPassword, (char*)pvMessage + sizeof(struct _tsDoorOpenRequest), psMessage->u8PasswordLen);

    DBG_vPrintln( DBG_ZCB, "Door lock request open door from 0x%04X - Endpoint %d, password id %d, password %s.\n",
                  psMessage->u16ShortAddress,
                  psMessage->u8SrcEndpoint,
                  psMessage->u8PasswordID,
                  auPassword);
    tsTemporaryPassword sPassword = {0};
    eZigbeeSqliteDoorLockRetrievePassword(psMessage->u8PasswordID, &sPassword);
    if(sPassword.u8AvailableNum != 0xFF && sPassword.u8AvailableNum != 0){
        sPassword.u8AvailableNum--;
    }
    sPassword.u8UseNum += 1;
    eZigbeeSqliteUpdateDoorLockPassword(psMessage->u8PasswordID, sPassword.u8AvailableNum, sPassword.u8Worked, sPassword.u8UseNum);
    eZigbeeSqliteAddDoorLockRecord((teDoorLockUserType) psMessage->u8UserType, psMessage->u8UserID,
                                   (uint32) time((time_t *) NULL), (const char*)sPassword.auPassword);
    sleep(1);/* Lock Door */
    eZCB_DoorLockDeviceOperator(&sControlBridge.sNode, E_CLD_DOOR_LOCK_DEVICE_CMD_LOCK);
    eSocketDoorLockReport(psMessage->u8UserID, 0);

    return ;
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eZCB_Init(char *cpSerialDevice, uint32 u32BaudRate)
{
    memset(&sControlBridge, 0, sizeof(tsZigbeeBase));
    dl_list_init(&sControlBridge.list);
    /** Lock Control bridge when add or delete the list of nodes */
    CHECK_STATUS(eLockCreate(&sControlBridge.mutex), E_THREAD_OK, E_ZB_ERROR);
    CHECK_RESULT(eSL_Init(cpSerialDevice, u32BaudRate), E_SL_OK, E_ZB_ERROR);
    /** Register listeners */
    eSL_AddListener(E_SL_MSG_NODE_CLUSTER_LIST,             vZCB_HandleNodeClusterList,               NULL);
    eSL_AddListener(E_SL_MSG_NODE_ATTRIBUTE_LIST,           vZCB_HandleNodeClusterAttributeList,      NULL);
    eSL_AddListener(E_SL_MSG_NODE_COMMAND_ID_LIST,          vZCB_HandleNodeCommandIDList,             NULL);
    eSL_AddListener(E_SL_MSG_NETWORK_JOINED_FORMED,         vZCB_HandleNetworkJoined,                 NULL);
    eSL_AddListener(E_SL_MSG_DEVICE_ANNOUNCE,               vZCB_HandleDeviceAnnounce,                NULL);
    eSL_AddListener(E_SL_MSG_LEAVE_INDICATION,              vZCB_HandleDeviceLeave,                   NULL);
    eSL_AddListener(E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE,     vZCB_HandleMatchDescriptorResponse,       NULL);
    eSL_AddListener(E_SL_MSG_REPORT_IND_ATTR_RESPONSE,      vZCB_HandleAttributeReport,               NULL);
    eSL_AddListener(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE,    vZCB_HandleSimpleDescriptorResponse,      NULL);
    eSL_AddListener(E_SL_MSG_NODE_NON_FACTORY_NEW_RESTART,  vZCB_HandleRestartProvisioned,            NULL);
    eSL_AddListener(E_SL_MSG_NODE_FACTORY_NEW_RESTART,      vZCB_HandleRestartFactoryNew,             NULL);
    eSL_AddListener(E_SL_MSG_DOOR_LOCK_SET_DOOR_USER,       vZCB_HandleDoorLockSetUser,               NULL);
    eSL_AddListener(E_SL_MSG_DOOR_LOCK_STATE_REPORT,        vZCB_HandleDoorLockStateReport,           NULL);
    eSL_AddListener(E_SL_MSG_LOCK_UNLOCK_DOOR_PASSWD,       vZCB_HandleDoorLockOpenRequest,           NULL);
    eSL_AddListener(E_SL_MSG_ALARM,                         vZCB_HandleAlarm,                         NULL);

    return E_ZB_OK;
}

teZbStatus eZCB_Finish(void)
{
    eSL_Destroy();
    eSL_RemoveAllListener();
    return E_ZB_OK;
}

teZbStatus eZCB_EstablishComm(void)
{
    if (eSL_SendMessage(E_SL_MSG_GET_VERSION, 0, NULL, NULL) == E_SL_OK) {
        uint16 u16Length;
        uint32  *u32Version;

        /* Wait 300ms for the versions message to arrive */
        if (eSL_MessageWait(E_SL_MSG_VERSION_LIST, 300, &u16Length, (void**)&u32Version) == E_SL_OK) {
            uint32 version = ntohl(*u32Version);

            DBG_vPrintln(DBG_ZCB, "Connected to control bridge version 0x%08x\n", version );
            free(u32Version);

            DBG_vPrintln(DBG_ZCB, "Reset control bridge\n");
            usleep(1000);
            if (eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL) != E_SL_OK) {
                return E_ZB_COMMS_FAILED;
            }
            return E_ZB_OK;
        }
    }

    return E_ZB_COMMS_FAILED;
}

teZbStatus eZCB_SetPermitJoining(uint8 u8Interval)
{
    struct _PermitJoiningMessage {
        uint16    u16TargetAddress;
        uint8     u8Interval;
        uint8     u8TCSignificance;
    } PACKED sPermitJoiningMessage;

    DBG_vPrintln(DBG_ZCB, "Permit joining (%d) \n", u8Interval);

    sPermitJoiningMessage.u16TargetAddress  = htons(E_ZB_BROADCAST_ADDRESS_ROUTERS);
    sPermitJoiningMessage.u8Interval        = u8Interval;
    sPermitJoiningMessage.u8TCSignificance  = 0;

    if (eSL_SendMessage(E_SL_MSG_PERMIT_JOINING_REQUEST, sizeof(struct _PermitJoiningMessage), &sPermitJoiningMessage, NULL) != E_SL_OK) {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}

teZbStatus eZCB_ChannelRequest(uint8 *pu8Channel)
{
    if (eSL_SendMessage(E_SL_MSG_CHANNEL_REQUEST, 0, NULL, NULL) == E_SL_OK) {
        uint8 *pu8chan;
        uint16 u16Length;

        /* Wait 300ms for the versions message to arrive */
        if (eSL_MessageWait(E_SL_MSG_CHANNEL_RESPONSE, 300, &u16Length, (void**)&pu8chan) == E_SL_OK) {
            *pu8Channel = *pu8chan;
            DBG_vPrintln(DBG_ZCB, "the devices' channel is %d\n", *pu8Channel );
        } else {
            ERR_vPrintln(T_TRUE, "No response to channel request\n");
            return E_ZB_ERROR;
        }
    }
    return E_ZB_OK;
}

teZbStatus eZCB_MatchDescriptorRequest(uint16 u16TargetAddress,
                                       uint16 u16ProfileID,
                                       uint8  u8NumInputClusters,
                                       uint16 *pau16InputClusters,
                                       uint8  u8NumOutputClusters,
                                       uint16 *pau16OutputClusters,
                                       uint8  *pu8SequenceNo)
{
    uint8 au8Buffer[256];
    uint16 u16Position = 0;
    int i;

    DBG_vPrintln(DBG_ZCB, "Send Match Descriptor request for profile ID 0x%04X to 0x%04X\n", u16ProfileID, u16TargetAddress);

    u16TargetAddress = htons(u16TargetAddress);
    memcpy(&au8Buffer[u16Position], &u16TargetAddress, sizeof(uint16));
    u16Position += sizeof(uint16);

    u16ProfileID = htons(u16ProfileID);
    memcpy(&au8Buffer[u16Position], &u16ProfileID, sizeof(uint16));
    u16Position += sizeof(uint16);

    au8Buffer[u16Position] = u8NumInputClusters;
    u16Position++;

    DBG_vPrintln(DBG_ZCB, "Input Cluster List:\n");

    for (i = 0; i < u8NumInputClusters; i++) {
        uint16 u16ClusterID = htons(pau16InputClusters[i]);
        DBG_vPrintln(DBG_ZCB, "0x%04X\n", pau16InputClusters[i]);
        memcpy(&au8Buffer[u16Position], &u16ClusterID , sizeof(uint16));
        u16Position += sizeof(uint16);
    }

    DBG_vPrintln(DBG_ZCB, "Output Cluster List:\n");

    au8Buffer[u16Position] = u8NumOutputClusters;
    u16Position++;

    for (i = 0; i < u8NumOutputClusters; i++) {
        uint16 u16ClusterID = htons(pau16OutputClusters[i] );
        DBG_vPrintln(DBG_ZCB, "0x%04X\n", pau16OutputClusters[i]);
        memcpy(&au8Buffer[u16Position], &u16ClusterID , sizeof(uint16));
        u16Position += sizeof(uint16);
    }

    if (eSL_SendMessage(E_SL_MSG_MATCH_DESCRIPTOR_REQUEST, u16Position, au8Buffer, pu8SequenceNo) != E_SL_OK) {
        return E_ZB_COMMS_FAILED;
    }

    return E_ZB_OK;
}

teZbStatus eZCB_IEEEAddressRequest(tsZigbeeBase *psZigbee_Node)
{
    struct _IEEEAddressRequest {
        uint16    u16TargetAddress;
        uint16    u16ShortAddress;
        uint8     u8RequestType;
        uint8     u8StartIndex;
    } PACKED sIEEEAddressRequest;

    struct _IEEEAddressResponse {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint64    u64IEEEAddress;
        uint16    u16ShortAddress;
        uint8     u8NumAssociatedDevices;
        uint8     u8StartIndex;
        uint16    au16DeviceList[255];
    } PACKED *psIEEEAddressResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    DBG_vPrintln(DBG_ZCB, "Send IEEE Address request to 0x%04X\n", psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u16TargetAddress    = htons(psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u16ShortAddress     = htons(psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u8RequestType       = 0;
    sIEEEAddressRequest.u8StartIndex        = 0;

    if (eSL_SendMessage(E_SL_MSG_IEEE_ADDRESS_REQUEST, sizeof(struct _IEEEAddressRequest), &sIEEEAddressRequest, &u8SequenceNo) != E_SL_OK){
        goto done;
    }

    while (1)
    {
        /** Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_IEEE_ADDRESS_RESPONSE, 1000, &u16Length, (void**)&psIEEEAddressResponse) != E_SL_OK){
            ERR_vPrintln(T_TRUE, "No response to IEEE address request\n");
            goto done;
        }
        if (u8SequenceNo == psIEEEAddressResponse->u8SequenceNo){
            break;
        } else {
            DBG_vPrintln(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n",
                         psIEEEAddressResponse->u8SequenceNo, u8SequenceNo);
            FREE(psIEEEAddressResponse);
            psIEEEAddressResponse = NULL;
        }
    }
    if(NULL != psIEEEAddressResponse){
        psZigbee_Node->u64IEEEAddress = be64toh(psIEEEAddressResponse->u64IEEEAddress);
    }

    DBG_vPrintln(DBG_ZCB, "Short address 0x%04X has IEEE Address 0x%016llX\n", psZigbee_Node->u16ShortAddress, (unsigned long long int)psZigbee_Node->u64IEEEAddress);
    eStatus = E_ZB_OK;

    done:
    //vZigbee_NodeUpdateComms(psZigbee_Node, eStatus);
    FREE(psIEEEAddressResponse);
    return eStatus;
}

teZbStatus eZCB_SimpleDescriptorRequest(tsZigbeeBase *psZigbee_Node, uint8 u8Endpoint)
{
    struct _SimpleDescriptorRequest {
        uint16    u16TargetAddress;
        uint8     u8Endpoint;
    } PACKED sSimpleDescriptorRequest;

    uint8 u8SequenceNo;

    DBG_vPrintln(DBG_ZCB, "Send Simple Desciptor request for Endpoint %d to 0x%04X\n", u8Endpoint, psZigbee_Node->u16ShortAddress);

    sSimpleDescriptorRequest.u16TargetAddress = htons(psZigbee_Node->u16ShortAddress);
    sSimpleDescriptorRequest.u8Endpoint       = u8Endpoint;

    if (eSL_SendMessage(E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST, sizeof(struct _SimpleDescriptorRequest), &sSimpleDescriptorRequest, &u8SequenceNo) != E_SL_OK){
        return E_ZB_COMMS_FAILED;
    }

    return E_ZB_OK;
}

teZbStatus eZCB_ZLL_OnOff(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode)
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8           u8SequenceNo;

    struct {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Mode;
    } PACKED sOnOffMessage;

    /** Just read control bridge, not need lock */
    psSourceEndpoint = psZigbeeNodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_ONOFF);
    if (!psSourceEndpoint) {
        DBG_vPrintln(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_ONOFF);
        return E_ZB_ERROR;
    }
    sOnOffMessage.u8SourceEndpoint = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode) {
        if (bZCB_EnableAPSAck) {
            sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sOnOffMessage.u16TargetAddress = htons(psZigbeeNode->u16ShortAddress);
        psDestinationEndpoint = psZigbeeNodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_ONOFF);
        if (psDestinationEndpoint) {
            sOnOffMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        } else {
            sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
    } else {
        sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sOnOffMessage.u16TargetAddress      = htons(u16GroupAddress);
        sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    sOnOffMessage.u8Mode = u8Mode;

    if (eSL_SendMessage(E_SL_MSG_ONOFF_NOEFFECTS, sizeof(sOnOffMessage), &sOnOffMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZB_COMMS_FAILED;
    }
    if(bZCB_EnableAPSAck) {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    } else {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_AddGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress)
{
    struct _AddGroupMembershipRequest {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
    } PACKED sAddGroupMembershipRequest;

    struct _sAddGroupMembershipResponse {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
    } PACKED *psAddGroupMembershipResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    DBG_vPrintln(DBG_ZCB, "Send add group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZigbeeNode->u16ShortAddress);

    if (bZCB_EnableAPSAck) {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    } else {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sAddGroupMembershipRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);

    if ((eStatus = eZigbeeGetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_GROUPS,
                                       &sAddGroupMembershipRequest.u8SourceEndpoint,
                                       &sAddGroupMembershipRequest.u8DestinationEndpoint)) != E_ZB_OK) {
        return eStatus;
    }

    sAddGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_ADD_GROUP_REQUEST,
                        sizeof(struct _AddGroupMembershipRequest), &sAddGroupMembershipRequest, &u8SequenceNo) != E_SL_OK) {
        goto done;
    }

    while (1)
    {
        /** Wait 1 second for the add group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_ADD_GROUP_RESPONSE, 1000, &u16Length, (void**)&psAddGroupMembershipResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE,  "No response to add group membership request");
            goto done;
        }

        /** Work around bug in Zigbee */
        //TODO:
        if (u8SequenceNo != psAddGroupMembershipResponse->u8SequenceNo) {
            break;
        } else {
            DBG_vPrintln(DBG_ZCB, "Add group membership sequence number received 0x%02X does not match that sent 0x%02X\n",
                         psAddGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            FREE(psAddGroupMembershipResponse);
        }
    }

    DBG_vPrintln(DBG_ZCB, "Add group membership 0x%04X on Node 0x%04X status: %d\n",
                 u16GroupAddress, psZigbeeNode->u16ShortAddress, psAddGroupMembershipResponse->u8Status);
    eStatus = (teZbStatus)psAddGroupMembershipResponse->u8Status;

    done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psAddGroupMembershipResponse);
    return eStatus;
}

teZbStatus eZCB_RemoveGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress)
{
    struct _RemoveGroupMembershipRequest {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
    } PACKED sRemoveGroupMembershipRequest;

    struct _sRemoveGroupMembershipResponse {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
    } PACKED *psRemoveGroupMembershipResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    DBG_vPrintln(DBG_ZCB, "Send remove group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZigbeeNode->u16ShortAddress);

    if (bZCB_EnableAPSAck) {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    } else {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sRemoveGroupMembershipRequest.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);

    if (eZigbeeGetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_GROUPS, &sRemoveGroupMembershipRequest.u8SourceEndpoint,
                            &sRemoveGroupMembershipRequest.u8DestinationEndpoint) != E_ZB_OK) {
        return E_ZB_ERROR;
    }

    sRemoveGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_REMOVE_GROUP_REQUEST, sizeof(struct _RemoveGroupMembershipRequest), &sRemoveGroupMembershipRequest, &u8SequenceNo) != E_SL_OK) {
        goto done;
    }

    while (1)
    {
        /** Wait 1 second for the remove group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_GROUP_RESPONSE, 1000, &u16Length, (void**)&psRemoveGroupMembershipResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE,  "No response to remove group membership request");
            goto done;
        }

        /** Work around bug in Zigbee */
        if (u8SequenceNo != psRemoveGroupMembershipResponse->u8SequenceNo){
            break;
        }else{
            DBG_vPrintln(DBG_ZCB, "Remove group membership sequence number received 0x%02X does not match that sent 0x%02X\n",
                         psRemoveGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            FREE(psRemoveGroupMembershipResponse);
        }
    }

    DBG_vPrintln(DBG_ZCB, "Remove group membership 0x%04X on Node 0x%04X status: %d\n",
                 u16GroupAddress, psZigbeeNode->u16ShortAddress, psRemoveGroupMembershipResponse->u8Status);

    eStatus = (teZbStatus)psRemoveGroupMembershipResponse->u8Status;

    done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psRemoveGroupMembershipResponse);
    return eStatus;
}

teZbStatus eZCB_ClearGroupMembership(tsZigbeeBase *psZigbeeNode)
{
    struct _ClearGroupMembershipRequest {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
    } PACKED sClearGroupMembershipRequest;

    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    DBG_vPrintln(DBG_ZCB, "Send clear group membership request to 0x%04X\n", psZigbeeNode->u16ShortAddress);

    if (bZCB_EnableAPSAck) {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    } else {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sClearGroupMembershipRequest.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);

    if (eZigbeeGetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_GROUPS, &sClearGroupMembershipRequest.u8SourceEndpoint,
                            &sClearGroupMembershipRequest.u8DestinationEndpoint) != E_ZB_OK) {
        return E_ZB_ERROR;
    }

    if (eSL_SendMessage(E_SL_MSG_REMOVE_ALL_GROUPS, sizeof(struct _ClearGroupMembershipRequest), &sClearGroupMembershipRequest, NULL) != E_SL_OK) {
        goto done;
    }
    eStatus = E_ZB_OK;

    done:
    return eStatus;
}

teZbStatus eZCB_StoreScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID)
{
    struct _StoreSceneRequest {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED sStoreSceneRequest;

    struct _sStoreSceneResponse {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED *psStoreSceneResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    DBG_vPrintln(DBG_ZCB, "Send store scene %d (Group 0x%04X)\n",
                 u8SceneID, u16GroupAddress);

    if (psZigbeeNode) {
        if (bZCB_EnableAPSAck) {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sStoreSceneRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);

        if (eZigbeeGetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_SCENES,
                                &sStoreSceneRequest.u8SourceEndpoint, &sStoreSceneRequest.u8DestinationEndpoint) != E_ZB_OK) {
            return E_ZB_ERROR;
        }
    } else {
        sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sStoreSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sStoreSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;

        if (eZigbeeGetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sStoreSceneRequest.u8SourceEndpoint, NULL) != E_ZB_OK) {
            return E_ZB_ERROR;
        }
    }
    sStoreSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sStoreSceneRequest.u8SceneID        = u8SceneID;
    if (eSL_SendMessage(E_SL_MSG_STORE_SCENE, sizeof(struct _StoreSceneRequest), &sStoreSceneRequest, &u8SequenceNo) != E_SL_OK) {
        goto done;
    }

    while (1)
    {
        /** Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_STORE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psStoreSceneResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE,  "No response to store scene request");
            goto done;
        }

        /** Work around bug in Zigbee */
        if (u8SequenceNo != psStoreSceneResponse->u8SequenceNo) {
            break;
        }
        else {
            DBG_vPrintln(DBG_ZCB, "Store scene sequence number received 0x%02X does not match that sent 0x%02X\n",
                         psStoreSceneResponse->u8SequenceNo, u8SequenceNo);
            FREE(psStoreSceneResponse);
        }
    }

    DBG_vPrintln(DBG_ZCB, "Store scene %d (Group0x%04X) on Node 0x%04X status: %d\n",
                 psStoreSceneResponse->u8SceneID, ntohs(psStoreSceneResponse->u16GroupAddress),
                 ntohs(psZigbeeNode->u16ShortAddress), psStoreSceneResponse->u8Status);

    eStatus = (teZbStatus)psStoreSceneResponse->u8Status;
    done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psStoreSceneResponse);
    return eStatus;
}

teZbStatus eZCB_RemoveScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SceneID)
{
    struct _RemoveSceneRequest {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED sRemoveSceneRequest;

    struct _sStoreSceneResponse {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED *psRemoveSceneResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;


    if (psZigbeeNode) {
        DBG_vPrintln(DBG_ZCB, "Send remove scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n",
                     u16SceneID, u16GroupAddress, sRemoveSceneRequest.u8DestinationEndpoint, psZigbeeNode->u16ShortAddress);
        if (bZCB_EnableAPSAck) {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRemoveSceneRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);

        if (eZigbeeGetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint,
                                &sRemoveSceneRequest.u8DestinationEndpoint) != E_ZB_OK) {
            return E_ZB_ERROR;
        }
    } else {
        sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sRemoveSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sRemoveSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;

        if (eZigbeeGetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint, NULL) != E_ZB_OK) {
            return E_ZB_ERROR;
        }
    }

    sRemoveSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRemoveSceneRequest.u8SceneID        = (uint8)u16SceneID;

    if (eSL_SendMessage(E_SL_MSG_REMOVE_SCENE, sizeof(struct _RemoveSceneRequest), &sRemoveSceneRequest, &u8SequenceNo) != E_SL_OK) {
        goto done;
    }

    while (1)
    {
        /** Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psRemoveSceneResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE,  "No response to remove scene request");
            goto done;
        }

        /** Work around bug in Zigbee */
        if (u8SequenceNo != psRemoveSceneResponse->u8SequenceNo) {
            break;
        } else {
            DBG_vPrintln(DBG_ZCB, "Remove scene sequence number received 0x%02X does not match that sent 0x%02X\n",
                         psRemoveSceneResponse->u8SequenceNo, u8SequenceNo);
            FREE(psRemoveSceneResponse);
        }
    }

    DBG_vPrintln(DBG_ZCB, "Remove scene %d (Group0x%04X) on Node 0x%04X status: %d\n",
                 psRemoveSceneResponse->u8SceneID, ntohs(psRemoveSceneResponse->u16GroupAddress),
                 psZigbeeNode->u16ShortAddress, psRemoveSceneResponse->u8Status);

    eStatus = (teZbStatus)psRemoveSceneResponse->u8Status;
    done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psRemoveSceneResponse);
    return eStatus;
}

teZbStatus eZCB_RecallScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID)
{
    uint8         u8SequenceNo;
    struct _RecallSceneRequest {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED sRecallSceneRequest;

    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    if (psZigbeeNode) {
        DBG_vPrintln(DBG_ZCB, "Send recall scene %d (Group 0x%04X) to 0x%04X\n",
                     u8SceneID, u16GroupAddress, psZigbeeNode->u16ShortAddress);

        if (bZCB_EnableAPSAck) {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRecallSceneRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);

        if (eZigbeeGetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_SCENES,
                                &sRecallSceneRequest.u8SourceEndpoint, &sRecallSceneRequest.u8DestinationEndpoint) != E_ZB_OK) {
            return E_ZB_ERROR;
        }
    } else {
        sRecallSceneRequest.u8TargetAddressMode  = E_ZB_ADDRESS_MODE_GROUP;
        sRecallSceneRequest.u16TargetAddress     = htons(u16GroupAddress);
        sRecallSceneRequest.u8DestinationEndpoint= ZB_DEFAULT_ENDPOINT_ZLL;

        if (eZigbeeGetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRecallSceneRequest.u8SourceEndpoint, NULL) != E_ZB_OK) {
            return E_ZB_ERROR;
        }
    }

    sRecallSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRecallSceneRequest.u8SceneID        = u8SceneID;

    if (eSL_SendMessage(E_SL_MSG_RECALL_SCENE, sizeof(struct _RecallSceneRequest), &sRecallSceneRequest, &u8SequenceNo) != E_SL_OK) {
        goto done;
    }

    if (psZigbeeNode) {
        eStatus = eZCB_GetDefaultResponse(u8SequenceNo);
    } else {
        eStatus = E_ZB_OK;
    }
    done:
    return eStatus;
}

teZbStatus eZCB_ReadAttributeRequest(tsZigbeeBase *psZigbee_Node,
                                     uint16 u16ClusterID,
                                     uint8 u8Direction,
                                     uint8 u8ManufacturerSpecific,
                                     uint16 u16ManufacturerID,
                                     uint16 u16AttributeID,
                                     void *pvData)
{
    struct _ReadAttributeRequest {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16ClusterID;
        uint8     u8Direction;
        uint8     u8ManufacturerSpecific;
        uint16    u16ManufacturerID;
        uint8     u8NumAttributes;
        uint16    au16Attribute[1];
    } PACKED sReadAttributeRequest;

    struct _ReadAttributeResponse {
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8Status;
        uint8     u8Type;
        uint16    u16SizeOfAttributesInBytes;
        union {
            uint8     u8Data;
            uint16    u16Data;
            uint32    u32Data;
            uint64    u64Data;
        } uData;
    } PACKED *psReadAttributeResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    DBG_vPrintln(DBG_ZCB, "Send Read Attribute request to 0x%04X\n", psZigbee_Node->u16ShortAddress);
    DBG_vPrintln(DBG_ZCB, "Send Read Cluster ID = 0x%04X\n", u16ClusterID);

    if (bZCB_EnableAPSAck) {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    } else {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sReadAttributeRequest.u16TargetAddress      = htons(psZigbee_Node->u16ShortAddress);

    if ((eStatus = eZigbeeGetEndpoints(psZigbee_Node, u16ClusterID,
                                       &sReadAttributeRequest.u8SourceEndpoint,
                                       &sReadAttributeRequest.u8DestinationEndpoint)) != E_ZB_OK) {
        ERR_vPrintln(T_TRUE, "Can't Find Endpoint\n");
        goto done;
    }

    sReadAttributeRequest.u16ClusterID = htons(u16ClusterID);
    sReadAttributeRequest.u8Direction = u8Direction;
    sReadAttributeRequest.u8ManufacturerSpecific = u8ManufacturerSpecific;
    sReadAttributeRequest.u16ManufacturerID = htons(u16ManufacturerID);
    sReadAttributeRequest.u8NumAttributes = 1;
    sReadAttributeRequest.au16Attribute[0] = htons(u16AttributeID);

    if (eSL_SendMessage(E_SL_MSG_READ_ATTRIBUTE_REQUEST, sizeof(struct _ReadAttributeRequest), &sReadAttributeRequest, &u8SequenceNo) != E_SL_OK) {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_READ_ATTRIBUTE_RESPONSE, 1000, &u16Length, (void**)&psReadAttributeResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE, "No response to read attribute request\n");
            eStatus = E_ZB_COMMS_FAILED;
            goto done;
        }

        if (u8SequenceNo == psReadAttributeResponse->u8SequenceNo) {
            break;
        } else {
            ERR_vPrintln(T_TRUE, "Read Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", psReadAttributeResponse->u8SequenceNo, u8SequenceNo);
            FREE(psReadAttributeResponse);
            goto done;
        }
    }

    if (psReadAttributeResponse->u8Status != E_ZB_OK) {
        DBG_vPrintln(DBG_ZCB, "Read Attribute respose error status: %d\n", psReadAttributeResponse->u8Status);
        goto done;
    }

    /* Copy the data into the pointer passed to us.
     * We assume that the memory pointed to will be the right size for the data that has been requested!
     */
    DBG_vPrintln(DBG_ZCB, "Copy the data into the pointer passed to us\n");
    switch(psReadAttributeResponse->u8Type)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            WAR_vPrintln(1, "data:%d\n", psReadAttributeResponse->uData.u8Data);
            memcpy(pvData, &psReadAttributeResponse->uData.u8Data, sizeof(uint8));
            eStatus = E_ZB_OK;
            break;

        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            psReadAttributeResponse->uData.u16Data = ntohs(psReadAttributeResponse->uData.u16Data);
            memcpy(pvData, &psReadAttributeResponse->uData.u16Data, sizeof(uint16));
            eStatus = E_ZB_OK;
            break;

        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            psReadAttributeResponse->uData.u32Data = ntohl(psReadAttributeResponse->uData.u32Data);
            memcpy(pvData, &psReadAttributeResponse->uData.u32Data, sizeof(uint32));
            eStatus = E_ZB_OK;
            break;

        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            psReadAttributeResponse->uData.u64Data = be64toh(psReadAttributeResponse->uData.u64Data);
            memcpy(pvData, &psReadAttributeResponse->uData.u64Data, sizeof(uint64));
            eStatus = E_ZB_OK;
            break;

        default:
            ERR_vPrintln(T_TRUE,  "Unknown attribute data type (%d) received from node 0x%04X", psReadAttributeResponse->u8Type, psZigbee_Node->u16ShortAddress);
            break;
    }
    done:
    FREE(psReadAttributeResponse);
    return eStatus;
}

teZbStatus eZCB_ManagementLeaveRequest(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren)
{
    struct _sManagementLeavRequest {
        uint16    u16TargetShortAddress;
        uint64    u64TargetAddress;
        uint8     u8Rejoin;
        uint8     u8RemoveChildren;
    } PACKED sManagementLeaveRequest;

    struct _sManagementLeavResponse {
        uint8     u8SequenceNo;
        uint8     u8Status;
    } PACKED *psManagementLeaveResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_ERROR;

    if (psZigbeeNode) {
        DBG_vPrintln(DBG_ZCB, "Send management Leave (u64TargetAddress %llu)\n", psZigbeeNode->u64IEEEAddress);
        sManagementLeaveRequest.u16TargetShortAddress = 0x0000;
        sManagementLeaveRequest.u8Rejoin = u8Rejoin;
        sManagementLeaveRequest.u8RemoveChildren = u8RemoveChildren;
        sManagementLeaveRequest.u64TargetAddress = Swap64(psZigbeeNode->u64IEEEAddress);
    }

    if (eSL_SendMessage(E_SL_MSG_MANAGEMENT_LEAVE_REQUEST, sizeof(sManagementLeaveRequest), &sManagementLeaveRequest, &u8SequenceNo) != E_SL_OK) {
        ERR_vPrintln(T_TRUE,  "eSL_SendMessage Error\n");
        goto done;
    }

    while (1)
    {
        if (eSL_MessageWait(E_SL_MSG_MANAGEMENT_LEAVE_RESPONSE, 1000, &u16Length, (void**)&psManagementLeaveResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE,  "No response to Management Leave request\n");
            goto done;
        }
        break;
    }

    eStatus = (teZbStatus)psManagementLeaveResponse->u8Status;

    //psZigbeeNode->u8DeviceOnline = 0;
    //eZigbeeSqliteUpdateDeviceTable(psZigbeeNode);
    eZigbeeSqliteUpdateDeviceOnline(psZigbeeNode->u64IEEEAddress, 0);

    tsZigbeeNodes *psZigbeeNodeT = NULL;
    psZigbeeNodeT = psZigbeeFindNodeByIEEEAddress(psZigbeeNode->u64IEEEAddress);
    if(psZigbeeNodeT) eZigbeeRemoveNode(psZigbeeNodeT);
    done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psManagementLeaveResponse);
    return eStatus;
}

teZbStatus eZCB_ZLL_MoveToLevel(tsZigbeeBase *psZigbeeNode,
                                uint16 u16GroupAddress,
                                uint8 u8OnOff,
                                uint8 u8Level,
                                uint16 u16TransitionTime)
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;

    struct {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8OnOff;
        uint8     u8Level;
        uint16    u16TransitionTime;
    } PACKED sLevelMessage;

    if (u8Level > 254) {
        u8Level = 254;
    }

    psSourceEndpoint = psZigbeeNodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_LEVEL_CONTROL);
    if (!psSourceEndpoint) {
        DBG_vPrintln(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_LEVEL_CONTROL);
        return E_ZB_ERROR;
    }
    sLevelMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode) {
        if (bZCB_EnableAPSAck) {
            sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sLevelMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);

        psDestinationEndpoint = psZigbeeNodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_LEVEL_CONTROL);

        if (psDestinationEndpoint) {
            sLevelMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        } else {
            sLevelMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
    } else {
        sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sLevelMessage.u16TargetAddress      = htons(u16GroupAddress);
        sLevelMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }

    sLevelMessage.u8OnOff               = u8OnOff;
    sLevelMessage.u8Level               = u8Level;
    sLevelMessage.u16TransitionTime     = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_LEVEL_ONOFF, sizeof(sLevelMessage), &sLevelMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZB_COMMS_FAILED;
    }
    if (bZCB_EnableAPSAck) {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    } else {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_ZLL_MoveToHueSaturation(tsZigbeeBase *psZigbeeNode,
                                        uint16 u16GroupAddress,
                                        uint8 u8Hue,
                                        uint8 u8Saturation,
                                        uint16 u16TransitionTime)
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;

    struct {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Hue;
        uint8     u8Saturation;
        uint16    u16TransitionTime;
    } PACKED sMoveToHueSaturationMessage;

    DBG_vPrintln(DBG_ZCB, "Set Hue %d, Saturation %d\n", u8Hue, u8Saturation);

    psSourceEndpoint = psZigbeeNodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint) {
        DBG_vPrintln(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }
    sMoveToHueSaturationMessage.u8SourceEndpoint = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode) {
        if (bZCB_EnableAPSAck) {
            sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToHueSaturationMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);

        psDestinationEndpoint = psZigbeeNodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint) {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        } else {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
    } else {
        sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToHueSaturationMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToHueSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }

    sMoveToHueSaturationMessage.u8Hue               = u8Hue;
    sMoveToHueSaturationMessage.u8Saturation        = u8Saturation;
    sMoveToHueSaturationMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE_SATURATION,
                        sizeof(sMoveToHueSaturationMessage), &sMoveToHueSaturationMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZB_COMMS_FAILED;
    }

    if (bZCB_EnableAPSAck) {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    } else {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_NeighbourTableRequest(int *pStart)
{
    struct _ManagementLQIRequest {
        uint16    u16TargetAddress;
        uint8     u8StartIndex;
    } PACKED sManagementLQIRequest;

    struct _ManagementLQIResponse {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint8     u8NeighbourTableSize;
        uint8     u8TableEntries;
        uint8     u8StartIndex;
        struct {
            uint16    u16ShortAddress;
            uint64    u64PanID;
            uint64    u64IEEEAddress;
            uint8     u8Depth;
            uint8     u8LQI;
            struct {
                unsigned    uDeviceType : 2;
                unsigned    uPermitJoining: 2;
                unsigned    uRelationship : 2;
                unsigned    uMacCapability : 2;
            } PACKED sBitmap;
        } PACKED asNeighbours[255];
    } PACKED *psManagementLQIResponse = NULL;

    uint16 u16ShortAddress;
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    int i;uint8 u8MacCapability = 0;

    u16ShortAddress = 0x0000; //coordinator

    sManagementLQIRequest.u16TargetAddress = htons(u16ShortAddress);
    sManagementLQIRequest.u8StartIndex     = (uint8)*pStart;

    DBG_vPrintln(DBG_ZCB, "Send management LQI request to 0x%04X for entries starting at %d\n",
                 u16ShortAddress, sManagementLQIRequest.u8StartIndex);

    if (eSL_SendMessage(E_SL_MSG_MANAGEMENT_LQI_REQUEST, sizeof(struct _ManagementLQIRequest), &sManagementLQIRequest, &u8SequenceNo) != E_SL_OK) {
        goto done;
    }

    while (1)
    {
        /** Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_MANAGEMENT_LQI_RESPONSE, 1000, &u16Length, (void**)&psManagementLQIResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE, "No response to management LQI requestn\n");
            goto done;
        } else if (u8SequenceNo == psManagementLQIResponse->u8SequenceNo) {
            break;
        } else {
            DBG_vPrintln(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psManagementLQIResponse->u8SequenceNo, u8SequenceNo);
            FREE(psManagementLQIResponse);
        }
    }
    if(psManagementLQIResponse->u8Status == CZD_NW_STATUS_SUCCESS) {
        DBG_vPrintln(DBG_ZCB, "Received management LQI response. Table size: %d, Entry count: %d, start index: %d\n",
                     psManagementLQIResponse->u8NeighbourTableSize,
                     psManagementLQIResponse->u8TableEntries,
                     psManagementLQIResponse->u8StartIndex);
    } else {
        DBG_vPrintln(DBG_ZCB, "Received error status in management LQI response : 0x%02x\n",psManagementLQIResponse->u8Status);
        goto done;
    }

    for (i = 0; i < psManagementLQIResponse->u8TableEntries; i++)
    {
        psManagementLQIResponse->asNeighbours[i].u16ShortAddress    = ntohs(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);
        psManagementLQIResponse->asNeighbours[i].u64PanID           = be64toh(psManagementLQIResponse->asNeighbours[i].u64PanID);
        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress     = be64toh(psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);

        if ((psManagementLQIResponse->asNeighbours[i].u16ShortAddress >= 0xFFFA) ||
            (psManagementLQIResponse->asNeighbours[i].u64IEEEAddress  == 0)) {
            /* Illegal short / IEEE address */
            continue;
        }

        DBG_vPrintln(DBG_ZCB, "  Entry %02d: Short Address 0x%04X, PAN ID: 0x%016llX, IEEE Address: 0x%016llX\n", i,
                     psManagementLQIResponse->asNeighbours[i].u16ShortAddress,
                     psManagementLQIResponse->asNeighbours[i].u64PanID,
                     psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);

        DBG_vPrintln(DBG_ZCB, "    Type: %d, Permit Joining: %d, Relationship: %d, RxOnWhenIdle: %d\n",
                     psManagementLQIResponse->asNeighbours[i].sBitmap.uDeviceType,
                     psManagementLQIResponse->asNeighbours[i].sBitmap.uPermitJoining,
                     psManagementLQIResponse->asNeighbours[i].sBitmap.uRelationship,
                     psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability);
        if(psManagementLQIResponse->asNeighbours[i].sBitmap.uDeviceType == 0x02){   //EndDevice
            u8MacCapability &= ~(E_ZB_MAC_CAPABILITY_FFD);
        } else {
            u8MacCapability |= (E_ZB_MAC_CAPABILITY_FFD);
        }
        if(psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability == 0x00){   //EndDevice
            u8MacCapability &= ~(E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE);
        } else {
            u8MacCapability |= (E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE);
        }

        DBG_vPrintln(DBG_ZCB, "    Depth: %d, LQI: %d\n",
                     psManagementLQIResponse->asNeighbours[i].u8Depth,
                     psManagementLQIResponse->asNeighbours[i].u8LQI);

        vZCB_AddNodeIntoNetwork(psManagementLQIResponse->asNeighbours[i].u16ShortAddress,
                                psManagementLQIResponse->asNeighbours[i].u64IEEEAddress, u8MacCapability);

        sleep(1);//avoid compete
    }

    if (psManagementLQIResponse->u8TableEntries > 0) {
        // We got some entries, so next time request the entries after these.
        *pStart += psManagementLQIResponse->u8TableEntries;
        if (*pStart >= psManagementLQIResponse->u8NeighbourTableSize)/* Make sure we're still in table boundaries */
        {
            *pStart = 0;
        }
    } else {
        // No more valid entries.
        *pStart = 0;
    }

    eStatus = E_ZB_OK;
    done:
    FREE(psManagementLQIResponse);
    return eStatus;
}

teZbStatus eZCB_WindowCoveringDeviceOperator(tsZigbeeBase *psZigbeeNode, teCLD_WindowCovering_CommandID eCommand )
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8         u8SequenceNo;

    struct {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Command;
    } PACKED sWindowCoveringDeviceMessage;

    /* Just read control bridge, not need lock */
    psSourceEndpoint = psZigbeeNodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_WINDOW_COVERING_DEVICE);
    if (!psSourceEndpoint) {
        ERR_vPrintln(T_TRUE, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_WINDOW_COVERING_DEVICE);
        return E_ZB_ERROR;
    }
    sWindowCoveringDeviceMessage.u8SourceEndpoint = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode) {
        if (bZCB_EnableAPSAck) {
            sWindowCoveringDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sWindowCoveringDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sWindowCoveringDeviceMessage.u16TargetAddress = htons(psZigbeeNode->u16ShortAddress);
        psDestinationEndpoint = psZigbeeNodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_WINDOW_COVERING_DEVICE);
        if (psDestinationEndpoint) {
            sWindowCoveringDeviceMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        } else {
            sWindowCoveringDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
        }
    } else {
        sWindowCoveringDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_BROADCAST;
        sWindowCoveringDeviceMessage.u16TargetAddress      = htons(E_ZB_BROADCAST_ADDRESS_ALL);
        sWindowCoveringDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
    }
    sWindowCoveringDeviceMessage.u8Command = (uint8)eCommand;

    if (eSL_SendMessage(E_SL_MSG_WINDOW_COVERING_DEVICE_OPERATOR, sizeof(sWindowCoveringDeviceMessage), &sWindowCoveringDeviceMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZB_COMMS_FAILED;
    }
    if(bZCB_EnableAPSAck) {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    } else {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_DoorLockDeviceOperator(tsZigbeeBase *psZigbeeNode, teCLD_DoorLock_CommandID eCommand )
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8         u8SequenceNo;

    DBG_vPrintln( DBG_ZCB, "eZCB_DoorLockDeviceOperator:%d", eCommand);
                  struct {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Command;
    } PACKED sDoorLockDeviceMessage;

    /* Just read control bridge, not need lock */
    psSourceEndpoint = psZigbeeNodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_DOOR_LOCK);
    if (!psSourceEndpoint) {
        ERR_vPrintln(T_TRUE, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_DOOR_LOCK);
        return E_ZB_ERROR;
    }
    sDoorLockDeviceMessage.u8SourceEndpoint = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode) {
        if (bZCB_EnableAPSAck) {
            sDoorLockDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        } else {
            sDoorLockDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sDoorLockDeviceMessage.u16TargetAddress = htons(psZigbeeNode->u16ShortAddress);
        psDestinationEndpoint = psZigbeeNodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_DOOR_LOCK);
        if (psDestinationEndpoint) {
            sDoorLockDeviceMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        } else {
            sDoorLockDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
        }
    } else {
        sDoorLockDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_BROADCAST;
        sDoorLockDeviceMessage.u16TargetAddress      = htons(E_ZB_BROADCAST_ADDRESS_ALL);
        sDoorLockDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
    }
    sDoorLockDeviceMessage.u8Command = (uint8)eCommand;

    if(sDoorLockDeviceMessage.u16TargetAddress == 0){/*Coordinator*/
        if (eSL_SendMessage(E_SL_MSG_DOOR_LOCK_SET_DOOR_STATE, sizeof(sDoorLockDeviceMessage), &sDoorLockDeviceMessage, &u8SequenceNo) != E_SL_OK) {
            return E_ZB_COMMS_FAILED;
        }
    }else{
        if (eSL_SendMessage(E_SL_MSG_LOCK_UNLOCK_DOOR, sizeof(sDoorLockDeviceMessage), &sDoorLockDeviceMessage, &u8SequenceNo) != E_SL_OK) {
            return E_ZB_COMMS_FAILED;
        }
        if(bZCB_EnableAPSAck) {
            return eZCB_GetDefaultResponse(u8SequenceNo);
        }
    }
    return E_ZB_OK;
}

teZbStatus eZCB_GetDefaultResponse(uint8 u8SequenceNo)
{
    uint16 u16Length;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    tsSL_Msg_DefaultResponse *psDefaultResponse = NULL;

    while (1)
    {
        /* Wait 1 second for a default response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_DEFAULT_RESPONSE, 1000, &u16Length, (void**)&psDefaultResponse) != E_SL_OK) {
            ERR_vPrintln(T_TRUE,  "No response to command sequence number %d received", u8SequenceNo);
            goto done;
        }

        if (u8SequenceNo != psDefaultResponse->u8SequenceNo) {
            DBG_vPrintln(DBG_ZCB, "Default response sequence number received 0x%02X does not match that sent 0x%02X\n",
                         psDefaultResponse->u8SequenceNo, u8SequenceNo);
            FREE(psDefaultResponse);
        } else {
            DBG_vPrintln(DBG_ZCB, "Default response for message sequence number 0x%02X status is %d\n",
                         psDefaultResponse->u8SequenceNo, psDefaultResponse->u8Status);
            eStatus = (teZbStatus)psDefaultResponse->u8Status;
            break;
        }
    }
    done:
    FREE(psDefaultResponse);
    return eStatus;
}

teZbStatus eZCB_ResetNetwork(tsZigbeeBase *psZigbeeNode)
{
    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_ERASE_PERSISTENT_DATA, 0, NULL, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

teZbStatus eZCB_SetDoorLockPassword(tsZigbeeBase *psZigbeeNode, uint8 u8PasswordId, uint8 u8Command, uint8 u8PasswordLen, const char *psPassword)
{
    struct _tDoorLockSetPassword{
        uint8 u8Sequence;
        uint16 u16Address;
        uint8 u8Add;
        uint8 u8PasswordId;
        uint8 u8Length;
        uint8 auPassword[DOOR_LOCK_PASSWORD_LEN];
    }PACKED sDoorLockSetPassword = {0};

    sDoorLockSetPassword.u16Address = 0x0000;
    sDoorLockSetPassword.u8Add = u8Command;
    sDoorLockSetPassword.u8PasswordId = u8PasswordId;
    sDoorLockSetPassword.u8Length = u8PasswordLen;
    memcpy(sDoorLockSetPassword.auPassword, psPassword, u8PasswordLen);

    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_DOOR_LOCK_SET_DOOR_PASSWORD,
                                 sizeof(struct _tDoorLockSetPassword),
                                 &sDoorLockSetPassword,
                                 NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

teZbStatus eZCB_DeviceRecognition(uint64 u64MacAddress)
{
    tsZigbeeNodes *psZigbeeNode = psZigbeeFindNodeByIEEEAddress(u64MacAddress);
    if(NULL == psZigbeeNode){
        ERR_vPrintln(T_TRUE, "Can't find this node in network");
        return E_ZB_ERROR;
    }

    if (eZCB_SimpleDescriptorRequest(&psZigbeeNode->sNode, CONFIG_DEFAULT_ENDPOINT) != E_ZB_OK){
        ERR_vPrintln(T_TRUE, "Failed to read endpoint simple descriptor - requeue\n");
        return E_ZB_ERROR;
    }
    return E_ZB_OK;
}