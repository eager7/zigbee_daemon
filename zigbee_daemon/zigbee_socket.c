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
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/un.h>


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

static void vResponseCommandError(int iSocketfd);
static void vResponseJsonFormatError(int iSocketfd);
static void vResponseFailed(int iSocketfd);
static void vResponseSuccess(int iSocketfd);

static teSS_Status eSocketHandleGetVersion(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetChannel(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandlePermitjoin(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleLeaveNetwork(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetAllDevicesList(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetLightOnOff(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetLightLevel(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetLightRGB(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetLightStatus(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetLightLevel(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetLightRGB(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleGetSensorValue(int iSocketfd, struct json_object *psJsonMessage);
static teSS_Status eSocketHandleSetClosuresState(int iSocketfd, struct json_object *psJsonMessage);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern int verbosity;
extern const char *Version;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static int iSequenceNumber = 0;
static tsSocketServer sSocketServer;
static tsClientSocket ClientSocket[NUMBER_SOCKET_CLIENT];

static tsSocketHandleMap sSocketHandleMap[] = {
    {E_SS_COMMAND_GETVERSION,               eSocketHandleGetVersion},
    {E_SS_COMMAND_PREMITJOIN,               eSocketHandlePermitjoin},
    {E_SS_COMMAND_LEAVE_NETWORK,            eSocketHandleLeaveNetwork},
    {E_SS_COMMAND_GET_CHANNEL,              eSocketHandleGetChannel},
        
    {E_SS_COMMAND_GET_DEVICES_LIST_ALL,     eSocketHandleGetAllDevicesList},
        
    {E_SS_COMMAND_LIGHT_SET_ON_OFF,         eSocketHandleSetLightOnOff},
    {E_SS_COMMAND_LIGHT_SET_LEVEL,          eSocketHandleSetLightLevel},
    {E_SS_COMMAND_LIGHT_SET_RGB,            eSocketHandleSetLightRGB},
    {E_SS_COMMAND_LIGHT_GET_STATUS,         eSocketHandleGetLightStatus},
    {E_SS_COMMAND_LIGHT_GET_LEVEL,          eSocketHandleGetLightLevel},
    {E_SS_COMMAND_LIGHT_GET_RGB,            eSocketHandleGetLightRGB},
        
    {E_SS_COMMAND_SENSOR_GET_SENSOR,        eSocketHandleGetSensorValue},
            
    {E_SS_COMMAND_SET_CLOSURES_STATE,       eSocketHandleSetClosuresState},
};
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teSS_Status eSocketServer_Init(void)
{ 
    signal(SIGPIPE, SIG_IGN);//ingnore signal interference

    memset(&sSocketServer, 0, sizeof(sSocketServer));
    for(int i = 0; i < NUMBER_SOCKET_CLIENT; i++)//init client socket fd
    {
        memset(&ClientSocket[i], 0, sizeof(tsClientSocket));
        ClientSocket[i].iSocketClient = -1;
    }

    teSS_Status SStatus = E_SS_OK;
    if(-1 == (sSocketServer.iSocketFd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        ERR_vPrintf(T_TRUE,"setsockopt failed");  
        SStatus = E_SS_ERROR_SOCKET;
        return SStatus;
    }

    struct sockaddr_in server_addr;  
    server_addr.sin_family      = AF_INET;  
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*receive any address*/
    server_addr.sin_port        = htons(SOCKET_SERVER_PORT);

    int on = 1;  
    if((setsockopt(sSocketServer.iSocketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))<0) 
    {  
        ERR_vPrintf(T_TRUE,"setsockopt failed, %s\n", strerror(errno)); 
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_SOCKET;
        return SStatus;
    }  

    if(-1 == bind(sSocketServer.iSocketFd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        ERR_vPrintf(T_TRUE,"bind error! %s\n", strerror(errno));
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_BIND;
        return SStatus;
    }

    if(-1 == listen(sSocketServer.iSocketFd, 5))
    {
        PERR_vPrintf("listen error!");
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_LISTEN;
        return SStatus;
    }

    eQueueCreate(&sSocketServer.sQueue, NUM_SOCKET_QUEUE);
    DBG_vPrintf(DBG_SOCKET, "Create pvSocketServerThread\n");
    sSocketServer.sThreadSocket.pvThreadData = &sSocketServer;
    CHECK_RESULT(eThreadStart(pvSocketServerThread, &sSocketServer.sThreadSocket, E_THREAD_DETACHED), E_THREAD_OK, E_SS_ERROR);
    
    DBG_vPrintf(DBG_SOCKET, "Create pvSocketCallbackHandlerThread\n");
    sSocketServer.sThreadQueue.pvThreadData = &sSocketServer;
    CHECK_RESULT(eThreadStart(pvSocketCallbackHandlerThread, &sSocketServer.sThreadQueue, E_THREAD_DETACHED), E_THREAD_OK, E_SS_ERROR);
    
    return E_SS_OK;
}

teSS_Status eSocketServer_Destroy(void)
{
    DBG_vPrintf(DBG_SOCKET, "eSocketServer_Destroy\n");
    eThreadStop(&sSocketServer.sThreadSocket);
    eThreadStop(&sSocketServer.sThreadQueue);
    eQueueDestroy(&sSocketServer.sQueue);
    
    return E_SS_OK;
}

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
    if(psSocketServer->iSocketFd > iListenFD)
    {
        iListenFD = psSocketServer->iSocketFd;
    }
    
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        fdTemp = fdSelect;  /* use temp value, because this value will be clear */
        int iResult = select(iListenFD + 1, &fdTemp, NULL, NULL, NULL);
        switch(iResult)
        {
            case 0:
                DBG_vPrintf(DBG_SOCKET, "receive message time out \n");
            break;

            case -1:
                WAR_vPrintf(T_TRUE,"receive message error:%s \n", strerror(errno));
            break;

            default:
            {
                if(FD_ISSET(psSocketServer->iSocketFd, &fdTemp))//there is client accept
                {
                    DBG_vPrintf(DBG_SOCKET, "A client connecting... \n");
                    if(psSocketServer->u8NumClientSocket >= 5){
                        DBG_vPrintf(DBG_SOCKET, "Client already connected 5, don't allow connected\n");  
                        FD_CLR(psSocketServer->iSocketFd, &fdSelect);//delete this Server from select set
                        break;
                    }
                    for(int i = 0; i < NUMBER_SOCKET_CLIENT; i++)
                    {
                        if(ClientSocket[i].iSocketClient == -1){
                            ClientSocket[i].iSocketClient = accept(psSocketServer->iSocketFd, (struct sockaddr *)&ClientSocket[i].addrclint, (socklen_t *) &ClientSocket[i].u16Length);
                            if(-1 == ClientSocket[i].iSocketClient){
                                ERR_vPrintf(T_TRUE,"accept client connecting error \n");   
                                break;
                            } else {                              
                                DBG_vPrintf(DBG_SOCKET, "Client (%d-%d) already connected\n", i, ClientSocket[i].iSocketClient);  
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
                            ClientSocket[i].u16Length = recv(ClientSocket[i].iSocketClient, ClientSocket[i].auClientData, sizeof(ClientSocket[i].auClientData), 0);
                            if (0 == ClientSocket[i].u16Length){
                                ERR_vPrintf(T_TRUE, "Can't recv client[%d] message, close it\n", i);  
                                close(ClientSocket[i].iSocketClient);
                                FD_SET(psSocketServer->iSocketFd, &fdSelect);//Add socketserver fd into select fd
                                FD_CLR(ClientSocket[i].iSocketClient, &fdSelect);//delete this client from select set
                                ClientSocket[i].iSocketClient = -1;
                                psSocketServer->u8NumClientSocket --;
                            } else {
                                tsSSCallbackThreadData *psSCallBackThreadData = NULL;
                                psSCallBackThreadData = (tsSSCallbackThreadData*)malloc(sizeof(tsSSCallbackThreadData));
                                if (!psSCallBackThreadData){
                                    ERR_vPrintf(T_TRUE, "Memory allocation failure");
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
    
    DBG_vPrintf(DBG_SOCKET, "pvSocketServerThread Exit\n");
    vThreadFinish(psThreadInfo)/* Return from thread clearing resources */;
    return NULL;
}

static void *pvSocketCallbackHandlerThread(void *psThreadInfoVoid)
{
    DBG_vPrintf(DBG_SOCKET, "pvSocketCallbackHandlerThread\n");
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
            if(json_object_object_get_ex(psJsonMessage,"command", &psJsonTemp))
            {
                eSocketCommand = json_object_get_int(psJsonTemp);
                if(json_object_object_get_ex(psJsonMessage,"sequence", &psJsonTemp))
                {
                    iSequenceNumber = json_object_get_int(psJsonTemp);
                    for(int i = 0; i < sizeof(sSocketHandleMap)/sizeof(tsSocketHandleMap); i++)
                    {
                        if(eSocketCommand == sSocketHandleMap[i].eSocketCommand)
                        {
                            teSS_Status eStatus = sSocketHandleMap[i].preMessageHandlePacket(psCallbackData->iSocketClientfd, psJsonMessage);
                            if(E_SS_ERROR_JSON_FORMAT == eStatus){
                                vResponseJsonFormatError(psCallbackData->iSocketClientfd);
                            } else if(E_SS_ERROR == eStatus){
                                vResponseFailed(psCallbackData->iSocketClientfd);
                            }
                        }
                    }
                }
            }
            else
            {
                vResponseCommandError(psCallbackData->iSocketClientfd);
            }
            json_object_put(psJsonMessage);//free json object's memory
        }
        else
        {
            ERR_vPrintf(T_TRUE, "ResponseJsonFormatError error\n");
            vResponseJsonFormatError(psCallbackData->iSocketClientfd);
        }
        FREE(psCallbackData);
        eThreadYield();
    }
    DBG_vPrintf(DBG_SOCKET, "pvSocketCallbackHandlerThread Exit\n");   
    vThreadFinish(psThreadInfo);

    return NULL;    
}
//////////////////////////////////////Message Handle////////////////////////////////////////
static void vResponseSuccess(int iSocketfd)
{
    DBG_vPrintf(DBG_SOCKET, "Handle message success\n");

    struct json_object* psJsonMessage = json_object_new_object();
    if(NULL == psJsonMessage)
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return ;
    }
    json_object_object_add(psJsonMessage, "status",json_object_new_int(E_SS_OK));
    json_object_object_add(psJsonMessage, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonMessage, "description",json_object_new_string("success"));

    if(-1 == send(iSocketfd, 
            json_object_get_string(psJsonMessage),strlen(json_object_get_string(psJsonMessage)),0))
    {
        ERR_vPrintf(T_TRUE, "send data to client error\n");
    }     
    json_object_put(psJsonMessage);
    return ;
}

static void vResponseFailed(int iSocketfd)
{
    struct json_object* psJsonMessage = json_object_new_object();
    if(NULL == psJsonMessage)
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return ;
    }
    json_object_object_add(psJsonMessage, "status",json_object_new_int(E_SS_ERROR));
    json_object_object_add(psJsonMessage, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonMessage, "description",json_object_new_string("failed"));

    if(-1 == send(iSocketfd, 
            json_object_get_string(psJsonMessage),strlen(json_object_get_string(psJsonMessage)),0))
    {
        ERR_vPrintf(T_TRUE, "send data to client error\n");
    }     
    json_object_put(psJsonMessage);
    return ;
}

static void vResponseJsonFormatError(int iSocketfd)
{
    struct json_object* psJsonMessage = json_object_new_object();
    if(NULL == psJsonMessage)
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return ;
    }
    json_object_object_add(psJsonMessage, "status",json_object_new_int(E_SS_ERROR_JSON_FORMAT));
    json_object_object_add(psJsonMessage, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonMessage, "description",json_object_new_string("nvalid format"));

    if(-1 == send(iSocketfd, 
            json_object_get_string(psJsonMessage),strlen(json_object_get_string(psJsonMessage)),0))
    {
        ERR_vPrintf(T_TRUE, "send data to client error\n");
    }     
    json_object_put(psJsonMessage);
    return;
}

static void vResponseCommandError(int iSocketfd)
{
    struct json_object* psJsonMessage = json_object_new_object();
    if(NULL == psJsonMessage)
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return ;
    }
    json_object_object_add(psJsonMessage, "status",json_object_new_int(E_SS_ERROR_NO_COMMAND));
    json_object_object_add(psJsonMessage, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonMessage, "description",json_object_new_string("can't find command"));

    if(-1 == send(iSocketfd, 
            json_object_get_string(psJsonMessage),strlen(json_object_get_string(psJsonMessage)),0))
    {
        ERR_vPrintf(T_TRUE, "send data to client error\n");
    }     
    json_object_put(psJsonMessage);
    return;
}

static teSS_Status eSocketHandleGetVersion(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request is get version\n");
    teSS_Status eSS_Status = E_SS_OK;
    struct json_object* psJsonReturn = json_object_new_object();
    if(NULL == psJsonReturn)
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return E_SS_ERROR;
    }
    json_object_object_add(psJsonReturn, "status",json_object_new_int(E_SS_OK));
    json_object_object_add(psJsonReturn, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonReturn, "description",json_object_new_string(Version));
    INF_vPrintf(DBG_SOCKET, "return message is ----%s\n",json_object_get_string(psJsonReturn));
    if(-1 == send(iSocketfd, 
        json_object_get_string(psJsonReturn),strlen(json_object_get_string(psJsonReturn)), 0))
    {
        ERR_vPrintf(T_TRUE, "send data to client error\n");
        eSS_Status = E_SS_ERROR;
    }
    json_object_put(psJsonReturn);
    return eSS_Status;
}

static teSS_Status eSocketHandleGetChannel(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request is get coordiantor channel\n");
    teSS_Status eSS_Status = E_SS_ERROR;

    uint8 u8Channel = 0;
    if((NULL != sControlBridge.Method.preCoordinatorGetChannel)&&
        (E_ZB_OK == sControlBridge.Method.preCoordinatorGetChannel(&u8Channel))){
        struct json_object* psJsonReturn = json_object_new_object();
        if(NULL == psJsonReturn)
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonReturn, "status",json_object_new_int(E_SS_OK));
        json_object_object_add(psJsonReturn, "sequence",json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonReturn, "description",json_object_new_int(u8Channel));
        INF_vPrintf(DBG_SOCKET, "return message is ----%s\n",json_object_get_string(psJsonReturn));
        if(-1 == send(iSocketfd, 
            json_object_get_string(psJsonReturn),strlen(json_object_get_string(psJsonReturn)), 0))
        {
            ERR_vPrintf(T_TRUE, "send data to client error\n");
        }
        json_object_put(psJsonReturn);
        return E_SS_OK;
    }

    return eSS_Status;
}

static teSS_Status eSocketHandlePermitjoin(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request open zigbee network\n");
    json_object *psJsonTemp = NULL;
    if(json_object_object_get_ex(psJsonMessage,"time", &psJsonTemp))
    {
        uint8 uiPermitjoinTime = json_object_get_int(psJsonTemp);
        CALL(sControlBridge.Method.preCoordinatorPermitJoin, uiPermitjoinTime);
        vResponseSuccess(iSocketfd);
        return E_SS_OK;
    }
    ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
    return E_SS_ERROR;
}

static teSS_Status eSocketHandleLeaveNetwork(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request leave a device\n");
    json_object *psJsonTemp = NULL, *psJsonRejoin = NULL, *psJsonChildren = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonTemp)&&
        json_object_object_get_ex(psJsonMessage,"rejoin", &psJsonRejoin)&&
        json_object_object_get_ex(psJsonMessage,"remove_children", &psJsonChildren))
    {
        uint8 u8Rejoin = json_object_get_int(psJsonRejoin);
        uint8 u8RemoveChildren = json_object_get_int(psJsonChildren);
        uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceRemoveNetwork) || 
            (E_ZB_OK != psZigbeeNode->Method.preDeviceRemoveNetwork(&psZigbeeNode->sNode, u8Rejoin, u8RemoveChildren)))
        {
            ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceRemoveNetwork error\n");
            return E_SS_ERROR;
        }
        vResponseSuccess(iSocketfd);
        return E_SS_OK;
    }
    return E_SS_ERROR_JSON_FORMAT;
}


static teSS_Status eSocketHandleGetAllDevicesList(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request is get online devices' message\n");
    
    struct json_object *psJsonDevice, *psJsonDevicesArray, *psJsonResult = NULL;
    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return E_SS_ERROR;
    }
    json_object_object_add(psJsonResult, "status",json_object_new_int(SUCCESS)); 
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
    
    if(NULL == (psJsonDevicesArray = json_object_new_array()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_array error\n");
        json_object_put(psJsonResult);
        return E_SS_ERROR;
    }
    
    tsZigbeeBase psZigbeeNode, *psZigbeeItem = NULL;
    memset(&psZigbeeNode, 0, sizeof(psZigbeeNode));
    eZigbeeSqliteRetrieveDevicesList(&psZigbeeNode);
    dl_list_for_each(psZigbeeItem, &psZigbeeNode.list, tsZigbeeBase, list)
    {
        psJsonDevice = NULL;
        if(NULL == (psJsonDevice = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            json_object_put(psJsonDevicesArray);
            eZigbeeSqliteRetrieveDevicesListFree(&psZigbeeNode);
            return E_SS_ERROR;
        }
                    
        json_object_object_add(psJsonDevice,"device_name",json_object_new_string((const char*)psZigbeeItem->auDeviceName)); 
        json_object_object_add(psJsonDevice,"device_id",(json_object_new_int((psZigbeeItem->u16DeviceID)))); 
        json_object_object_add(psJsonDevice,"device_online",json_object_new_int((psZigbeeItem->u8DeviceOnline))); 
        json_object_object_add(psJsonDevice,"device_mac_address",json_object_new_int64((psZigbeeItem->u64IEEEAddress))); 
        json_object_array_add(psJsonDevicesArray, psJsonDevice);
    }
    eZigbeeSqliteRetrieveDevicesListFree(&psZigbeeNode);
    json_object_object_add(psJsonResult,"description",psJsonDevicesArray); 

    INF_vPrintf(DBG_SOCKET, "return message is ----%s\n",json_object_to_json_string(psJsonResult));
    if(-1 == send(iSocketfd, 
            json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
    {
        ERR_vPrintf(T_TRUE, "send data to client error\n");
        json_object_put(psJsonResult);
        return E_SS_ERROR;
    }
    
    json_object_put(psJsonResult);
    return E_SS_OK;
}

static teSS_Status eSocketHandleSetLightOnOff(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request open light on\n");

    json_object *psJsonAddr, *psJsonGroup, *psJsonMode = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr) && 
       json_object_object_get_ex(psJsonMessage,"group_id", &psJsonGroup) &&
       json_object_object_get_ex(psJsonMessage,"mode", &psJsonMode))
    {
        uint8  u8Mode = json_object_get_int(psJsonMode);
        uint16 u16GroupID = json_object_get_int(psJsonGroup);
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceSetOnOff))
        {
            ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceSetOnOff error\n");
            return E_SS_ERROR;
        }
        if(
           (E_ZB_OK != psZigbeeNode->Method.preDeviceSetOnOff(&psZigbeeNode->sNode, u16GroupID, u8Mode)))
            {
                ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceSetOnOff error\n");
                return E_SS_ERROR;
            }
        vResponseSuccess(iSocketfd);
        return E_SS_OK;
    }

    return E_SS_ERROR_JSON_FORMAT;
}

static teSS_Status eSocketHandleSetLightLevel(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request set light level\n");
    
    json_object *psJsonAddr, *psJsonGroup, *psJsonLevel = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr) &&
      json_object_object_get_ex(psJsonMessage,"group_id", &psJsonGroup) &&
      json_object_object_get_ex(psJsonMessage,"light_level", &psJsonLevel))
    {
        uint8 u8Level = json_object_get_int(psJsonLevel);
        uint16 u16GroupID = json_object_get_int(psJsonGroup);
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode)||(NULL == psZigbeeNode->Method.preDeviceSetLevel)||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceSetLevel(&psZigbeeNode->sNode, u16GroupID, u8Level, 5)))
        {
            ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.preDeviceSetLevel error\n");
            return E_SS_ERROR;
        }
        vResponseSuccess(iSocketfd);
        return E_SS_OK;
    }
    return E_SS_ERROR_JSON_FORMAT;
}

static teSS_Status eSocketHandleSetLightRGB(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request set light rgb\n");
    json_object *psJsonAddr, *psJsonGroup, *psJsonRgb, *psJsonR, *psJsonG, *psJsonB = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,"group_id", &psJsonGroup)&&
       json_object_object_get_ex(psJsonMessage,"rgb_value", &psJsonRgb)&&
        json_object_object_get_ex(psJsonRgb,"R", &psJsonR)&&
        json_object_object_get_ex(psJsonRgb,"G", &psJsonG)&&
        json_object_object_get_ex(psJsonRgb,"B", &psJsonB))
    {
        uint16 u16GroupID = json_object_get_int(psJsonGroup);
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);

        tsRGB sRGB;
        sRGB.R = json_object_get_int(psJsonR);
        sRGB.G = json_object_get_int(psJsonG);
        sRGB.B = json_object_get_int(psJsonB);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode)||(NULL == psZigbeeNode->Method.preDeviceSetLightColour)||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceSetLightColour(&psZigbeeNode->sNode, u16GroupID, sRGB, 5)))
        {
            ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.preDeviceSetLightColour error\n");
            return E_SS_ERROR;
        }
        vResponseSuccess(iSocketfd);
        return E_SS_OK;
    }
    
    return E_SS_ERROR_JSON_FORMAT;
}

