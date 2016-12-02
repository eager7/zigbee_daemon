/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          SocketServer interface
 *
 * REVISION:           $Revision: 43420 $
 *
 * DATED:              $Date: 2015-10-01 15:13:17 +0100 (Mon, 18 Jun 2012) $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright Tonly B.V. 2015. All rights reserved
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


#include "zigbee_socket_server.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
//#define NEW_FRAM
#define NUM_QUEUE 2
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


static void *pvSocketServerThread(void *psThreadInfoVoid);
static void MessageHandlePacket(int iSocketfd, uint8 u8Command, struct json_object *psJsonMessage);

static void *pvSocketCallbackHandlerThread(void *psThreadInfoVoid);
static json_object *MessageHandleGetDevicesList(void);
static teSS_Status MessageHandlePermitjoin(uint8 uitime);
static teSS_Status MessageHandleLeaveNetwork(uint64 u64Address);
static teSS_Status MessageHandleLightOnOff(uint64 u64Address, uint16 u16groupid, uint8 u8mode);
static json_object * MessageHandleGetLightStatus(uint64 u64Address);
static teSS_Status MessageHandleSetLightLevel(uint64 u64Address, uint16 u16groupid, uint8 u8level);
static struct json_object* MessageHandleGetLightLevel(uint64 u64Address);
static teSS_Status MessageHandleSetLightRGB(uint64 u64Address, uint16 u16GroupID, uint32 u32HueSatTarget);
static json_object * MessageHandleGetLightRGB(uint64 u64Address);
static json_object * MessageHandleGetSensorValue(uint64 u64Address, teSensorType u8Type);
static json_object * MessageHandleGetSensorAlarm(uint64 u64Address, teSensorType u8Type);
static json_object * MessageHandleGetSensorAllAlarm(void);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern int verbosity;
extern const char *Version;
static int iSequenceNumber = 0;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

static tsSocketServer sSocketServer;
static tsQueue  sQueueMsg;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


