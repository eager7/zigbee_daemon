/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          Cloud interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2016-12-22 15:13:17 +0100 (Fri, 12 Dec 2016 $
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
#include "zigbee_cloud.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_CLOUD (verbosity > 1 )
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void *pvCloudHandleThread(void *psThreadInfoVoid);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const char *pauServerSubAddr = "tcp://"CLOUD_ADDRESS":"CLOUD_SUB_PORT;
const char *pauServerPushAddr = "tcp://"CLOUD_ADDRESS":"CLOUD_PUSH_PORT;
tsZigbeeCloud sZigbeeCloud;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teCloudStatus eZigbeeCloudInit()
{
    memset(&sZigbeeCloud, 0, sizeof(sZigbeeCloud));
    sZigbeeCloud.pvContext = zmq_ctx_new();
    sZigbeeCloud.pvSender = zmq_socket(sZigbeeCloud.pvContext, ZMQ_PUSH);
    CHECK_STATUS(zmq_bind(sZigbeeCloud.pvSender, pauServerPushAddr), 0, E_CLOUD_ERROR);
    sZigbeeCloud.pvSubscriber = zmq_socket(sZigbeeCloud.pvContext, ZMQ_SUB);
    CHECK_STATUS(zmq_connect(sZigbeeCloud.pvSubscriber, pauServerSubAddr), 0, E_CLOUD_ERROR);
    CHECK_STATUS(zmq_setsockopt(sZigbeeCloud.pvSubscriber, ZMQ_SUBSCRIBE, NULL, 0), 0, E_CLOUD_ERROR);
    
    sZigbeeCloud.sThreadCloud.pvThreadData = &sZigbeeCloud;  
    CHECK_RESULT(eThreadStart(pvCloudHandleThread, &sZigbeeCloud.sThreadCloud, E_THREAD_DETACHED), E_THREAD_OK, E_CLOUD_ERROR);

    return E_CLOUD_OK;
}

teCloudStatus eZigbeeCloudFinished()
{
    eThreadStop(&sZigbeeCloud.sThreadCloud);
    zmq_close(sZigbeeCloud.pvSender);
    zmq_close(sZigbeeCloud.pvSubscriber);
    zmq_ctx_destroy(sZigbeeCloud.pvContext);
    return E_CLOUD_OK;
}

teCloudStatus eZigbeeCloudPush(const char *pmsg, uint16 u16Length)
{
    if(-1 == zmq_send(sZigbeeCloud.pvSender, pmsg, u16Length, ZMQ_DONTWAIT)){
        ERR_vPrintf(T_TRUE, "zmq send error\n");
        return E_CLOUD_ERROR;
    }
    return E_CLOUD_OK;
}

teCloudStatus eCloudPushAllDevicesList(void)
{
    struct json_object *psJsonDevice, *psJsonDevicesArray, *psJsonResult = NULL;
    if(NULL == (psJsonResult = json_object_new_object()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_object error\n");
        return E_CLOUD_ERROR;
    }
    json_object_object_add(psJsonResult, "status",json_object_new_int(SUCCESS)); 
    json_object_object_add(psJsonResult, "sequence",json_object_new_int(0));
    
    if(NULL == (psJsonDevicesArray = json_object_new_array()))
    {
        ERR_vPrintf(T_TRUE, "json_object_new_array error\n");
        json_object_put(psJsonResult);
        return E_CLOUD_ERROR;
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
            return E_CLOUD_ERROR;
        }
                    
        json_object_object_add(psJsonDevice,"device_name",json_object_new_string((const char*)psZigbeeItem->auDeviceName)); 
        json_object_object_add(psJsonDevice,"device_id",(json_object_new_int((psZigbeeItem->u16DeviceID)))); 
        json_object_object_add(psJsonDevice,"device_online",json_object_new_int((psZigbeeItem->u8DeviceOnline))); 
        json_object_object_add(psJsonDevice,"device_mac_address",json_object_new_int64((psZigbeeItem->u64IEEEAddress))); 
        json_object_array_add(psJsonDevicesArray, psJsonDevice);
    }
    eZigbeeSqliteRetrieveDevicesListFree(&psZigbeeNode);
    json_object_object_add(psJsonResult,"description",psJsonDevicesArray); 

    INF_vPrintf(DBG_CLOUD, "return message is ----%s\n",json_object_to_json_string(psJsonResult));
    eZigbeeCloudPush(json_object_to_json_string(psJsonResult), strlen(json_object_to_json_string(psJsonResult)));   
    json_object_put(psJsonResult);
    return E_SS_OK;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
static void *pvCloudHandleThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsZigbeeCloud *psZigbeeCloud = (tsZigbeeCloud*)psThreadInfo->pvThreadData;
    psThreadInfo->eState = E_THREAD_RUNNING;

    char auSubBuf[MABF] = {0};
    while(psThreadInfo->eState == E_THREAD_RUNNING)
    {
        DBG_vPrintf(DBG_CLOUD, "pvCloudHandleThread Recv \n");
        
        int ret = zmq_recv(psZigbeeCloud->pvSubscriber, auSubBuf, sizeof(auSubBuf), 0);
        if(ret != -1){
            NOT_vPrintf(DBG_CLOUD, "Server Pub Msg:%s\n", auSubBuf);
        } else {
            WAR_vPrintf(T_TRUE, "zmq_recv message error:%s\n", strerror(errno));
            continue;
        }
    }

    DBG_vPrintf(DBG_CLOUD, "pvCloudHandleThread Exit \n");
    vThreadFinish(psThreadInfo);
    return NULL;
}





