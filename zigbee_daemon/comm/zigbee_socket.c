/****************************************************************************
 *
 * MODULE:             Zigbee - daemon
 *
 * COMPONENT:          SocketServer interface
 *
 * REVISION:           $Revision: 43420 $
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <arpa/inet.h>
#include <zigbee_node.h>
#include <zigbee_sqlite.h>
#include <door_lock.h>

#include "utils.h"
#include "zigbee_socket.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_SOCKET (verbosity >= 7)
#define NUM_SOCKET_QUEUE 5
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


static void *pvSocketServerThread(void *psThreadInfoVoid);
static void *pvSocketCallbackHandlerThread(void *psThreadInfoVoid);

static void vResponseJsonString(int iSocketFd, teSS_Status eStatus, const char *info);

static teSS_Status eSocketHandleGetMac(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetVersion(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetChannel(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandlePermitJoin(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleLeaveNetwork(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetAllDevicesList(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetLightOnOff(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetLightLevel(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetLightRGB(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetLightStatus(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetLightLevel(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetLightRGB(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetSensorValue(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetClosuresState(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetDoorLockState(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleDoorLockAddPassword(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleDoorLockDelPassword(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleDoorLockGetPassword(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleDoorLockGetRecord(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleDoorLockGetUser(int iSocketFd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSearchDevice(int iSocketFd, struct json_object *psJsonMessage);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern int verbosity;
extern const char *pVersion;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static int iSequenceNumber = 0;
static tsSocketServer sSocketServer;
static tsClientSocket ClientSocket[NUMBER_SOCKET_CLIENT];

static tsSocketHandleMap sSocketHandleMap[] = {
    {E_SS_COMMAND_GET_MAC,                  eSocketHandleGetMac},
    {E_SS_COMMAND_GET_VERSION,              eSocketHandleGetVersion},
    {E_SS_COMMAND_OPEN_NETWORK,             eSocketHandlePermitJoin},
    {E_SS_COMMAND_GET_CHANNEL,              eSocketHandleGetChannel},
    {E_SS_COMMAND_GET_DEVICES_LIST_ALL,     eSocketHandleGetAllDevicesList},
    {E_SS_COMMAND_LEAVE_NETWORK,            eSocketHandleLeaveNetwork},
    {E_SS_COMMAND_SEARCH_DEVICE,            eSocketHandleSearchDevice},
    /** Light */
    {E_SS_COMMAND_LIGHT_SET_ON_OFF,         eSocketHandleSetLightOnOff},
    {E_SS_COMMAND_LIGHT_SET_LEVEL,          eSocketHandleSetLightLevel},
    {E_SS_COMMAND_LIGHT_SET_RGB,            eSocketHandleSetLightRGB},
    {E_SS_COMMAND_LIGHT_GET_STATUS,         eSocketHandleGetLightStatus},
    {E_SS_COMMAND_LIGHT_GET_LEVEL,          eSocketHandleGetLightLevel},
    {E_SS_COMMAND_LIGHT_GET_RGB,            eSocketHandleGetLightRGB},
    /** Sensor */
    {E_SS_COMMAND_SENSOR_GET_SENSOR,        eSocketHandleGetSensorValue},
    /** Window Covering */
    {E_SS_COMMAND_SET_CLOSURES_STATE,       eSocketHandleSetClosuresState},
    /** Door Lock */
    {E_SS_COMMAND_DOOR_LOCK_ADD_PASSWORD,           eSocketHandleDoorLockAddPassword},
    {E_SS_COMMAND_DOOR_LOCK_DEL_PASSWORD,           eSocketHandleDoorLockDelPassword},
    {E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD,           eSocketHandleDoorLockGetPassword},
    {E_SS_COMMAND_DOOR_LOCK_GET_RECORD,             eSocketHandleDoorLockGetRecord},
    {E_SS_COMMAND_DOOR_LOCK_GET_USER,               eSocketHandleDoorLockGetUser},
    {E_SS_COMMAND_SET_DOOR_LOCK_STATE,              eSocketHandleSetDoorLockState},
};
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
static void *pvSocketServerThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSocketServer *psSocketServer = (tsSocketServer*)psThreadInfo->pvThreadData;
    psThreadInfo->eState = E_THREAD_RUNNING;

    fd_set fdSelect, fdTemp;
    FD_ZERO(&fdSelect);//Init fd
    FD_SET(psSocketServer->iSocketFd, &fdSelect);//Add socketserver fd into select fd
    int iListenFD = 0;
    if(psSocketServer->iSocketFd > iListenFD) {
        iListenFD = psSocketServer->iSocketFd;
    }

    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        fdTemp = fdSelect;  /* use temp value, because this value will be clear */
        int iResult = select(iListenFD + 1, &fdTemp, NULL, NULL, NULL);
        switch(iResult)
        {
            case 0:
                DBG_vPrintln(DBG_SOCKET, "receive message time out \n");
                break;

            case -1:
                WAR_vPrintln(T_TRUE,"receive message error:%s \n", strerror(errno));
                break;

            default:
            {
                if(FD_ISSET(psSocketServer->iSocketFd, &fdTemp))//there is client accept
                {
                    DBG_vPrintln(DBG_SOCKET, "A client connecting... \n");
                    if(psSocketServer->u8NumClientSocket >= 5){
                        DBG_vPrintln(DBG_SOCKET, "Client already connected 5, don't allow connected\n");
                        FD_CLR(psSocketServer->iSocketFd, &fdSelect);//delete this Server from select set
                        break;
                    }
                    for(int i = 0; i < NUMBER_SOCKET_CLIENT; i++)
                    {
                        if(ClientSocket[i].iSocketClient == -1){
                            ClientSocket[i].iSocketClient = accept(psSocketServer->iSocketFd, (struct sockaddr *)&ClientSocket[i].addrclint, (socklen_t *) &ClientSocket[i].u16Length);
                            if(-1 == ClientSocket[i].iSocketClient){
                                ERR_vPrintln(T_TRUE,"accept client connecting error \n");
                                break;
                            } else {
                                DBG_vPrintln(DBG_SOCKET, "Client (%d-%d) already connected\n", i, ClientSocket[i].iSocketClient);
                                FD_SET(ClientSocket[i].iSocketClient, &fdSelect);
                                if(ClientSocket[i].iSocketClient > iListenFD){
                                    iListenFD = ClientSocket[i].iSocketClient;
                                }
                                psSocketServer->u8NumClientSocket ++;
                                break;
                            }
                        }
                    }
                }
                else
                {   //there is client communication
                    for(int i = 0; ((i < NUMBER_SOCKET_CLIENT)&&(-1 != ClientSocket[i].iSocketClient)); i++)
                    {
                        if(FD_ISSET(ClientSocket[i].iSocketClient, &fdTemp)) {
                            memset(ClientSocket[i].auClientData, 0, sizeof(ClientSocket[i].auClientData));
                            ClientSocket[i].u16Length = (uint16)recv(ClientSocket[i].iSocketClient, ClientSocket[i].auClientData, sizeof(ClientSocket[i].auClientData), 0);
                            if (0 == ClientSocket[i].u16Length){
                                ERR_vPrintln(T_TRUE, "Can't recv client[%d] message, close it\n", i);
                                close(ClientSocket[i].iSocketClient);
                                FD_SET(psSocketServer->iSocketFd, &fdSelect);//Add socketserver fd into select fd
                                FD_CLR(ClientSocket[i].iSocketClient, &fdSelect);//delete this client from select set
                                ClientSocket[i].iSocketClient = -1;
                                psSocketServer->u8NumClientSocket --;
                            } else {
                                tsSSCallbackThreadData *psSCallBackThreadData = NULL;
                                psSCallBackThreadData = (tsSSCallbackThreadData*)malloc(sizeof(tsSSCallbackThreadData));
                                if (!psSCallBackThreadData){
                                    ERR_vPrintln(T_TRUE, "Memory allocation failure");
                                    break;
                                }
                                memset(psSCallBackThreadData, 0, sizeof(tsSSCallbackThreadData));
                                psSCallBackThreadData->u16Length = ClientSocket[i].u16Length;
                                psSCallBackThreadData->iSocketClientfd = ClientSocket[i].iSocketClient;
                                memcpy(psSCallBackThreadData->au8Message, ClientSocket[i].auClientData, sizeof(psSCallBackThreadData->au8Message));

                                eQueueEnqueue(&psSocketServer->sQueue, psSCallBackThreadData);
                            }
                        }
                    }
                }
            }
                break;  /*default*/
        }
    }

    DBG_vPrintln(DBG_SOCKET, "pvSocketServerThread Exit\n");
    vThreadFinish(psThreadInfo)/* Return from thread clearing resources */;
    return NULL;
}