teSS_Status eSocketServer_Init(void)
{ 
    DBG_vPrintf(verbosity, "eScketServer_Init\n");
    
    teSS_Status SStatus = E_SS_OK;
    signal(SIGPIPE, SIG_IGN);//ingnore signal interference

    if(-1 == (sSocketServer.iSocketFd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        ERR_vPrintf(T_TRUE,"setsockopt failed");  
        SStatus = E_SS_ERROR_SOCKET;
        return SStatus;
    }

    struct sockaddr_in server_addr;  
    server_addr.sin_family      = AF_INET;  
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(SOCKET_SERVER_PORT);

    DBG_vPrintf(verbosity, "Set server socket option\n");
    int on = 1;  
    if((setsockopt(sSocketServer.iSocketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))<0) 
    {  
        ERR_vPrintf(T_TRUE,"setsockopt failed, %s\n", strerror(errno)); 
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_SOCKET;
        return SStatus;
    }  

    DBG_vPrintf(verbosity, "bind socket to port (%d)\n", SOCKET_SERVER_PORT);
    if(-1 == bind(sSocketServer.iSocketFd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        ERR_vPrintf(T_TRUE,"bind error! %s\n", strerror(errno));
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_BIND;
        return SStatus;
    }

    DBG_vPrintf(verbosity, "Start listen from client\n");
    if(-1 == listen(sSocketServer.iSocketFd, 5))
    {
        ERR_vPrintf(T_TRUE,"listen error! %s\n", strerror(errno));
        close(sSocketServer.iSocketFd);
        SStatus = E_SS_ERROR_LISTEN;
        return SStatus;
    }
    
    sSocketServer.sSocketServer.pvThreadData = &sSocketServer;

    DBG_vPrintf(verbosity, "Create pvSocketServerThread\n");
    if (mThreadStart(pvSocketServerThread, &sSocketServer.sSocketServer, E_THREAD_JOINABLE) != E_THREAD_OK)
    {
        ERR_vPrintf(T_TRUE, "Failed to start socket reader thread\n");
        return E_SS_ERROR;
    }
    
    DBG_vPrintf(verbosity, "Create pvSocketCallbackHandlerThread\n");
    memset(&sQueueMsg, 0, sizeof(sQueueMsg));
    mQueueCreate(&sQueueMsg, NUM_QUEUE);
    if (mThreadStart(pvSocketCallbackHandlerThread, &sSocketServer.sSocketQueue, E_THREAD_DETACHED) != E_THREAD_OK)
    {
        ERR_vPrintf(T_TRUE, "Failed to start handler thread\n");
        return E_SS_ERROR;
    }
    
    return E_SS_OK;
}


teSS_Status eSocketServer_Destroy(void)
{
    DBG_vPrintf(verbosity, "eSocketServer_Destroy\n");
    mThreadStop(&sSocketServer.sSocketServer);
    mQueueDestroy(&sQueueMsg);
    
    return E_SS_OK;
}



/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
static void ResponseSuccess(int iSocketfd)
{
    DBG_vPrintf(verbosity, "Handle message success\n");

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

static void ResponseFailed(int iSocketfd)
{
    ERR_vPrintf(T_TRUE, "Handle message failed\n");
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

static void ResponseJsonFormatError(int iSocketfd)
{
    ERR_vPrintf(T_TRUE, "This is not a vaild json format\n");
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

static void ResponseMessageError(int iSocketfd)
{
    ERR_vPrintf(T_TRUE, " message error\n");
    struct json_object* psJsonMessage = json_object_new_object();
    if(NULL == psJsonMessage)
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return ;
    }
    json_object_object_add(psJsonMessage, "status",json_object_new_int(E_SS_ERROR_MESSAGE));
    json_object_object_add(psJsonMessage, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonMessage, "description",json_object_new_string("message format error"));

    if(-1 == send(iSocketfd, 
            json_object_get_string(psJsonMessage),strlen(json_object_get_string(psJsonMessage)),0))
    {
        ERR_vPrintf(T_TRUE, "send data to client error\n");
    }     
    json_object_put(psJsonMessage);
    return ;
}

static void ResponseCommandError(int iSocketfd)
{
    ERR_vPrintf(T_TRUE, "can't find command\n");
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



static void *pvSocketServerThread(void *psThreadInfoVoid)
{
    uint8 i = 0;
    static uint8 u8NumConnSocket = 0;
    int iListenFD = 0;
    tsSSCallbackThreadData *psSCallBackThreadData = NULL;
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSocketServer *psSocketServer = (tsSocketServer*)psThreadInfo->pvThreadData;
    
    DBG_vPrintf(verbosity, "pvSocketServerThread Starting\n");
    
    psThreadInfo->eState = E_THREAD_RUNNING;

    tsClientSocket ClientSocket[NUMBER_SOCKET_CLIENT];
    for(i = 0; i < NUMBER_SOCKET_CLIENT; i++)//init client socket fd
    {
        memset(&ClientSocket[i], 0, sizeof(tsClientSocket));
        ClientSocket[i].iSocketClient = -1;
    }
    

    fd_set fdSelect, fdTemp;
    FD_ZERO(&fdSelect);//Init fd
    FD_SET(psSocketServer->iSocketFd, &fdSelect);//Add socketserver fd into select fd
    if(psSocketServer->iSocketFd > iListenFD)
        iListenFD = psSocketServer->iSocketFd;
    
    sleep(5);//waiting other component run
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        fdTemp = fdSelect;
        //DBG_vPrintf(verbosity, "wait select change... \n");
        int iResult = select(iListenFD + 1, &fdTemp, NULL, NULL, NULL);
        switch(iResult)
        {
            case 0:
            DBG_vPrintf(verbosity, "receive message time out \n");
            break;

            case -1:
            ERR_vPrintf(T_TRUE,"receive message error \n");
            break;

            default:
            {
                //DBG_vPrintf(verbosity, "select change \n");
                if(FD_ISSET(psSocketServer->iSocketFd, &fdTemp))//there is client accept
                {
                    DBG_vPrintf(verbosity, "A client connecting... \n");
                    if(u8NumConnSocket >= 5)
                    {
                        DBG_vPrintf(verbosity, "Client already connected 5, don't allow connected\n");  
                        sleep(1);
                        FD_CLR(psSocketServer->iSocketFd, &fdSelect);//delete this Server from select set
                        break;
                    }
                    for(i = 0; i < NUMBER_SOCKET_CLIENT; i++)
                    {
                        if(ClientSocket[i].iSocketClient == -1)
                        {
                            ClientSocket[i].iSocketClient = accept(psSocketServer->iSocketFd, 
                                                    (struct sockaddr *)&ClientSocket[i].addrclint, (socklen_t *) &ClientSocket[i].u16Length);
                            
                            if(-1 == ClientSocket[i].iSocketClient)
                            {
                                ERR_vPrintf(T_TRUE,"accept client connecting error \n");   
                                break;
                            }
                            else
                            {                              
                                DBG_vPrintf(verbosity, "Client (%d-%d) already connected\n", i, ClientSocket[i].iSocketClient);  
                                FD_SET(ClientSocket[i].iSocketClient, &fdSelect);
                                
                                if(ClientSocket[i].iSocketClient > iListenFD)
                                    iListenFD = ClientSocket[i].iSocketClient;
                                u8NumConnSocket ++;
                                break;
                            }
                        }
                    }
                }

                for(i = 0; i < NUMBER_SOCKET_CLIENT; i++)//there is client communication
                {
                    if(-1 == ClientSocket[i].iSocketClient)//connect already been disconnect
                    {
                        continue;
                    }

                    if(FD_ISSET(ClientSocket[i].iSocketClient, &fdTemp))
                    {
                        //DBG_vPrintf(verbosity, "Client (%d) begin recv data\n", i);  
                        memset(ClientSocket[i].sClientData, 0, sizeof(ClientSocket[i].sClientData));
                        ClientSocket[i].u16Length = recv(ClientSocket[i].iSocketClient, 
                                ClientSocket[i].sClientData, sizeof(ClientSocket[i].sClientData), 0);
                        if (0 == ClientSocket[i].u16Length)
                        {
                            ERR_vPrintf(T_TRUE, "Can't recv client[%d] message, close it\n", i);  
                            close(ClientSocket[i].iSocketClient);
                            FD_SET(psSocketServer->iSocketFd, &fdSelect);//Add socketserver fd into select fd
                            FD_CLR(ClientSocket[i].iSocketClient, &fdSelect);//delete this client from select set
                            ClientSocket[i].iSocketClient = -1;
                            u8NumConnSocket --;
                        }
                        else
                        {
                            psSCallBackThreadData = (tsSSCallbackThreadData*)malloc(sizeof(tsSSCallbackThreadData));
                            if (!psSCallBackThreadData)
                            {
                                ERR_vPrintf(T_TRUE, "Memory allocation failure");
                                break;
                            }
                            memset(psSCallBackThreadData, 0, sizeof(tsSSCallbackThreadData));  

                            psSCallBackThreadData->u16Length = ClientSocket[i].u16Length;
                            psSCallBackThreadData->iSocketClientfd = ClientSocket[i].iSocketClient;
                            memcpy(psSCallBackThreadData->au8Message, ClientSocket[i].sClientData, sizeof(psSCallBackThreadData->au8Message));

                            mQueueEnqueue(&sQueueMsg, psSCallBackThreadData);
                        }
                    }
                }
            }
            break;
        }
    }
    
    DBG_vPrintf(verbosity, "Exit\n");
    /* Return from thread clearing resources */
    mThreadFinish(psThreadInfo);
    return NULL;
}

static void *pvSocketCallbackHandlerThread(void *psThreadInfoVoid)
{
    DBG_vPrintf(verbosity, "pvSocketCallbackHandlerThread\n");
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    psThreadInfo->eState = E_THREAD_RUNNING;
    tsSSCallbackThreadData *psCallbackData = NULL;

    while(psThreadInfo->eState == E_THREAD_RUNNING)
    {
        mQueueDequeue(&sQueueMsg, (void **) &psCallbackData);

        teSocketCommand eSocketCommand;
        struct json_object *psJsonMessage = NULL;
        if(NULL != (psJsonMessage = json_tokener_parse((const char*)psCallbackData->au8Message)))
        {
            struct json_object *psJsonTemp  = NULL;
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"command")))
            {
                eSocketCommand = json_object_get_int(psJsonTemp);
                if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"sequence")))
                {
                    iSequenceNumber = json_object_get_int(psJsonTemp);
                    MessageHandlePacket(psCallbackData->iSocketClientfd, eSocketCommand, psJsonMessage);
                }
            }
            else
            {
                ResponseCommandError(psCallbackData->iSocketClientfd);
            }
            json_object_put(psJsonMessage);//free json object's memory
        }
        else
        {
            ERR_vPrintf(T_TRUE, "ResponseJsonFormatError error\n");
            ResponseJsonFormatError(psCallbackData->iSocketClientfd);
        }

        if(NULL != psCallbackData)
        {
            free(psCallbackData);
            psCallbackData = NULL;
        }
        mThreadYield();
    }

    DBG_vPrintf(verbosity, "Exit\n");
    
    /* Return from thread clearing resources */
    mThreadFinish(psThreadInfo);

    return NULL;    
}


