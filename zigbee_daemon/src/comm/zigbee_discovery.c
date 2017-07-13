/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          Discovery interface
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
#include "zigbee_discovery.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_DISC (verbosity >= 3 )
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void *pvDiscoveryHandleThread(void *psThreadInfoVoid);
static teDiscStatus eZigbeeDiscoverySocketInit(int iPort, tsZigbeeDiscovery *psZigbeeDiscovery);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
tsZigbeeDiscovery sZigbeeDiscovery;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teDiscStatus eZigbeeDiscoveryInit()
{
    memset(&sZigbeeDiscovery, 0, sizeof(sZigbeeDiscovery));
    eZigbeeDiscoverySocketInit(BROADCAST_PORT, &sZigbeeDiscovery);

    sZigbeeDiscovery.sThreadDiscovery.pvThreadData = &sZigbeeDiscovery;  
    CHECK_RESULT(eThreadStart(pvDiscoveryHandleThread, &sZigbeeDiscovery.sThreadDiscovery, E_THREAD_DETACHED), E_THREAD_OK, E_DISCOVERY_ERROR);

    return E_DISCOVERY_OK;
}

teDiscStatus eZigbeeDiscoveryFinished()
{
    DBG_vPrintln(DBG_DISC, "eZigbeeDiscoveryFinished\n");
    eThreadStop(&sZigbeeDiscovery.sThreadDiscovery);
    return E_DISCOVERY_OK;
}