static void *pvSocketCallbackHandlerThread(void *psThreadInfoVoid)
{
    DBG_vPrintln(DBG_SOCKET, "pvSocketCallbackHandlerThread\n");
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSocketServer *psSocketServer = (tsSocketServer*)psThreadInfo->pvThreadData;
    psThreadInfo->eState = E_THREAD_RUNNING;

    tsSSCallbackThreadData *psCallbackData = NULL;
    while(psThreadInfo->eState == E_THREAD_RUNNING)
    {
        eQueueDequeue(&psSocketServer->sQueue, (void **) &psCallbackData);

        teSocketCommand eSocketCommand;
        struct json_object *psJsonMessage = NULL;
        if(NULL != (psJsonMessage = json_tokener_parse((const char*)psCallbackData->au8Message)))
        {
            struct json_object *psJsonTemp  = NULL;
            if(json_object_object_get_ex(psJsonMessage,JSON_TYPE, &psJsonTemp))
            {
                eSocketCommand = (teSocketCommand)json_object_get_int(psJsonTemp);
                if(json_object_object_get_ex(psJsonMessage,JSON_SEQUENCE, &psJsonTemp)) {
                    iSequenceNumber = json_object_get_int(psJsonTemp);
                    for(int i = 0; i < sizeof(sSocketHandleMap)/sizeof(tsSocketHandleMap); i++) {
                        if(eSocketCommand == sSocketHandleMap[i].eSocketCommand) {
                            teSS_Status eStatus = sSocketHandleMap[i].preMessageHandlePacket(psCallbackData->iSocketClientfd, psJsonMessage);
                            if(E_SS_INCORRECT_PARAMETERS == eStatus){
                                vResponseJsonString(psCallbackData->iSocketClientfd, E_SS_INCORRECT_PARAMETERS, "Parameters Error");
                            } else if(E_SS_ERROR == eStatus){
                                vResponseJsonString(psCallbackData->iSocketClientfd, E_SS_ERROR, "Command Failed");
                            }
                        }
                    }
                }
            } else {
                vResponseJsonString(psCallbackData->iSocketClientfd, E_SS_ERROR_UNHANDLED_COMMAND, "Command invalid");
            }
            json_object_put(psJsonMessage);//free json object's memory
        } else {
            ERR_vPrintln(T_TRUE, "ResponseJsonFormatError error\n");
            vResponseJsonString(psCallbackData->iSocketClientfd, E_SS_INCORRECT_PARAMETERS, "Parameters Error");
        }
        FREE(psCallbackData);
        eThreadYield();
    }
    DBG_vPrintln(DBG_SOCKET, "pvSocketCallbackHandlerThread Exit\n");
    vThreadFinish(psThreadInfo);

    return NULL;
}
//////////////////////////////////////Message Handle////////////////////////////////////////
static void vResponseJsonString(int iSocketFd, teSS_Status eStatus, const char *info)
{
    struct json_object* psJsonMessage = json_object_new_object();
    if(NULL == psJsonMessage) {
        ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
        return ;
    }
    json_object_object_add(psJsonMessage, JSON_STATUS,json_object_new_int(eStatus));
    json_object_object_add(psJsonMessage, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonMessage, JSON_INFO,json_object_new_string(info));
    json_object_object_add(psJsonMessage, JSON_TYPE,json_object_new_int(E_SS_COMMAND_STATUS));

    if(-1 == send(iSocketFd,
                  json_object_get_string(psJsonMessage),strlen(json_object_get_string(psJsonMessage)),0)) {
        ERR_vPrintln(T_TRUE, "send data to client error\n");
    }
    json_object_put(psJsonMessage);
    return;
}