static void MessageHandlePacket(int iSocketfd, uint8 u8Command, struct json_object *psJsonMessage)
{
    struct json_object* psJsonTemp, *psJsonReturn = NULL;
    switch(u8Command)
    {
        /***********************************************Coordinator***************************************************/
        case(E_SS_COMMAND_GETVERSION):
        {
            INF_vPrintf(verbosity, "Client request is get version\n");
            struct json_object* psJsonReturn = json_object_new_object();
            if(NULL == psJsonReturn)
            {
                ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
                return ;
            }
            json_object_object_add(psJsonReturn, "status",json_object_new_int(E_SS_OK));
            json_object_object_add(psJsonReturn, "sequence",json_object_new_int(iSequenceNumber));
            json_object_object_add(psJsonReturn, "description",json_object_new_string(Version));
            INF_vPrintf(verbosity, "return message is ----%s\n",json_object_get_string(psJsonReturn));
            if(-1 == send(iSocketfd, 
                json_object_get_string(psJsonReturn),strlen(json_object_get_string(psJsonReturn)), 0))
            {
                ERR_vPrintf(T_TRUE, "send data to client error\n");
            }
            json_object_put(psJsonReturn);
        }
        break;

        case(E_SS_COMMAND_GET_DEVICES_LIST_ONLINE):
        {
            INF_vPrintf(verbosity, "Client request is get online devices' message\n");
            if(NULL == (psJsonReturn = MessageHandleGetDevicesList()))
            {
                ERR_vPrintf(T_TRUE, "MessageHandleGetDevicesList error\n");
                ResponseFailed(iSocketfd);
                return;
            }
            
            INF_vPrintf(verbosity, "return message is ----%s\n",json_object_to_json_string(psJsonReturn));
            if(-1 == send(iSocketfd, 
                    json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)),0))
            {
                ERR_vPrintf(T_TRUE, "send data to client error\n");
            }
            json_object_put(psJsonReturn);
        }
        break;
        
        case(E_SS_COMMAND_PREMITJOIN):
        {
            INF_vPrintf(verbosity, "Client request open zigbee network\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"time")))
            {
                uint8 uiPermitjoinTime = json_object_get_int(psJsonTemp);
                if(E_SS_OK != MessageHandlePermitjoin(uiPermitjoinTime))
                {
                    ERR_vPrintf(T_TRUE, "MessageHandlePermitjoin error\n");
                    ResponseFailed(iSocketfd);
                    return;
                }
                else
                {
                    ResponseSuccess(iSocketfd);
                    return;
                }
            }
            else
            {
                ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
                ResponseMessageError(iSocketfd);
            }
        }
        break;
        /********************************************ZLL**********************************************/        
        case(E_SS_COMMAND_LIGHT_ON):
        {
            INF_vPrintf(verbosity, "Client request open light on\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                
                if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"group_id")))
                {
                    uint16 u16GroupID = json_object_get_int(psJsonTemp);
                    if(E_SS_OK != MessageHandleLightOnOff(u64DeviceAddress, u16GroupID, 1))
                    {
                        ERR_vPrintf(T_TRUE, "MessageHandleLightOnOff error\n");
                        ResponseFailed(iSocketfd);
                        return;
                    }
                    else
                    {
                        ResponseSuccess(iSocketfd);
                        return;
                    }
                }
            }
            ERR_vPrintf(T_TRUE, "ResponseMessageError error, cant't get device_address\n");
            ResponseMessageError(iSocketfd);
        }
        break;
        
        case(E_SS_COMMAND_LIGHT_OFF):
        {
            INF_vPrintf(verbosity, "Client request set light off\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                
                if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"group_id")))
                {
                    uint16 u16GroupID = json_object_get_int(psJsonTemp);
                    if(E_SS_OK != MessageHandleLightOnOff(u64DeviceAddress, u16GroupID, 0))
                    {
                        ERR_vPrintf(T_TRUE, "MessageHandleLightOnOff error\n");
                        ResponseFailed(iSocketfd);
                        return;
                    }
                    else
                    {
                        ResponseSuccess(iSocketfd);
                        return;
                    }
                }
            }
            ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
            ResponseMessageError(iSocketfd);
        }
        break;                
        
        case(E_SS_COMMAND_LIGHT_SET_LEVEL):
        {
            INF_vPrintf(verbosity, "Client request set light level\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                
                if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"group_id")))
                {
                    uint16 u16GroupID = json_object_get_int(psJsonTemp);
                    if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"light_level")))
                    {
                        uint8 u8Level = json_object_get_int(psJsonTemp);
                        if(E_SS_OK != MessageHandleSetLightLevel(u64DeviceAddress, u16GroupID, u8Level))
                        {
                            ERR_vPrintf(T_TRUE, "MessageHandleSetLightLevel error\n");
                            ResponseFailed(iSocketfd);
                            return;
                        }
                        else
                        {
                            ResponseSuccess(iSocketfd);
                            return;
                        }
                    }
                }
            }
            ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
            ResponseMessageError(iSocketfd);
        }
        break;
        
        case(E_SS_COMMAND_LIGHT_GET_LEVEL):
        {
            INF_vPrintf(verbosity, "Client request get light level\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                if(NULL == (psJsonReturn = MessageHandleGetLightLevel(u64DeviceAddress)))
                {
                    ERR_vPrintf(T_TRUE, "MessageHandleGetLightLevel error\n");
                    ResponseFailed(iSocketfd);
                    return;
                }
                DBG_vPrintf(verbosity, "psJsonResult %s, length is %d\n", 
                        json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)));
                if(-1 == send(iSocketfd, 
                        json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)),0))
                {
                    ERR_vPrintf(T_TRUE, "send data to client error\n");
                }
                json_object_put(psJsonReturn);
                return;
            }
            else
            {
                ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
                ResponseMessageError(iSocketfd);
            }
        }
        break;
        
        case(E_SS_COMMAND_LIGHT_GET_STATUS):
        {
            INF_vPrintf(verbosity, "Client request get light status\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                
                if(NULL == (psJsonReturn = MessageHandleGetLightStatus(u64DeviceAddress)))
                {
                    ERR_vPrintf(T_TRUE, "MessageHandleGetLightStatus error\n");
                    ResponseFailed(iSocketfd);
                    return;
                }
                DBG_vPrintf(verbosity, "psJsonResult %s, length is %d\n", 
                        json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)));
                if(-1 == send(iSocketfd, 
                        json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)),0))
                {
                    ERR_vPrintf(T_TRUE, "send data to client error\n");
                }
                json_object_put(psJsonReturn);
                return;
            }
            else
            {
                ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
                ResponseMessageError(iSocketfd);
            }
        }
        break;
        
        case(E_SS_COMMAND_LIGHT_SET_RGB):
        {
            INF_vPrintf(verbosity, "Client request set light rgb\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                
                if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"group_id")))
                {
                    uint16 u16GroupID = json_object_get_int(psJsonTemp);

                    if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"rgb_value")))
                    {
                        uint32 u32RgbValue = json_object_get_int(psJsonTemp);
                        if(E_SS_OK != MessageHandleSetLightRGB(u64DeviceAddress, u16GroupID, u32RgbValue))
                        {
                            ERR_vPrintf(T_TRUE, "MessageHandleSetLightRGB error\n");
                            ResponseFailed(iSocketfd);
                            return;
                        }
                        else
                        {
                            ResponseSuccess(iSocketfd);
                            return;
                        }
                    }
                }
            }
            ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
            ResponseMessageError(iSocketfd);
        }
        break;
        
        case(E_SS_COMMAND_LIGHT_GET_RGB):
        {
            INF_vPrintf(verbosity, "Client request get light rgb\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                if(NULL == (psJsonReturn = MessageHandleGetLightRGB(u64DeviceAddress)))
                {
                    ERR_vPrintf(T_TRUE, "MessageHandleGetLightRGB error\n");
                    ResponseFailed(iSocketfd);
                    return;
                }
                DBG_vPrintf(verbosity, "psJsonResult %s, length is %d\n", 
                        json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)));
                if(-1 == send(iSocketfd, 
                        json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)),0))
                {
                    ERR_vPrintf(T_TRUE, "send data to client error\n");
                }
                json_object_put(psJsonReturn);
                return;
            }
            ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
            ResponseMessageError(iSocketfd);
        }
        break;

        /*******************************************Sensor***********************************************/
        case(E_SS_COMMAND_SENSOR_GET_HUMI):
        case(E_SS_COMMAND_SENSOR_GET_SIMPLE):
        case(E_SS_COMMAND_SENSOR_GET_POWER):
        case(E_SS_COMMAND_SENSOR_GET_TEMP):
        case(E_SS_COMMAND_SENSOR_GET_ILLU):
        {
            INF_vPrintf(verbosity, "Client request get sensor value\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"sensor_type")))
                {
                    teSensorType SensorType = json_object_get_int(psJsonTemp);

                    if(NULL == (psJsonReturn = MessageHandleGetSensorValue(u64DeviceAddress, SensorType)))
                    {
                        ERR_vPrintf(T_TRUE, "MessageHandleGetSensorValue error\n");
                        ResponseFailed(iSocketfd);
                        return;
                    }
                    DBG_vPrintf(verbosity, "psJsonResult %s, length is %d\n", 
                            json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)));
                    if(-1 == send(iSocketfd, 
                            json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)),0))
                    {
                        ERR_vPrintf(T_TRUE, "send data to client error\n");
                    }
                    json_object_put(psJsonReturn);
                    return;
                }
            }
            ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
            ResponseMessageError(iSocketfd);            
        }
        break;
        
        case(E_SS_COMMAND_SENSOR_GET_ALARM):
        {
            INF_vPrintf(verbosity, "Client request get sensor alarm\n");
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"sensor_type")))
                {
                    teSensorType SensorType = json_object_get_int(psJsonTemp);

                    if(NULL == (psJsonReturn = MessageHandleGetSensorAlarm(u64DeviceAddress, SensorType)))
                    {
                        ERR_vPrintf(T_TRUE, "MessageHandleGetSensorAlarm error\n");
                        ResponseFailed(iSocketfd);
                        return;
                    }
                    DBG_vPrintf(verbosity, "psJsonResult %s, length is %d\n", 
                            json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)));
                    if(-1 == send(iSocketfd, 
                            json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)),0))
                    {
                        ERR_vPrintf(T_TRUE, "send data to client error\n");
                    }
                    json_object_put(psJsonReturn);
                    return;
                }
            }
            ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
            ResponseMessageError(iSocketfd);            
        }
        break;
        
        case(E_SS_COMMAND_SENSOR_GET_ALL_ALARM):
        {
            INF_vPrintf(verbosity, "Client request get sensor alarm\n");
            
            if(NULL == (psJsonReturn = MessageHandleGetSensorAllAlarm()))
            {
                ERR_vPrintf(T_TRUE, "MessageHandleGetSensorAllAlarm error\n");
                ResponseFailed(iSocketfd);
                return;
            }
            DBG_vPrintf(verbosity, "psJsonResult %s, length is %d\n", 
                    json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)));
            if(-1 == send(iSocketfd, 
                    json_object_to_json_string(psJsonReturn), (int)strlen(json_object_to_json_string(psJsonReturn)),0))
            {
                ERR_vPrintf(T_TRUE, "send data to client error\n");
            }
            json_object_put(psJsonReturn);

            ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
            ResponseMessageError(iSocketfd);            
        }
        break;
        
        case(E_SS_COMMAND_LEAVE_NETWORK):
        {
            INF_vPrintf(verbosity, "Client request leave a device\n");
            
            if(NULL != (psJsonTemp = json_object_object_get(psJsonMessage,"device_address")))
            {
                uint64 u64DeviceAddress = json_object_get_int64(psJsonTemp);
                if(E_SS_OK != MessageHandleLeaveNetwork(u64DeviceAddress))
                {
                    ERR_vPrintf(T_TRUE, "MessageHandleLeaveNetwork error\n");
                    ResponseFailed(iSocketfd);
                    return;
                }
                else
                {
                    ResponseSuccess(iSocketfd);
                    return;
                }
            }
            else
            {
                ERR_vPrintf(T_TRUE, "ResponseMessageError error\n");
                ResponseMessageError(iSocketfd);
            }
        }
        break;       

        default:
        {
            ResponseCommandError(iSocketfd);
        }
        break;
    }
    return;
}