static teSS_Status eSocketHandleSetClosuresState(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request set closure device state\n");
    json_object *psJsonAddr = NULL, *psJsonOperator = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr)&&
       json_object_object_get_ex(psJsonMessage,"operator", &psJsonOperator))
    {
        uint8 u8Operator = json_object_get_int(psJsonOperator);
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);
        
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if((NULL == psZigbeeNode)||(NULL == psZigbeeNode->Method.preDeviceSetWindowCovering)||
           (E_ZB_OK != psZigbeeNode->Method.preDeviceSetWindowCovering(&psZigbeeNode->sNode, u8Operator)))
        {
            ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.preDeviceSetWindowCovering error\n");
            return E_SS_ERROR;
        }
        vResponseSuccess(iSocketfd);
        return E_SS_OK;
    }
    
    return E_SS_ERROR_JSON_FORMAT;
}


static teSS_Status eSocketHandleGetLightLevel(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request get light level\n");
    json_object *psJsonAddr = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr))
    {
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        uint8 u8level = 0;
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceGetLevel) || 
            (E_ZB_OK != psZigbeeNode->Method.preDeviceGetLevel(&psZigbeeNode->sNode, &u8level)))
        {
            ERR_vPrintf(T_TRUE, "preDeviceGetLevel callback failed\n");
            return E_SS_ERROR;
        }
        struct json_object *psJsonResult, *psJsonLevel = NULL;
        if(NULL == (psJsonResult = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        if(NULL == (psJsonLevel = json_object_new_object()))
        {
            json_object_put(psJsonResult);
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonLevel, "light_level", json_object_new_int(u8level));    
        json_object_object_add(psJsonResult, "status", json_object_new_int(SUCCESS));    
        json_object_object_add(psJsonResult, "sequence", json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonResult, "description", psJsonLevel); 
        DBG_vPrintf(DBG_SOCKET, "psJsonResult %s, length is %d\n", 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketfd, 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            json_object_put(psJsonResult);
            ERR_vPrintf(T_TRUE, "send data to client error\n");
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_ERROR_JSON_FORMAT;
}

static teSS_Status eSocketHandleGetLightStatus(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request get light status\n");
    json_object *psJsonAddr = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr))
    {
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        uint8 u8mode = 0;
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceGetLevel) || 
            (E_ZB_OK != psZigbeeNode->Method.preDeviceGetOnOff(&psZigbeeNode->sNode, &u8mode)))
        {
            ERR_vPrintf(T_TRUE, "preDeviceGetOnOff callback failed\n");
            return E_SS_ERROR;
        }
        
        struct json_object *psJsonResult, *psJsonStatus = NULL;
        if(NULL == (psJsonResult = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        if(NULL == (psJsonStatus = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonStatus, "light_status",json_object_new_int(u8mode));    
        json_object_object_add(psJsonResult, "status",json_object_new_int(SUCCESS));    
        json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonResult, "description",psJsonStatus); 

        DBG_vPrintf(DBG_SOCKET, "psJsonResult %s, length is %d\n", 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketfd, 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            json_object_put(psJsonResult);
            ERR_vPrintf(T_TRUE, "send data to client error\n");
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_ERROR_JSON_FORMAT;
}

static teSS_Status eSocketHandleGetLightRGB(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request get light rgb\n");
    json_object *psJsonAddr = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr))
    {
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        tsRGB sRGB;
        if((NULL == psZigbeeNode) || (NULL == psZigbeeNode->Method.preDeviceGetLevel) || 
            (E_ZB_OK != psZigbeeNode->Method.preDeviceGetLightColour(&psZigbeeNode->sNode, &sRGB)))
        {
            ERR_vPrintf(T_TRUE, "preDeviceGetOnOff callback failed\n");
            return E_SS_ERROR;
        }

        struct json_object *psJsonResult, *psJsonRGB, *psJsonRGBValue = NULL;
        if(NULL == (psJsonResult = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        if(NULL == (psJsonRGB = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        if(NULL == (psJsonRGBValue = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            json_object_put(psJsonRGB);
            return E_SS_ERROR;
        }
        json_object_object_add(psJsonRGBValue, "R",json_object_new_int(sRGB.R));    
        json_object_object_add(psJsonRGBValue, "G",json_object_new_int(sRGB.G));    
        json_object_object_add(psJsonRGBValue, "B",json_object_new_int(sRGB.B));    
        json_object_object_add(psJsonRGB, "light_rgb",psJsonRGBValue);    
        
        json_object_object_add(psJsonResult,"status",json_object_new_int(SUCCESS));    
        json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonResult,"description",psJsonRGB); 
        DBG_vPrintf(DBG_SOCKET, "psJsonResult %s, length is %d\n", 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketfd, 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            ERR_vPrintf(T_TRUE, "send data to client error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_ERROR_JSON_FORMAT;
}

static teSS_Status eSocketHandleGetSensorValue(int iSocketfd, struct json_object *psJsonMessage)
{
    INF_vPrintf(DBG_SOCKET, "Client request get sensor value\n");
    json_object *psJsonAddr = NULL, *psJsonSensor = NULL;
    if(json_object_object_get_ex(psJsonMessage,"device_address", &psJsonAddr) &&
       json_object_object_get_ex(psJsonMessage,"sensor_type", &psJsonSensor))
    {
        teZigbee_ClusterID eSensorType = (teZigbee_ClusterID)json_object_get_int(psJsonSensor);
        uint64 u64DeviceAddress = json_object_get_int64(psJsonAddr);
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64DeviceAddress);
        if(NULL == psZigbeeNode)
        {
            ERR_vPrintf(T_TRUE, "preDeviceGetOnOff callback failed\n");
            return E_SS_ERROR;
        }
        uint16 u16SensorValue = 0;
        if((NULL == psZigbeeNode->Method.preDeviceGetSensorValue)||
            (E_ZB_OK != psZigbeeNode->Method.preDeviceGetSensorValue(&psZigbeeNode->sNode, &u16SensorValue, eSensorType)))
        {
            ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.preDeviceGetSensorValue error\n");
            return E_SS_ERROR;
        }        

        struct json_object *psJsonResult, *psJsonValue = NULL;
        if(NULL == (psJsonResult = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            return E_SS_ERROR;
        }
        if(NULL == (psJsonValue = json_object_new_object()))
        {
            ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        switch(eSensorType)
        {
           case(E_ZB_CLUSTERID_TEMPERATURE):
            json_object_object_add(psJsonValue,"temp",json_object_new_int(u16SensorValue));  
           break;
           case(E_ZB_CLUSTERID_HUMIDITY):
               json_object_object_add(psJsonValue,"humi",json_object_new_int(u16SensorValue));  
           break;
           case(E_ZB_CLUSTERID_BINARY_INPUT_BASIC):
               json_object_object_add(psJsonValue,"simple",json_object_new_int(u16SensorValue));  
           break;
           case(E_ZB_CLUSTERID_POWER):
               json_object_object_add(psJsonValue,"power",json_object_new_int(u16SensorValue));  
           break;
           case(E_ZB_CLUSTERID_ILLUMINANCE):
               json_object_object_add(psJsonValue,"illu",json_object_new_int(u16SensorValue));  
           break;
           default:break;
        }
        json_object_object_add(psJsonResult,"status",json_object_new_int(SUCCESS));    
        json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
        json_object_object_add(psJsonResult,"description",psJsonValue); 

        DBG_vPrintf(DBG_SOCKET, "psJsonResult %s, length is %d\n", 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));
        if(-1 == send(iSocketfd, 
                json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)),0))
        {
            ERR_vPrintf(T_TRUE, "send data to client error\n");
            json_object_put(psJsonResult);
            return E_SS_ERROR;
        }
        json_object_put(psJsonResult);
        return E_SS_OK;
    }
    return E_SS_ERROR_JSON_FORMAT;
}



/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