static teSS_Status eSocketHandleGetVersion(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request is get version\n");
    teSS_Status eSS_Status = E_SS_OK;
    struct json_object* psJsonReturn = json_object_new_object();
    if(NULL == psJsonReturn) {
        ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
        return E_SS_ERROR;
    }
    json_object_object_add(psJsonReturn, JSON_TYPE,json_object_new_int(E_SS_COMMAND_VERSION_LIST));
    json_object_object_add(psJsonReturn, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonReturn, JSON_VERSION,json_object_new_string(pVersion));
    INF_vPrintln(DBG_SOCKET, "return message is ----%s\n",json_object_get_string(psJsonReturn));
    if(-1 == send(iSocketFd, json_object_get_string(psJsonReturn),strlen(json_object_get_string(psJsonReturn)), 0)) {
        ERR_vPrintln(T_TRUE, "send data to client error\n");
        eSS_Status = E_SS_ERROR;
    }
    json_object_put(psJsonReturn);
    return eSS_Status;
}

static teSS_Status eSocketHandleGetMac(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request is get host's mac\n");
    teSS_Status eSS_Status = E_SS_OK;
    struct json_object* psJsonReturn = json_object_new_object();
    if(NULL == psJsonReturn) {
        ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
        return E_SS_ERROR;
    }
    json_object_object_add(psJsonReturn, JSON_TYPE,json_object_new_int(E_SS_COMMAND_GET_MAC_RESPONSE));
    json_object_object_add(psJsonReturn, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonReturn, JSON_MAC,json_object_new_int64((int64_t)sControlBridge.sNode.u64IEEEAddress));
    INF_vPrintln(DBG_SOCKET, "return message is ----%s\n",json_object_get_string(psJsonReturn));
    if(-1 == send(iSocketFd, json_object_get_string(psJsonReturn),strlen(json_object_get_string(psJsonReturn)), 0)) {
        ERR_vPrintln(T_TRUE, "send data to client error\n");
        eSS_Status = E_SS_ERROR;
    }
    json_object_put(psJsonReturn);
    return eSS_Status;
}