static json_object *MessageHandleGetDevicesList(void)
{
    DBG_vPrintf(verbosity, "MessageHandleGetDevicesList\n");
    
    struct json_object *psJsonDevice, *psJsonDevicesArray, *psJsonResult = NULL;

    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }
    json_object_object_add(psJsonResult, "status",json_object_new_int(SUCCESS)); 
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
    
    if(NULL == (psJsonDevicesArray = json_object_new_array()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_array error\n");
        json_object_put(psJsonResult);
        return NULL;
    }
    
    mLockLock(&sZigbee_Network.mutex);
    tsZigbee_Node* tempNodes = &sZigbee_Network.sNodes;
    tempNodes = tempNodes->psNext;
    while(tempNodes != NULL)
    {
        if((tempNodes->device_name != NULL) && (tempNodes->u64IEEEAddress != 0))
        {
            psJsonDevice = NULL;
            if(NULL == (psJsonDevice = json_object_new_object()))
            {
                ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
                json_object_put(psJsonResult);
                json_object_put(psJsonDevicesArray);
                return NULL;
            }
                        
            json_object_object_add(psJsonDevice,"device_name",json_object_new_string((const char*)tempNodes->device_name)); 
            json_object_object_add(psJsonDevice,"device_id",(json_object_new_int((tempNodes->u16DeviceID)))); 
            json_object_object_add(psJsonDevice,"device_mac_address",json_object_new_int64((tempNodes->u64IEEEAddress))); 
            json_object_array_add(psJsonDevicesArray,psJsonDevice);
            
            tempNodes = tempNodes->psNext;
        }
        else
        {
            tempNodes = tempNodes->psNext;
        }
    }
    mLockUnlock(&sZigbee_Network.mutex);

    json_object_object_add(psJsonResult,"description",psJsonDevicesArray); 
    
    return psJsonResult;
}