teDiscStatus eZigbeeDiscoveryBroadcastEvent(uint8 *pu8Msg, tsBroadcastEvent sEvent)
{
    DBG_vPrintln(DBG_DISC, "eZigbeeDiscoveryBrocastEvent:%d\n", sEvent);
    int iSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1 == iSocketFd)
    {
        ERR_vPrintln(T_TRUE, "socket create error %s\n", strerror(errno));
        return E_DISCOVERY_ERROR_CREATESOCK;
    }
    int on = 1;int broadcastEnable = 1;//the permissions of broadcast
    setsockopt(iSocketFd, SOL_SOCKET, SO_BROADCAST, (char *)&broadcastEnable, sizeof(broadcastEnable));
    setsockopt(iSocketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family  = AF_INET;
    server_addr.sin_port    = htons(BROADCAST_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    struct json_object *psJsonResult = NULL;
    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintln(T_TRUE, "json_object_new_object error\n");
        close(iSocketFd);
        return E_DISCOVERY_ERROR;
    }
    json_object_object_add(psJsonResult, "broadcast", json_object_new_int(sEvent));    
    DBG_vPrintln(DBG_DISC, "psJsonResult %s, length is %d\n",
            json_object_to_json_string(psJsonResult), (int)strlen(json_object_to_json_string(psJsonResult)));

    if(-1 == sendto(iSocketFd, json_object_to_json_string(psJsonResult), 
        (int)strlen(json_object_to_json_string(psJsonResult)), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        ERR_vPrintln(T_TRUE, "Brocast error %s\n", strerror(errno));
        json_object_put(psJsonResult);
        close(iSocketFd);
        return E_DISCOVERY_ERROR;
    }

    json_object_put(psJsonResult);
    close(iSocketFd);
    return E_DISCOVERY_OK;
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
static teDiscStatus eZigbeeDiscoverySocketInit(int iPort, tsZigbeeDiscovery *psZigbeeDiscovery)
{
    DBG_vPrintln(DBG_DISC, "IotcBroadcastSocketInit\n");

    if(-1 == (psZigbeeDiscovery->iSocketFd = socket(AF_INET, SOCK_DGRAM, 0)))
    {
        ERR_vPrintln(T_TRUE, "socket create error %s\n", strerror(errno));
        return E_DISCOVERY_ERROR_CREATESOCK;
    }
    
    int on = 1;  /*SO_REUSEADDR port can used twice by program */
    if((setsockopt(psZigbeeDiscovery->iSocketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))<0) 
    {  
        ERR_vPrintln(T_TRUE,"setsockopt failed, %s\n", strerror(errno));
        close(psZigbeeDiscovery->iSocketFd);
        return E_DISCOVERY_ERROR_SETSOCK;
    }  

    bzero(&psZigbeeDiscovery->server_addr,sizeof(psZigbeeDiscovery->server_addr));  
    psZigbeeDiscovery->server_addr.sin_family       =   AF_INET;  
    psZigbeeDiscovery->server_addr.sin_addr.s_addr  =   htonl(INADDR_ANY);  
    psZigbeeDiscovery->server_addr.sin_port         =   htons(iPort);  
    
    if(-1 == bind(psZigbeeDiscovery->iSocketFd, 
                    (struct sockaddr*)&psZigbeeDiscovery->server_addr, sizeof(psZigbeeDiscovery->server_addr)))
    {
        ERR_vPrintln(T_TRUE,"bind socket failed, %s\n", strerror(errno));
        close(psZigbeeDiscovery->iSocketFd);
        return E_DISCOVERY_ERROR_BIND;
    }

    DBG_vPrintln(DBG_DISC, "Create Socket Fd %d\n", psZigbeeDiscovery->iSocketFd);
    return E_DISCOVERY_OK;
}

static void *pvDiscoveryHandleThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsZigbeeDiscovery *psZigbeeDiscovery = (tsZigbeeDiscovery*)psThreadInfo->pvThreadData;
    psThreadInfo->eState = E_THREAD_RUNNING;

    int  iRecvLen = 0;
    char paRecvBuffer[MDBF] = {0};
    int  iAddrLen = sizeof(psZigbeeDiscovery->server_addr);
    while(psThreadInfo->eState == E_THREAD_RUNNING)
    {
        eThreadYield();
        if((iRecvLen = recvfrom(psZigbeeDiscovery->iSocketFd, paRecvBuffer, MDBF, 0, 
                    (struct sockaddr*)&psZigbeeDiscovery->server_addr,(socklen_t*)&iAddrLen)) > 0)
        {
            struct sockaddr_in *p = (struct sockaddr_in*)&psZigbeeDiscovery->server_addr;
            DBG_vPrintln(DBG_DISC, "Recv Data[%d]: %s, from %s\n", iRecvLen, paRecvBuffer, inet_ntoa(p->sin_addr));
            struct json_object *psJsonMessage = NULL;
            if(NULL != (psJsonMessage = json_tokener_parse((const char*)paRecvBuffer)))
            {
                struct json_object *psJsonTemp  = NULL;
                if(json_object_object_get_ex(psJsonMessage,"type", &psJsonTemp)){
                    uint16 u16Command = (uint16)json_object_get_int(psJsonTemp);
                    if(E_SS_COMMAND_DISCOVERY == u16Command){
                        char auResponse[MDBF] = {0};
                        snprintf(auResponse, sizeof(auResponse), "{\"port\":%d}", SOCKET_SERVER_PORT);
                        if(sendto(psZigbeeDiscovery->iSocketFd, auResponse, strlen(auResponse), 0, 
                                    (struct sockaddr*)&psZigbeeDiscovery->server_addr, sizeof(psZigbeeDiscovery->server_addr)) < 0)
                        {
                            ERR_vPrintln(T_TRUE, "Send Data Error!\n");
                        } else {
                            DBG_vPrintln(DBG_DISC, "Send Data: %s\n", auResponse);
                        }
                    }
                }
                json_object_put(psJsonMessage);//free json object's memory
            }
        }
        else
        {
            WAR_vPrintln(T_TRUE,"recvfrom message error:%s \n", strerror(errno));
        }
    }
    close(sZigbeeDiscovery.iSocketFd);

    DBG_vPrintln(DBG_DISC, "pvDiscoveryHandleThread Exit \n");
    vThreadFinish(psThreadInfo);
    return NULL;
}