static teSS_Status eSocketHandleGetChannel(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request is get coordinator channel\n");
    teSS_Status eSS_Status = E_SS_ERROR;

    uint8 u8Channel = 0;
    if((NULL != sControlBridge.Method.preCoordinatorGetChannel)&& (E_ZB_OK == sControlBridge.Method.preCoordinatorGetChannel(&u8Channel))){
        struct json_object* psJsonReturn = json_object_new_object();
        if(NULL == psJsonReturn) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            return eSS_Status;
        }
        json_object_object_add(psJsonReturn, JSON_TYPE,json_object_new_int(E_SS_COMMAND_GET_CHANNEL_RESPONSE));
        json_object_object_add(psJsonReturn, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonReturn, JSON_CHANNEL,json_object_new_int(u8Channel));
        INF_vPrintln(DBG_SOCKET, "return message is ----%s\n",json_object_get_string(psJsonReturn));
        if(-1 == send(iSocketFd, json_object_get_string(psJsonReturn),strlen(json_object_get_string(psJsonReturn)), 0)) {
            ERR_vPrintln(T_TRUE, "send data to client error\n");
        }
        json_object_put(psJsonReturn);
        return E_SS_OK;
    }

    return eSS_Status;
}

static teSS_Status eSocketHandlePermitJoin(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request open zigbee network\n");
    json_object *psJsonTemp = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_TIME, &psJsonTemp)) {
        uint8 uiPermitJoinTime = (uint8)json_object_get_int(psJsonTemp);
        if(sControlBridge.Method.preCoordinatorPermitJoin){
            sControlBridge.Method.preCoordinatorPermitJoin(uiPermitJoinTime);
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }
    ERR_vPrintln(T_TRUE, "ResponseMessageError error\n");
    return E_SS_ERROR;
}