static teSS_Status MessageHandlePermitjoin(uint8 uitime)
{
    DBG_vPrintf(verbosity, "MessageHandlePermitjoin\n");
    teSS_Status SS_Status = E_SS_OK;

    if(NULL != sZigbee_Network.sNodes.Method.CoordinatorPermitJoin)
    {
        if(E_ZB_OK != sZigbee_Network.sNodes.Method.CoordinatorPermitJoin(uitime))
        {
            SS_Status = E_SS_ERROR;
        }
    }

    return SS_Status;
}

static teSS_Status MessageHandleLightOnOff(uint64 u64Address, uint16 u16groupid, uint8 u8mode)
{
    DBG_vPrintf(verbosity, "MessageHandleLightOnOff\n");
    teSS_Status SS_Status = E_SS_OK;

    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        if(NULL != ZigbeeNode->Method.DeviceSetOnOff)
        {
            if(E_ZB_OK != ZigbeeNode->Method.DeviceSetOnOff(ZigbeeNode, u16groupid, u8mode))
            {
                ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceSetOnOff error\n");
                SS_Status = E_SS_ERROR;
            }
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "psZigbee_FindNodeByIEEEAddress 0x%016llX error\n", u64Address);
        SS_Status = E_SS_ERROR;
    }

    return SS_Status;
}

static teSS_Status MessageHandleSetLightLevel(uint64 u64Address, uint16 u16groupid, uint8 u8level)
{
    DBG_vPrintf(verbosity, "MessageHandleSetLightLevel\n");
    teSS_Status SS_Status = E_SS_OK;

    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        if(NULL != ZigbeeNode->Method.DeviceSetLevel)
        {
            if(E_ZB_OK != ZigbeeNode->Method.DeviceSetLevel(ZigbeeNode, u16groupid, u8level, 5))
            {
                ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceSetLevel error\n");
                SS_Status = E_SS_ERROR;
            }
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "psZigbee_FindNodeByIEEEAddress error\n");
        SS_Status = E_SS_ERROR;
    }

    return SS_Status;
}

static struct json_object* MessageHandleGetLightLevel(uint64 u64Address)
{
    DBG_vPrintf(verbosity, "MessageHandleGetLightLevel\n");
    uint8 u8level = 0;
    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        if(NULL != ZigbeeNode->Method.DeviceGetLevel)
        {
            if(E_ZB_OK != ZigbeeNode->Method.DeviceGetLevel(ZigbeeNode, &u8level))
            {
                ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceGetLevel error\n");
                return NULL;
            }
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "psZigbee_FindNodeByIEEEAddress error\n");
        return NULL;
    }

    struct json_object *psJsonResult, *psJsonLevel = NULL;

    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }
    if(NULL == (psJsonLevel = json_object_new_object()))
    {
        json_object_put(psJsonResult);
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }

    json_object_object_add(psJsonLevel, "light_level", json_object_new_int(u8level));    
    json_object_object_add(psJsonResult, "status", json_object_new_int(SUCCESS));    
    json_object_object_add(psJsonResult, "sequence", json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonResult, "description", psJsonLevel); 

    return psJsonResult;
}

static json_object * MessageHandleGetLightStatus(uint64 u64Address)
{
    DBG_vPrintf(verbosity, "MessageHandleGetLightStatus\n");
    uint8 u8mode = 0;
    
    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        if(NULL != ZigbeeNode->Method.DeviceGetOnOff)
        {
            if(E_ZB_OK != ZigbeeNode->Method.DeviceGetOnOff(ZigbeeNode, &u8mode))
            {
                ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceGetOnOff error\n");
                return NULL;
            }
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "psZigbee_FindNodeByIEEEAddress error\n");
        return NULL;
    }

    struct json_object *psJsonResult, *psJsonStatus = NULL;

    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }
    if(NULL == (psJsonStatus = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        json_object_put(psJsonResult);
        return NULL;
    }
    
    json_object_object_add(psJsonStatus, "light_status",json_object_new_int(u8mode));    
    json_object_object_add(psJsonResult, "status",json_object_new_int(SUCCESS));    
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonResult, "description",psJsonStatus); 

    return psJsonResult;
}

static json_object * MessageHandleGetLightRGB(uint64 u64Address)
{
    DBG_vPrintf(verbosity, "MessageHandleGetLightRGB\n");
    uint32 u32HueSatTarget = 0;
    
    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        u32HueSatTarget = ZigbeeNode->u32DeviceRGB;
    }
    else
    {
        ERR_vPrintf(T_TRUE, "psZigbee_FindNodeByIEEEAddress error\n");
        return NULL;
    }

    struct json_object *psJsonResult, *psJsonRGB = NULL;

    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }
    if(NULL == (psJsonRGB = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        json_object_put(psJsonResult);
        return NULL;
    }

    json_object_object_add(psJsonRGB, "light_rgb",json_object_new_int(u32HueSatTarget));    
    json_object_object_add(psJsonResult,"status",json_object_new_int(SUCCESS));    
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonResult,"description",psJsonRGB); 

    return psJsonResult;
}

unsigned int Rgb2Hsv(unsigned int u32RGB)
{
    int iR = (u32RGB&0xff0000)>>16;
    int iG = (u32RGB&0xff00)>>8;
    int iB = u32RGB & 0xff;

    float H = 0;
    float R = (float)iR;
    float G = (float)iG;
    float B = (float)iB;

    float max = 0;
    max = (R>G)?R:G;
    max = (max>B)?max:B;

    float min = 0;
    min = (R>G)?G:R;
    min = (min>B)?B:min;

    printf("max = %f, min = %f\n", max, min);
    if(max == R)
    {
        H = (G-B)/(max-min);
    }
    else if(max == G)
    {
        H = 2 + (B-R)/(max-min);
    }
    else if(max == B)
    {
        H = 4 + (R-G)/(max - min);
    }

    H *= 60;
    if(H < 0)
    {
        H += 360;
    }
    printf("H = %f\n",H);

    float S = 0;
    if(0 == max)
    {
        S = 0;
    }
    else
    {
        S = (max - min)/max;	
    }
    printf("S = %f\n",S);
    float V = max/255;
    printf("V = %f\n",V);

    return ((unsigned int)H<<8)*10 | 0xff;
}