static teSS_Status eSocketHandleLeaveNetwork(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request leave a device\n");
    json_object *psJsonTemp = NULL, *psJsonRejoin = NULL, *psJsonChildren = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonTemp)&&
       json_object_object_get_ex(psJsonMessage,JSON_REJOIN, &psJsonRejoin)&&
       json_object_object_get_ex(psJsonMessage,JSON_REMOVE_CHILDREN, &psJsonChildren))
    {
        uint8 u8Rejoin = (uint8)json_object_get_int(psJsonRejoin);
        uint8 u8RemoveChildren = (uint8)json_object_get_int(psJsonChildren);
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonTemp);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceRemoveNetwork) ||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceRemoveNetwork(&psZigbeeNode->sNode, u8Rejoin, u8RemoveChildren)))
        {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.DeviceRemoveNetwork error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleSearchDevice(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request leave a device\n");

    vResponseJsonString(iSocketFd, E_SS_OK, "Success");
    if(sControlBridge.Method.preCoordinatorSearchDevices){
        sControlBridge.Method.preCoordinatorSearchDevices();
    }
    return E_SS_OK;
}

static teSS_Status eSocketHandleGetAllDevicesList(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request is get online devices' message\n");

    struct json_object *psJsonDevice, *psJsonDevicesArray, *psJsonResult = NULL;
    if(NULL == (psJsonResult = json_object_new_object())) {
        ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
        return E_SS_ERROR;
    }
    json_object_object_add(psJsonResult, JSON_TYPE,json_object_new_int(E_SS_COMMAND_GET_DEVICES_RESPONSE));
    json_object_object_add(psJsonResult, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));

    if(NULL == (psJsonDevicesArray = json_object_new_array())) {
        ERR_vPrintln(T_TRUE, "json_object_new_array error\n");
        json_object_put(psJsonResult);
        return E_SS_ERROR;
    }

    tsZigbeeBase psZigbeeNode, *psZigbeeItem = NULL;
    memset(&psZigbeeNode, 0, sizeof(psZigbeeNode));
    eZigbeeSqliteRetrieveDevicesList(&psZigbeeNode);
    dl_list_for_each(psZigbeeItem, &psZigbeeNode.list, tsZigbeeBase, list)
    {
        if(NULL == (psJsonDevice = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            json_object_put(psJsonDevicesArray);
            eZigbeeSqliteRetrieveDevicesListFree(&psZigbeeNode);
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonDevice,JSON_NAME,json_object_new_string((const char*)psZigbeeItem->auDeviceName));
        json_object_object_add(psJsonDevice,JSON_ID,(json_object_new_int((psZigbeeItem->u16DeviceID))));
        json_object_object_add(psJsonDevice,JSON_ONLINE,json_object_new_int((psZigbeeItem->u8DeviceOnline)));
        json_object_object_add(psJsonDevice,JSON_MAC,json_object_new_int64((int64_t)psZigbeeItem->u64IEEEAddress));
        json_object_array_add(psJsonDevicesArray, psJsonDevice);
    }
    eZigbeeSqliteRetrieveDevicesListFree(&psZigbeeNode);
    json_object_object_add(psJsonResult,JSON_DEVICES,psJsonDevicesArray);

    INF_vPrintln(DBG_SOCKET, "return message is ----%s\n",json_object_to_json_string(psJsonResult));
    if(-1 == send(iSocketFd, json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0)) {
        ERR_vPrintln(T_TRUE, "send data to client error\n");
        json_object_put(psJsonResult);
        return E_SS_ERROR;
    }

    json_object_put(psJsonResult);
    return E_SS_OK;
}

static teSS_Status eSocketHandleSetLightOnOff(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request open light on\n");

    json_object *psJsonAddr, *psJsonGroup, *psJsonMode = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr) &&
       json_object_object_get_ex(psJsonMessage,JSON_GROUP, &psJsonGroup) &&
       json_object_object_get_ex(psJsonMessage,JSON_MODE, &psJsonMode))
    {
        uint8  u8Mode = (uint8)json_object_get_int(psJsonMode);
        uint16 u16GroupID = (uint16)json_object_get_int(psJsonGroup);
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceSetOnOff)) {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.DeviceSetOnOff error\n");
            return E_SS_ERROR;
        }
        if(E_ZB_OK != psZigbeeNode->Method.preDeviceSetOnOff(&psZigbeeNode->sNode, u16GroupID, u8Mode)) {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.DeviceSetOnOff error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }

    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleSetLightLevel(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request set light level\n");

    json_object *psJsonAddr, *psJsonGroup, *psJsonLevel = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr) &&
       json_object_object_get_ex(psJsonMessage,JSON_GROUP, &psJsonGroup) &&
       json_object_object_get_ex(psJsonMessage,JSON_LEVEL, &psJsonLevel))
    {
        uint8 u8Level = (uint8)json_object_get_int(psJsonLevel);
        uint16 u16GroupID = (uint16)json_object_get_int(psJsonGroup);
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode)||(NULL == psZigbeeNode->Method.preDeviceSetLevel)||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceSetLevel(&psZigbeeNode->sNode, u16GroupID, u8Level, 5)))
        {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.preDeviceSetLevel error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleSetLightRGB(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request set light rgb\n");
    json_object *psJsonAddr, *psJsonGroup, *psJsonRgb, *psJsonR, *psJsonG, *psJsonB = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,JSON_GROUP, &psJsonGroup)&&
       json_object_object_get_ex(psJsonMessage,JSON_COLOR, &psJsonRgb)&&
       json_object_object_get_ex(psJsonRgb,"R", &psJsonR)&&
       json_object_object_get_ex(psJsonRgb,"G", &psJsonG)&&
       json_object_object_get_ex(psJsonRgb,"B", &psJsonB))
    {
        uint16 u16GroupID = (uint16)json_object_get_int(psJsonGroup);
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);

        tsRGB sRGB;
        sRGB.R = (uint8)json_object_get_int(psJsonR);
        sRGB.G = (uint8)json_object_get_int(psJsonG);
        sRGB.B = (uint8)json_object_get_int(psJsonB);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode)||(NULL == psZigbeeNode->Method.preDeviceSetLightColour)||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceSetLightColour(&psZigbeeNode->sNode, u16GroupID, sRGB, 5)))
        {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.preDeviceSetLightColour error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }

    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleSetClosuresState(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request set closure device state\n");
    json_object *psJsonAddr = NULL, *psJsonOperator = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,JSON_COMMAND, &psJsonOperator))
    {
        uint8 u8Operator = (uint8)json_object_get_int(psJsonOperator);
        uint64 u64DeviceAddress = (uint8)json_object_get_int64(psJsonAddr);

        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode)||(NULL == psZigbeeNode->Method.preDeviceSetWindowCovering)||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceSetWindowCovering(&psZigbeeNode->sNode, (teCLD_WindowCovering_CommandID)u8Operator)))
        {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.preDeviceSetWindowCovering error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }

    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleGetLightLevel(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request get light level\n");
    json_object *psJsonAddr = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        uint8 u8level = 0;
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceGetLevel) ||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceGetLevel(&psZigbeeNode->sNode, &u8level)))
        {
            ERR_vPrintln(T_TRUE, "preDeviceGetLevel callback failed\n");
            return E_SS_ERROR;
        }
        struct json_object *psJsonResult = NULL;
        if(NULL == (psJsonResult = json_object_new_object()))
        {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }

        json_object_object_add(psJsonResult, JSON_TYPE,json_object_new_int(E_SS_COMMAND_LIGHT_GET_LEVEL_RESPONSE));
        json_object_object_add(psJsonResult, JSON_LEVEL, json_object_new_int(u8level));
        json_object_object_add(psJsonResult, JSON_SEQUENCE, json_object_new_int(iSequenceNumber));
        DBG_vPrintln(DBG_SOCKET, "psJsonResult %s, length is %d\n",
                     json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketFd,
                      json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            json_object_put(psJsonResult);
            ERR_vPrintln(T_TRUE, "send data to client error\n");
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleGetLightStatus(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request get light status\n");
    json_object *psJsonAddr = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        uint8 u8mode = 0;
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceGetLevel) ||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceGetOnOff(&psZigbeeNode->sNode, &u8mode)))
        {
            ERR_vPrintln(T_TRUE, "preDeviceGetOnOff callback failed\n");
            return E_SS_ERROR;
        }

        struct json_object *psJsonResult = NULL;
        if(NULL == (psJsonResult = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonResult, JSON_TYPE,json_object_new_int(E_SS_COMMAND_LIGHT_GET_STATUS_RESPONSE));
        json_object_object_add(psJsonResult, JSON_MODE,json_object_new_int(u8mode));
        json_object_object_add(psJsonResult, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));

        DBG_vPrintln(DBG_SOCKET, "psJsonResult %s, length is %d\n",
                     json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketFd,
                      json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            json_object_put(psJsonResult);
            ERR_vPrintln(T_TRUE, "send data to client error\n");
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleGetLightRGB(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request get light rgb\n");
    json_object *psJsonAddr = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        tsRGB sRGB;
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceGetLevel) ||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceGetLightColour(&psZigbeeNode->sNode, &sRGB))) {
            ERR_vPrintln(T_TRUE, "preDeviceGetOnOff callback failed\n");
            return E_SS_ERROR;
        }

        struct json_object *psJsonResult, *psJsonRGBValue = NULL;
        if(NULL == (psJsonResult = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        if(NULL == (psJsonRGBValue = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonRGBValue, "R",json_object_new_int(sRGB.R));
        json_object_object_add(psJsonRGBValue, "G",json_object_new_int(sRGB.G));
        json_object_object_add(psJsonRGBValue, "B",json_object_new_int(sRGB.B));

        json_object_object_add(psJsonResult,JSON_COLOR,psJsonRGBValue);
        json_object_object_add(psJsonResult, JSON_TYPE,json_object_new_int(E_SS_COMMAND_LIGHT_GET_RGB_RESPONSE));
        json_object_object_add(psJsonResult, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));
        DBG_vPrintln(DBG_SOCKET, "psJsonResult %s, length is %d\n",
                     json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketFd,
                      json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            ERR_vPrintln(T_TRUE, "send data to client error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleGetSensorValue(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request get sensor value\n");
    json_object *psJsonAddr = NULL, *psJsonSensor = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr) && json_object_object_get_ex(psJsonMessage,JSON_SENSOR, &psJsonSensor))
    {
        teZigbee_ClusterID eSensorType = (teZigbee_ClusterID)json_object_get_int(psJsonSensor);
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if(NULL == psZigbeeNode) {
            ERR_vPrintln(T_TRUE, "preDeviceGetOnOff callback failed\n");
            return E_SS_ERROR;
        }
        uint16 u16SensorValue = 0;
        if((NULL == psZigbeeNode->Method.preDeviceGetSensorValue)||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceGetSensorValue(&psZigbeeNode->sNode, &u16SensorValue, eSensorType)))
        {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.preDeviceGetSensorValue error\n");
            return E_SS_ERROR;
        }

        struct json_object *psJsonResult = NULL;
        if(NULL == (psJsonResult = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonResult, JSON_SENSOR, json_object_new_int(u16SensorValue));
        json_object_object_add(psJsonResult, JSON_SEQUENCE, json_object_new_int(iSequenceNumber));

        DBG_vPrintln(DBG_SOCKET, "psJsonResult %s, length is %d\n",
                     json_object_to_json_string(psJsonResult),
                     (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketFd, json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0)) {
            ERR_vPrintln(T_TRUE, "send data to client error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleSetDoorLockState(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request set door lock device state\n");
    json_object *psJsonAddr = NULL, *psJsonOperator = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,JSON_COMMAND, &psJsonOperator))
    {
        uint8 u8Operator = (uint8)json_object_get_int(psJsonOperator);
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);

        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        CHECK_POINTER(psZigbeeNode, E_SS_ERROR);
        CHECK_POINTER(psZigbeeNode->Method.preDeviceSetDoorLock, E_SS_ERROR);
        if(E_ZB_OK != psZigbeeNode->Method.preDeviceSetDoorLock(&psZigbeeNode->sNode, (teCLD_DoorLock_CommandID)u8Operator))
        {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.preDeviceSetDoorLock error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }

    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleDoorLockAddPassword(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request add a password into door lock\n");
    json_object *psJsonAddr, *psJsonID, *psJsonAvailable, *psJsonTime, *psJsonLen, *psJsonPassword = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,JSON_ID, &psJsonID)&&
       json_object_object_get_ex(psJsonMessage,JSON_PASSWORD_AVAILABLE, &psJsonAvailable)&&
       json_object_object_get_ex(psJsonMessage,JSON_TIME, &psJsonTime)&&
       json_object_object_get_ex(psJsonMessage,JSON_PASSWORD_LEN, &psJsonLen)&&
       json_object_object_get_ex(psJsonMessage,JSON_PASSWORD, &psJsonPassword))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);

        tsCLD_DoorLock_Payload sPayload = {0};
        sPayload.u8PasswordID = (uint8)json_object_get_int(psJsonID);
        sPayload.u8AvailableNum = (uint8)json_object_get_int(psJsonAvailable);
        sPayload.psTime = json_object_get_string(psJsonTime);
        sPayload.u8PasswordLen = (uint8)json_object_get_int(psJsonLen);
        sPayload.psPassword = json_object_get_string(psJsonPassword);

        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        CHECK_POINTER(psZigbeeNode, E_SS_ERROR);
        CHECK_POINTER(psZigbeeNode->Method.preDeviceSetDoorLockPassword, E_SS_ERROR);
        if(E_ZB_OK != psZigbeeNode->Method.preDeviceSetDoorLockPassword(&psZigbeeNode->sNode, &sPayload)) {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.preDeviceSetDoorLock error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleDoorLockDelPassword(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request add a password into door lock\n");
    json_object *psJsonAddr, *psJsonID = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,JSON_ID, &psJsonID))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);

        tsCLD_DoorLock_Payload sPayload = {0};
        sPayload.u8PasswordID = (uint8)json_object_get_int(psJsonID);
        sPayload.u8AvailableNum = 0;

        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        CHECK_POINTER(psZigbeeNode, E_SS_ERROR);
        CHECK_POINTER(psZigbeeNode->Method.preDeviceSetDoorLockPassword, E_SS_ERROR);
        if(E_ZB_OK != psZigbeeNode->Method.preDeviceSetDoorLockPassword(&psZigbeeNode->sNode, &sPayload)) {
            ERR_vPrintln(T_TRUE, "ZigbeeNode->Method.preDeviceSetDoorLock error\n");
            return E_SS_ERROR;
        }
        vResponseJsonString(iSocketFd, E_SS_OK, "Success");
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleDoorLockGetPassword(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request add a password into door lock\n");
    json_object *psJsonAddr, *psJsonID = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,JSON_ID, &psJsonID))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        uint8 u8PasswordID = (uint8)json_object_get_int(psJsonID);

        struct json_object *psJsonResult, *psJsonPassword, *psJsonArray = NULL;
        if(NULL == (psJsonResult = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        if(NULL == (psJsonPassword = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        if(NULL == (psJsonArray = json_object_new_array())){
            json_object_put(psJsonResult);
            json_object_put(psJsonPassword);
            return E_SS_ERROR;
        }

        json_object_object_add(psJsonResult, JSON_TYPE,json_object_new_int(E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD_RESPONSE));
        json_object_object_add(psJsonResult, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonResult, JSON_MAC,json_object_new_int64((int64_t)u64DeviceAddress));

        tsTemporaryPassword sPasswordHeader = {0};
        if(u8PasswordID == 0xff){//All Password
            eZigbeeSqliteDoorLockRetrievePasswordList(&sPasswordHeader);
            tsTemporaryPassword *Temp = NULL;
            dl_list_for_each(Temp, &sPasswordHeader.list, tsTemporaryPassword, list){
                struct json_object *psJosnTemp = json_object_new_object();
                json_object_object_add(psJosnTemp, JSON_ID,json_object_new_int(Temp->u8PasswordId));
                json_object_object_add(psJosnTemp, JSON_PASSWORD_AVAILABLE,json_object_new_int(Temp->u8AvailableNum));
                //json_object_object_add(psJosnTemp, JSON_TIME,json_object_new_int(Temp->u8AvailableNum));
                json_object_object_add(psJosnTemp, JSON_PASSWORD_LEN,json_object_new_int(Temp->u8PasswordLen));
                json_object_object_add(psJosnTemp, JSON_PASSWORD,json_object_new_string((const char*)Temp->auPassword));
                json_object_array_add(psJsonArray, psJosnTemp);
            }
            eZigbeeSqliteDoorLockRetrievePasswordListFree(&sPasswordHeader);
        } else {
            eZigbeeSqliteDoorLockRetrievePassword(u8PasswordID, &sPasswordHeader);
            struct json_object *psJosnTemp = json_object_new_object();
            json_object_object_add(psJosnTemp, JSON_ID,json_object_new_int(sPasswordHeader.u8PasswordId));
            json_object_object_add(psJosnTemp, JSON_PASSWORD_AVAILABLE,json_object_new_int(sPasswordHeader.u8AvailableNum));
            //json_object_object_add(psJosnTemp, JSON_TIME,json_object_new_int(Temp->u8AvailableNum));
            json_object_object_add(psJosnTemp, JSON_PASSWORD_LEN,json_object_new_int(sPasswordHeader.u8PasswordLen));
            json_object_object_add(psJosnTemp, JSON_PASSWORD,json_object_new_string((const char*)sPasswordHeader.auPassword));
            json_object_array_add(psJsonArray, psJosnTemp);
        }
        json_object_object_add(psJsonResult, JSON_PASSWORD,psJsonArray);
        DBG_vPrintln(DBG_SOCKET, "psJsonResult %s, length is %d\n",
                     json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketFd,
                      json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            ERR_vPrintln(T_TRUE, "send data to client error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleDoorLockGetRecord(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request add a password into door lock\n");
    json_object *psJsonAddr, *psJsonID, *psJsonNum = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,JSON_ID, &psJsonID)&&
            json_object_object_get_ex(psJsonMessage,JSON_NUM, &psJsonNum))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        uint8 u8RecordID = (uint8)json_object_get_int(psJsonID);
        uint8 u8Number = (uint8)json_object_get_int(psJsonNum);
        if(u8RecordID == 0xff){//All Users' Record

        } else {

        }
        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

static teSS_Status eSocketHandleDoorLockGetUser(int iSocketFd, struct json_object *psJsonMessage)
{
    INF_vPrintln(DBG_SOCKET, "Client request add a password into door lock\n");
    json_object *psJsonAddr = NULL;
    if(json_object_object_get_ex(psJsonMessage,JSON_MAC, &psJsonAddr))
    {
        uint64 u64DeviceAddress = (uint64)json_object_get_int64(psJsonAddr);
        struct json_object *psJsonResult, *psJsonUser, *psJsonArray = NULL;
        if(NULL == (psJsonResult = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        if(NULL == (psJsonUser = json_object_new_object())) {
            ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        if(NULL == (psJsonArray = json_object_new_array())){
            json_object_put(psJsonResult);
            json_object_put(psJsonUser);
            return E_SS_ERROR;
        }

        json_object_object_add(psJsonResult, JSON_TYPE,json_object_new_int(E_SS_COMMAND_DOOR_LOCK_GET_USER_RESPONSE));
        json_object_object_add(psJsonResult, JSON_SEQUENCE,json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonResult, JSON_MAC,json_object_new_int64((int64_t)u64DeviceAddress));

        tsDoorLockUser sUserHeader = {0};
        eZigbeeSqliteDelDoorLockRetrieveUserList(&sUserHeader);
        tsDoorLockUser *Temp = NULL;
        dl_list_for_each(Temp, &sUserHeader.list, tsDoorLockUser, list){
            struct json_object *psJosnTemp = json_object_new_object();
            json_object_object_add(psJosnTemp, JSON_ID,json_object_new_int(Temp->u8UserID));
            json_object_object_add(psJosnTemp, JSON_TYPE,json_object_new_int(Temp->eUserType));
            json_object_object_add(psJosnTemp, JSON_PERM,json_object_new_int(Temp->eUserPerm));
            json_object_array_add(psJsonArray, psJosnTemp);
        }
        eZigbeeSqliteDoorLockRetrieveUserListFree(&sUserHeader);

        json_object_object_add(psJsonResult, JSON_USER,psJsonArray);
        DBG_vPrintln(DBG_SOCKET, "psJsonResult %s, length is %d\n",
                     json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketFd,
                      json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            ERR_vPrintln(T_TRUE, "send data to client error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);

        return E_SS_OK;
    }
    return E_SS_INCORRECT_PARAMETERS;
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teSS_Status eSocketServer_Init(void)
{ 
    signal(SIGPIPE, SIG_IGN);//ingnore signal interference

    memset(&sSocketServer, 0, sizeof(sSocketServer));
    for(int i = 0; i < NUMBER_SOCKET_CLIENT; i++){//init client socket fd
        memset(&ClientSocket[i], 0, sizeof(tsClientSocket));
        ClientSocket[i].iSocketClient = -1;
    }

    teSS_Status SStatus = E_SS_OK;
    if(-1 == (sSocketServer.iSocketFd = socket(AF_INET, SOCK_STREAM, 0))) {
        ERR_vPrintln(T_TRUE,"open socket failed");
        SStatus = E_SS_ERROR_SOCKET;
        return SStatus;
    }

    struct sockaddr_in server_addr;  
    server_addr.sin_family      = AF_INET;  
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*receive any address*/
    server_addr.sin_port        = htons(SOCKET_SERVER_PORT);

    int on = 1;  /*端口复用*/
    if((setsockopt(sSocketServer.iSocketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))<0) {
        ERR_vPrintln(T_TRUE,"set socket option failed, %s\n", strerror(errno));
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_SOCKET;
        return SStatus;
    }  

    if(-1 == bind(sSocketServer.iSocketFd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        ERR_vPrintln(T_TRUE,"bind error! %s\n", strerror(errno));
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_BIND;
        return SStatus;
    }

    if(-1 == listen(sSocketServer.iSocketFd, 5)) {
        PERR_vPrintln("listen error!");
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_LISTEN;
        return SStatus;
    }

    eQueueCreate(&sSocketServer.sQueue, NUM_SOCKET_QUEUE);
    DBG_vPrintln(DBG_SOCKET, "Create pvSocketServerThread\n");
    sSocketServer.sThreadSocket.pvThreadData = &sSocketServer;
    CHECK_RESULT(eThreadStart(pvSocketServerThread, &sSocketServer.sThreadSocket, E_THREAD_DETACHED), E_THREAD_OK, E_SS_ERROR);
    
    DBG_vPrintln(DBG_SOCKET, "Create pvSocketCallbackHandlerThread\n");
    sSocketServer.sThreadQueue.pvThreadData = &sSocketServer;
    CHECK_RESULT(eThreadStart(pvSocketCallbackHandlerThread, &sSocketServer.sThreadQueue, E_THREAD_DETACHED), E_THREAD_OK, E_SS_ERROR);
    
    return SStatus;
}

teSS_Status eSocketServer_Destroy(void)
{
    DBG_vPrintln(DBG_SOCKET, "eSocketServer_Destroy\n");
    eThreadStop(&sSocketServer.sThreadSocket);
    eThreadStop(&sSocketServer.sThreadQueue);
    eQueueDestroy(&sSocketServer.sQueue);
    
    return E_SS_OK;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