static teSS_Status MessageHandleSetLightRGB(uint64 u64Address, uint16 u16GroupID, uint32 u32HueSatTarget)
{
    DBG_vPrintf(verbosity, "MessageHandleSetLightRGB %u\n", u32HueSatTarget);
    teSS_Status SS_Status = E_SS_ERROR;

    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        if(NULL != ZigbeeNode->Method.DeviceSetLightColour)
        {
            ZigbeeNode->u32DeviceRGB = u32HueSatTarget;//store the value first
            
            u32HueSatTarget = Rgb2Hsv(ZigbeeNode->u32DeviceRGB);
            DBG_vPrintf(verbosity, "MessageHandleSetLight HSV %d\n", u32HueSatTarget);
            if(E_ZB_OK == ZigbeeNode->Method.DeviceSetLightColour(ZigbeeNode, u16GroupID, u32HueSatTarget, 1))
            {
                SS_Status = E_SS_OK;
            }
        }
    }
    
    return SS_Status;
}

static json_object * MessageHandleGetSensorValue(uint64 u64Address, teSensorType u8Type)
{
    DBG_vPrintf(verbosity, "MessageHandleGetSensorValue %d\n", u8Type);

    uint16 u16SensorValue = 0;
    struct json_object *psJsonResult, *psJsonValue = NULL;

    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }
    if(NULL == (psJsonValue = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        json_object_put(psJsonResult);
        return NULL;
    }
    
    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        switch(u8Type)
        {
            case(E_SENSOR_TEMP):
            {
                if(NULL != ZigbeeNode->Method.DeviceGetTemperature)
                {
                    DBG_vPrintf(verbosity, "ZigbeeNode->Method.DeviceGetTemperature\n");
                    if(E_ZB_OK != ZigbeeNode->Method.DeviceGetTemperature(ZigbeeNode, &u16SensorValue))
                    {
                        ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceGetTemperature error\n");
                        json_object_put(psJsonResult);
                        json_object_put(psJsonValue);
                        return NULL;
                    }
                    json_object_object_add(psJsonValue,"temp",json_object_new_int(u16SensorValue));  
                }
                else
                {
                    ERR_vPrintf(T_TRUE, "The Func is not existed\n");
                    json_object_put(psJsonResult);
                    json_object_put(psJsonValue);
                    return NULL;
                }
            }
            break;
            case(E_SENSOR_HUMI):
            {
                if(NULL != ZigbeeNode->Method.DeviceGetHumidity)
                {
                    if(E_ZB_OK != ZigbeeNode->Method.DeviceGetHumidity(ZigbeeNode, &u16SensorValue))
                    {
                        ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceGetHumidity error\n");
                        json_object_put(psJsonResult);
                        json_object_put(psJsonValue);
                        return NULL;
                    }
                    json_object_object_add(psJsonValue,"humi",json_object_new_int(u16SensorValue));  
                }
                else
                {
                    json_object_put(psJsonResult);
                    json_object_put(psJsonValue);
                    return NULL;
                }
            }
            break;
            case(E_SENSOR_SIMPLE):
            {
                if(NULL != ZigbeeNode->Method.DeviceGetSimple)
                {
                    if(E_ZB_OK != ZigbeeNode->Method.DeviceGetSimple(ZigbeeNode, &u16SensorValue))
                    {
                        ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceGetSimple error\n");
                        json_object_put(psJsonResult);
                        json_object_put(psJsonValue);
                        return NULL;
                    }
                    json_object_object_add(psJsonValue,"simple",json_object_new_int(u16SensorValue));  

                }
                else
                {
                    json_object_put(psJsonResult);
                    json_object_put(psJsonValue);
                    return NULL;
                }
            }
            break;
            case(E_SENSOR_POWER):
            {
                if(NULL != ZigbeeNode->Method.DeviceGetPower)
                {
                    if(E_ZB_OK != ZigbeeNode->Method.DeviceGetPower(ZigbeeNode, &u16SensorValue))
                    {
                        ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceGetPower error\n");
                        json_object_put(psJsonResult);
                        json_object_put(psJsonValue);
                        return NULL;
                    }
                    json_object_object_add(psJsonValue,"power",json_object_new_int(u16SensorValue));  
                }
                else
                {
                    json_object_put(psJsonResult);
                    json_object_put(psJsonValue);
                    return NULL;
                }
            }
            break;
            case(E_SENSOR_ILLU):
            {
                if(NULL != ZigbeeNode->Method.DeviceGetIlluminance)
                {
                    if(E_ZB_OK != ZigbeeNode->Method.DeviceGetIlluminance(ZigbeeNode, &u16SensorValue))
                    {
                        ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceGetIlluminance error\n");
                        json_object_put(psJsonResult);
                        json_object_put(psJsonValue);
                        return NULL;
                    }
                    json_object_object_add(psJsonValue,"illu",json_object_new_int(u16SensorValue));  
                }
                else
                {
                    json_object_put(psJsonResult);
                    json_object_put(psJsonValue);
                    return NULL;
                }
            }
            break;
            default:
                json_object_put(psJsonValue);
                json_object_put(psJsonResult);
                return NULL;
                break;
        }
                
    }
    else
    {
        json_object_put(psJsonResult);
        json_object_put(psJsonValue);
        return NULL;
    }

    json_object_object_add(psJsonResult,"status",json_object_new_int(SUCCESS));    
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonResult,"description",psJsonValue); 

    return psJsonResult;
}

static json_object * MessageHandleGetSensorAlarm(uint64 u64Address, teSensorType u8Type)
{
    DBG_vPrintf(verbosity, "MessageHandleGetSensorAlarm\n");
    struct json_object *psJsonResult, *psJsonValue = NULL;

    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }
    if(NULL == (psJsonValue = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        json_object_put(psJsonResult);
        return NULL;
    }
    
    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);
    
    if(NULL != ZigbeeNode)
    {
        mLockLock(&ZigbeeNode->mutex);
        switch(u8Type)
        {
            case(E_SENSOR_TEMP):
            {
                json_object_object_add(psJsonValue,"temp_alarm",json_object_new_int(ZigbeeNode->u8DeviceTempAlarm));  
            }
            break;
            case(E_SENSOR_HUMI):
            {
                json_object_object_add(psJsonValue,"humi_alarm",json_object_new_int(ZigbeeNode->u8DeviceHumiAlarm));  
            }
            break;
            case(E_SENSOR_SIMPLE):
            {
                json_object_object_add(psJsonValue,"simple_alarm",json_object_new_int(ZigbeeNode->u8DeviceSimpAlarm));  
            }
            break;
            default:
                json_object_put(psJsonValue);
                json_object_put(psJsonResult);
                return NULL;
                break;
        }
        mLockUnlock(&ZigbeeNode->mutex);                
    }
    else
    {
        json_object_put(psJsonValue);
        json_object_put(psJsonResult);
        ERR_vPrintf(T_TRUE, "psZigbee_FindNodeByIEEEAddress error\n");
        return NULL;
    }

    json_object_object_add(psJsonResult,"status",json_object_new_int(SUCCESS));    
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
    json_object_object_add(psJsonResult,"description",psJsonValue); 

    return psJsonResult;
}

static json_object * MessageHandleGetSensorAllAlarm(void)
{
    DBG_vPrintf(verbosity, "MessageHandleGetSensorAllAlarm\n");
    struct json_object *psJsonDevice, *psJsonDevicesArray, *psJsonResult = NULL;

    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return NULL;
    }
    json_object_object_add(psJsonResult,"status",json_object_new_int(SUCCESS)); 
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(iSequenceNumber));
    
    if(NULL == (psJsonDevicesArray = json_object_new_array()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_array error\n");
        json_object_put(psJsonResult);
        return NULL;
    }
    
    mLockLock(&sZigbee_Network.mutex);
    tsZigbee_Node* tempNodes = &sZigbee_Network.sNodes;
    while(tempNodes != NULL)
    {
        if((tempNodes->device_name != NULL) && (tempNodes->u64IEEEAddress != 0))
        {
            psJsonDevice = NULL;
            if(tempNodes->u16DeviceID == E_ZB_TEMPERATURE_SENSOR)
            {
                if(NULL == (psJsonDevice = json_object_new_object()))
                {
                    ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
                    json_object_put(psJsonResult);
                    json_object_put(psJsonDevicesArray);
                    return NULL;
                }
                            
                json_object_object_add(psJsonDevice,"device_name",json_object_new_string((const char*)tempNodes->device_name)); 
                json_object_object_add(psJsonDevice,"device_id",json_object_new_int(tempNodes->u16DeviceID)); 
                json_object_object_add(psJsonDevice,"temp_alarm",json_object_new_int(tempNodes->u8DeviceTempAlarm)); 
                json_object_object_add(psJsonDevice,"humi_alarm",json_object_new_int(tempNodes->u8DeviceHumiAlarm)); 
                json_object_object_add(psJsonDevice,"device_mac_address",json_object_new_int64(tempNodes->u64IEEEAddress)); 
                json_object_array_add(psJsonDevicesArray,psJsonDevice);
            }
            else if(tempNodes->u16DeviceID == E_ZB_SIMPLE_SENSOR)
            {
                if(NULL == (psJsonDevice = json_object_new_object()))
                {
                    ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
                    json_object_put(psJsonResult);
                    json_object_put(psJsonDevicesArray);
                    return NULL;
                }
                            
                json_object_object_add(psJsonDevice,"device_name",json_object_new_string((const char*)tempNodes->device_name)); 
                json_object_object_add(psJsonDevice,"device_id",json_object_new_int(tempNodes->u16DeviceID)); 
                json_object_object_add(psJsonDevice,"simple_alarm",json_object_new_int(tempNodes->u8DeviceSimpAlarm)); 
                json_object_object_add(psJsonDevice,"device_mac_address",json_object_new_int64(tempNodes->u64IEEEAddress)); 
                json_object_array_add(psJsonDevicesArray,psJsonDevice);
            }
            tempNodes = tempNodes->psNext;
        }
        else
        {
            tempNodes = tempNodes->psNext;
            continue;
        }
    }
    mLockUnlock(&sZigbee_Network.mutex);

    json_object_object_add(psJsonResult,"description",psJsonDevicesArray); 
    return psJsonResult;
}

static teSS_Status MessageHandleLeaveNetwork(uint64 u64Address)
{
    DBG_vPrintf(verbosity, "MessageHandleLightOnOff\n");
    teSS_Status SS_Status = E_SS_OK;

    tsZigbee_Node *ZigbeeNode = psZigbee_FindNodeByIEEEAddress(u64Address);

    if(NULL != ZigbeeNode)
    {
        if(NULL != ZigbeeNode->Method.DeviceRemoveNetwork)
        {
            if(E_ZB_OK != ZigbeeNode->Method.DeviceRemoveNetwork(ZigbeeNode))
            {
                ERR_vPrintf(T_TRUE, "ZigbeeNode->Method.DeviceRemoveNetwork error\n");
                SS_Status = E_SS_ERROR;
            }
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "psZigbee_FindNodeByIEEEAddress error\n");
        SS_Status = E_SS_ERROR;
    }

    return SS_Status;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

