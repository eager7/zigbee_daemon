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
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <termios.h>

#include "list.h"
#include "zigbee_control_bridge.h"
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
uint64          u64PanID    = CONFIG_DEFAULT_PANID;
teChannel       eChannel    = CONFIG_DEFAULT_CHANNEL;
teStartMode     eStartMode  = CONFIG_DEFAULT_START_MODE;

/* APS Ack enabled by default */
int bZCB_EnableAPSAck  = 1;

uint16 au16ProfileZLL =         E_ZB_PROFILEID_ZLL;
static uint16 au16ProfileHA =   E_ZB_PROFILEID_HA;
static uint16 au16Cluster[] = {
                                E_ZB_CLUSTERID_ONOFF,                   /*Light*/
                                E_ZB_CLUSTERID_DOOR_LOCK,
                              };

tsZigbeeNodes sControlBridge;
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void vZCB_HandleRemoveSceneResponse(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleRemoveGroupMembershipResponse(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleAddGroupResponse(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleAttributeReport(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleMatchDescriptorResponse(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleDeviceLeave(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleDeviceAnnounce(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleNetworkJoined(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleNodeCommandIDList(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleNodeClusterAttributeList(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleNodeClusterList(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleRestartFactoryNew(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleRestartProvisioned(void *pvUser, uint16 u16Length, void *pvMessage);
static void vZCB_HandleSimpleDescriptorResponse(void *pvUser, uint16 u16Length, void *pvMessage);
static teZbStatus eZCB_ConfigureControlBridge(void);
static void vZCB_AddNodeIntoNetwork(uint16 u16ShortAddress, uint64 u64IEEEAddress, uint8 u8MacCapability);
static void vZCB_InitZigbeeNodeInfo(tsZigbeeNodes *psZigbeeNode, uint16 u16DeviceID);
static void vZCB_HandleDoorLockControllerRequest(void *pvUser, uint16 u16Length, void *pvMessage);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eZCB_Init(char *cpSerialDevice, uint32 u32BaudRate)
{
    memset(&sControlBridge, 0, sizeof(tsZigbeeBase));
    dl_list_init(&sControlBridge.list);
    CHECK_STATUS(eLockCreate(&sControlBridge.mutex), E_THREAD_OK, E_ZB_ERROR); /* Lock Control bridge when add or delete the list of nodes */
    CHECK_RESULT(eSL_Init(cpSerialDevice, u32BaudRate), E_SL_OK, E_ZB_ERROR);
    /* Register listeners */
    eSL_AddListener(E_SL_MSG_NODE_CLUSTER_LIST,             vZCB_HandleNodeClusterList,               NULL);
    eSL_AddListener(E_SL_MSG_NODE_ATTRIBUTE_LIST,           vZCB_HandleNodeClusterAttributeList,      NULL);
    eSL_AddListener(E_SL_MSG_NODE_COMMAND_ID_LIST,          vZCB_HandleNodeCommandIDList,             NULL);
    eSL_AddListener(E_SL_MSG_NETWORK_JOINED_FORMED,         vZCB_HandleNetworkJoined,                 NULL);
    eSL_AddListener(E_SL_MSG_DEVICE_ANNOUNCE,               vZCB_HandleDeviceAnnounce,                NULL);
    eSL_AddListener(E_SL_MSG_LEAVE_INDICATION,              vZCB_HandleDeviceLeave,                   NULL);
    eSL_AddListener(E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE,     vZCB_HandleMatchDescriptorResponse,       NULL);
    eSL_AddListener(E_SL_MSG_REPORT_IND_ATTR_RESPONSE,      vZCB_HandleAttributeReport,               NULL);
    eSL_AddListener(E_SL_MSG_ADD_GROUP_RESPONSE,            vZCB_HandleAddGroupResponse,              NULL);
    eSL_AddListener(E_SL_MSG_REMOVE_GROUP_RESPONSE,         vZCB_HandleRemoveGroupMembershipResponse, NULL);
    eSL_AddListener(E_SL_MSG_REMOVE_SCENE_RESPONSE,         vZCB_HandleRemoveSceneResponse,           NULL);
    eSL_AddListener(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE,    vZCB_HandleSimpleDescriptorResponse,      NULL);
    eSL_AddListener(E_SL_MSG_LOCK_UNLOCK_DOOR_UPDATE,       vZCB_HandleDoorLockControllerRequest,     NULL);
    //eSL_AddListener(E_SL_MSG_VERSION_LIST,                  vZCB_HandleVersionResponse,    NULL);
    //eSL_AddListener(E_SL_MSG_GET_PERMIT_JOIN_RESPONSE,      vZCB_HandleGetPermitResponse,  NULL);
    eSL_AddListener(E_SL_MSG_NODE_NON_FACTORY_NEW_RESTART,  vZCB_HandleRestartProvisioned, NULL);
    eSL_AddListener(E_SL_MSG_NODE_FACTORY_NEW_RESTART,      vZCB_HandleRestartFactoryNew,  NULL);
    //eSL_AddListener(E_SL_MSG_STORE_SCENE_RESPONSE,        vZCB_HandleStoreSceneResponse,       NULL);
    //eSL_AddListener(E_SL_MSG_READ_ATTRIBUTE_RESPONSE,     vZCB_HandleReadAttrResp,             NULL);
    //eSL_AddListener(E_SL_MSG_ACTIVE_ENDPOINT_RESPONSE,    vZDActiveEndPointResp,               NULL);
    //eSL_AddListener(E_SL_MSG_ROUTE_DISCOVERY_CONFIRM,                         vZCB_HandleLog,                      NULL);
    

    return E_ZB_OK;
}

teZbStatus eZCB_Finish(void)
{
    eSL_Destroy();
    return E_ZB_OK;
}

teZbStatus eZCB_EstablishComms(void)
{
    if (eSL_SendMessage(E_SL_MSG_GET_VERSION, 0, NULL, NULL) == E_SL_OK) {
        uint16 u16Length;
        uint32  *u32Version;
        
        /* Wait 300ms for the versions message to arrive */
        if (eSL_MessageWait(E_SL_MSG_VERSION_LIST, 300, &u16Length, (void**)&u32Version) == E_SL_OK) {
            uint32 version = ntohl(*u32Version);
            
            DBG_vPrintf(DBG_ZCB, "Connected to control bridge version 0x%08x\n", version ); 
            free(u32Version);
            
            DBG_vPrintf(DBG_ZCB, "Reset control bridge\n");
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
    struct _PermitJoiningMessage
    {
        uint16    u16TargetAddress;
        uint8     u8Interval;
        uint8     u8TCSignificance;
    } PACKED sPermitJoiningMessage;
    
    DBG_vPrintf(DBG_ZCB, "Permit joining (%d) \n", u8Interval);
    
    sPermitJoiningMessage.u16TargetAddress  = htons(E_ZB_BROADCAST_ADDRESS_ROUTERS);
    sPermitJoiningMessage.u8Interval        = u8Interval;
    sPermitJoiningMessage.u8TCSignificance  = 0;
    
    if (eSL_SendMessage(E_SL_MSG_PERMIT_JOINING_REQUEST, sizeof(struct _PermitJoiningMessage), &sPermitJoiningMessage, NULL) != E_SL_OK)
    {
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
        if (eSL_MessageWait(E_SL_MSG_CHANNEL_RESPONE, 300, &u16Length, (void**)&pu8chan) == E_SL_OK) {
            *pu8Channel = *pu8chan;
            DBG_vPrintf(DBG_ZCB, "the devices' channel is %d\n", *pu8Channel ); 
        } else {
            ERR_vPrintf(T_TRUE, "No response to channel request\n");
            return E_ZB_ERROR;
        }
    }
    return E_ZB_OK;
}

teZbStatus eZCB_MatchDescriptorRequest(uint16 u16TargetAddress, uint16 u16ProfileID, uint8 u8NumInputClusters, 
    uint16 *pau16InputClusters,  uint8 u8NumOutputClusters, uint16 *pau16OutputClusters,uint8 *pu8SequenceNo)
{
    uint8 au8Buffer[256];
    uint16 u16Position = 0;
    int i;
    
    DBG_vPrintf(DBG_ZCB, "Send Match Desciptor request for profile ID 0x%04X to 0x%04X\n", u16ProfileID, u16TargetAddress);

    u16TargetAddress = htons(u16TargetAddress);
    memcpy(&au8Buffer[u16Position], &u16TargetAddress, sizeof(uint16));
    u16Position += sizeof(uint16);
    
    u16ProfileID = htons(u16ProfileID);
    memcpy(&au8Buffer[u16Position], &u16ProfileID, sizeof(uint16));
    u16Position += sizeof(uint16);
    
    au8Buffer[u16Position] = u8NumInputClusters;
    u16Position++;
    
    DBG_vPrintf(DBG_ZCB, "  Input Cluster List:\n");
    
    for (i = 0; i < u8NumInputClusters; i++)
    {
        uint16 u16ClusterID = htons(pau16InputClusters[i]);
        DBG_vPrintf(DBG_ZCB, "    0x%04X\n", pau16InputClusters[i]);
        memcpy(&au8Buffer[u16Position], &u16ClusterID , sizeof(uint16));
        u16Position += sizeof(uint16);
    }
    
    DBG_vPrintf(DBG_ZCB, "  Output Cluster List:\n");
    
    au8Buffer[u16Position] = u8NumOutputClusters;
    u16Position++;
    
    for (i = 0; i < u8NumOutputClusters; i++)
    {
        uint16 u16ClusterID = htons(pau16OutputClusters[i] );
        DBG_vPrintf(DBG_ZCB, "    0x%04X\n", pau16OutputClusters[i]);
        memcpy(&au8Buffer[u16Position], &u16ClusterID , sizeof(uint16));
        u16Position += sizeof(uint16);
    }

    if (eSL_SendMessage(E_SL_MSG_MATCH_DESCRIPTOR_REQUEST, u16Position, au8Buffer, pu8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }

    return E_ZB_OK;
}

teZbStatus eZCB_IEEEAddressRequest(tsZigbeeBase *psZigbee_Node)
{
    struct _IEEEAddressRequest
    {
        uint16    u16TargetAddress;
        uint16    u16ShortAddress;
        uint8     u8RequestType;
        uint8     u8StartIndex;
    } PACKED sIEEEAddressRequest;
    
    struct _IEEEAddressResponse
    {
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

    DBG_vPrintf(DBG_ZCB, "Send IEEE Address request to 0x%04X\n", psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u16TargetAddress    = htons(psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u16ShortAddress     = htons(psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u8RequestType       = 0;
    sIEEEAddressRequest.u8StartIndex        = 0;
    
    if (eSL_SendMessage(E_SL_MSG_IEEE_ADDRESS_REQUEST, sizeof(struct _IEEEAddressRequest), 
                                            &sIEEEAddressRequest, &u8SequenceNo) != E_SL_OK){
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_IEEE_ADDRESS_RESPONSE, 1000, &u16Length, (void**)&psIEEEAddressResponse) != E_SL_OK){
            ERR_vPrintf(T_TRUE, "No response to IEEE address request\n");
            goto done;
        }
        if (u8SequenceNo == psIEEEAddressResponse->u8SequenceNo){
            break;
        } else {
            DBG_vPrintf(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", 
                                                                psIEEEAddressResponse->u8SequenceNo, u8SequenceNo);
            FREE(psIEEEAddressResponse);
            psIEEEAddressResponse = NULL;
        }
    }
    if(NULL != psIEEEAddressResponse){
        psZigbee_Node->u64IEEEAddress = be64toh(psIEEEAddressResponse->u64IEEEAddress);
    }
    
    DBG_vPrintf(DBG_ZCB, "Short address 0x%04X has IEEE Address 0x%016llX\n", psZigbee_Node->u16ShortAddress, (unsigned long long int)psZigbee_Node->u64IEEEAddress);
    eStatus = E_ZB_OK;

done:
    //vZigbee_NodeUpdateComms(psZigbee_Node, eStatus);
    FREE(psIEEEAddressResponse);
    return eStatus;
}

teZbStatus eZCB_SimpleDescriptorRequest(tsZigbeeBase *psZigbee_Node, uint8 u8Endpoint)
{
    struct _SimpleDescriptorRequest
    {
        uint16    u16TargetAddress;
        uint8     u8Endpoint;
    } PACKED sSimpleDescriptorRequest;
        
    uint8 u8SequenceNo;
    
    DBG_vPrintf(DBG_ZCB, "Send Simple Desciptor request for Endpoint %d to 0x%04X\n", u8Endpoint, psZigbee_Node->u16ShortAddress);
    
    sSimpleDescriptorRequest.u16TargetAddress       = htons(psZigbee_Node->u16ShortAddress);
    sSimpleDescriptorRequest.u8Endpoint             = u8Endpoint;
    
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
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Mode;
    } PACKED sOnOffMessage;

    /* Just read control bridge, not need lock */
    psSourceEndpoint = psZigbee_NodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_ONOFF);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_ONOFF);
        return E_ZB_ERROR;
    }
    sOnOffMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    
    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sOnOffMessage.u16TargetAddress = htons(psZigbeeNode->u16ShortAddress);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_ONOFF);
        if (psDestinationEndpoint)
        {
            sOnOffMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
    }
    else
    {
        sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sOnOffMessage.u16TargetAddress      = htons(u16GroupAddress);
        sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    sOnOffMessage.u8Mode = u8Mode;
    
    if (eSL_SendMessage(E_SL_MSG_ONOFF_NOEFFECTS, sizeof(sOnOffMessage), &sOnOffMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    if(bZCB_EnableAPSAck)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_AddGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress)
{
    struct _AddGroupMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
    } PACKED sAddGroupMembershipRequest;
    
    struct _sAddGroupMembershipResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
    } PACKED *psAddGroupMembershipResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send add group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZigbeeNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sAddGroupMembershipRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);
    
    if ((eStatus = eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_GROUPS, 
            &sAddGroupMembershipRequest.u8SourceEndpoint, &sAddGroupMembershipRequest.u8DestinationEndpoint)) != E_ZB_OK)
    {
        return eStatus;
    }
    
    sAddGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_ADD_GROUP_REQUEST, sizeof(struct _AddGroupMembershipRequest), &sAddGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the add group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_ADD_GROUP_RESPONSE, 1000, &u16Length, (void**)&psAddGroupMembershipResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to add group membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psAddGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Add group membership sequence number received 0x%02X does not match that sent 0x%02X\n", 
                psAddGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            FREE(psAddGroupMembershipResponse);
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Add group membership 0x%04X on Node 0x%04X status: %d\n", 
        u16GroupAddress, psZigbeeNode->u16ShortAddress, psAddGroupMembershipResponse->u8Status);
    
    eStatus = psAddGroupMembershipResponse->u8Status;

done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psAddGroupMembershipResponse);
    return eStatus;
}

teZbStatus eZCB_RemoveGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress)
{
    struct _RemoveGroupMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
    } PACKED sRemoveGroupMembershipRequest;
    
    struct _sRemoveGroupMembershipResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
    } PACKED *psRemoveGroupMembershipResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send remove group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZigbeeNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sRemoveGroupMembershipRequest.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
    
    if (eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_GROUPS, &sRemoveGroupMembershipRequest.u8SourceEndpoint, &sRemoveGroupMembershipRequest.u8DestinationEndpoint) != E_ZB_OK)
    {
        return E_ZB_ERROR;
    }
    
    sRemoveGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_REMOVE_GROUP_REQUEST, sizeof(struct _RemoveGroupMembershipRequest), &sRemoveGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the remove group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_GROUP_RESPONSE, 1000, &u16Length, (void**)&psRemoveGroupMembershipResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to remove group membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psRemoveGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Remove group membership sequence number received 0x%02X does not match that sent 0x%02X\n", 
                psRemoveGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            FREE(psRemoveGroupMembershipResponse);
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Remove group membership 0x%04X on Node 0x%04X status: %d\n", 
            u16GroupAddress, psZigbeeNode->u16ShortAddress, psRemoveGroupMembershipResponse->u8Status);
    
    eStatus = psRemoveGroupMembershipResponse->u8Status;

done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psRemoveGroupMembershipResponse);
    return eStatus;
}

teZbStatus eZCB_ClearGroupMembership(tsZigbeeBase *psZigbeeNode)
{
    struct _ClearGroupMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
    } PACKED sClearGroupMembershipRequest;
    
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send clear group membership request to 0x%04X\n", psZigbeeNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sClearGroupMembershipRequest.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
    
    if (eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_GROUPS, &sClearGroupMembershipRequest.u8SourceEndpoint, &sClearGroupMembershipRequest.u8DestinationEndpoint) != E_ZB_OK)
    {
        return E_ZB_ERROR;
    }

    if (eSL_SendMessage(E_SL_MSG_REMOVE_ALL_GROUPS, 
            sizeof(struct _ClearGroupMembershipRequest), &sClearGroupMembershipRequest, NULL) != E_SL_OK)
    {
        goto done;
    }
    eStatus = E_ZB_OK;
    
done:
    return eStatus;
}

teZbStatus eZCB_StoreScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID)
{
    struct _StoreSceneRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED sStoreSceneRequest;
    
    struct _sStoreSceneResponse
    {
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

    DBG_vPrintf(DBG_ZCB, "Send store scene %d (Group 0x%04X)\n", 
                u8SceneID, u16GroupAddress);
    
    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sStoreSceneRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);
        
        if (eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_SCENES, 
                &sStoreSceneRequest.u8SourceEndpoint, &sStoreSceneRequest.u8DestinationEndpoint) != E_ZB_OK)
        {
            return E_ZB_ERROR;
        }
    }
    else
    {
        sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sStoreSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sStoreSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        
        if (eZigbee_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sStoreSceneRequest.u8SourceEndpoint, NULL) != E_ZB_OK)
        {
            return E_ZB_ERROR;
        }
    }
    
    sStoreSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sStoreSceneRequest.u8SceneID        = u8SceneID;

    if (eSL_SendMessage(E_SL_MSG_STORE_SCENE, sizeof(struct _StoreSceneRequest), &sStoreSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_STORE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psStoreSceneResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to store scene request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Store scene sequence number received 0x%02X does not match that sent 0x%02X\n", 
                    psStoreSceneResponse->u8SequenceNo, u8SequenceNo);
            FREE(psStoreSceneResponse);
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Store scene %d (Group0x%04X) on Node 0x%04X status: %d\n", 
                psStoreSceneResponse->u8SceneID, ntohs(psStoreSceneResponse->u16GroupAddress), 
                ntohs(psZigbeeNode->u16ShortAddress), psStoreSceneResponse->u8Status);
    
    eStatus = psStoreSceneResponse->u8Status;
done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psStoreSceneResponse);
    return eStatus;
}

teZbStatus eZCB_RemoveScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SceneID)
{
    struct _RemoveSceneRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED sRemoveSceneRequest;
    
    struct _sStoreSceneResponse
    {
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


    if (psZigbeeNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send remove scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n", 
                    u16SceneID, u16GroupAddress, sRemoveSceneRequest.u8DestinationEndpoint, psZigbeeNode->u16ShortAddress);
        if (bZCB_EnableAPSAck)
        {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRemoveSceneRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);
        
        if (eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint, &sRemoveSceneRequest.u8DestinationEndpoint) != E_ZB_OK)
        {
            return E_ZB_ERROR;
        }
    }
    else
    {
        sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sRemoveSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sRemoveSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        
        if (eZigbee_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint, NULL) != E_ZB_OK)
        {
            return E_ZB_ERROR;
        }
    }
    
    sRemoveSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRemoveSceneRequest.u8SceneID        = (uint8)u16SceneID;

    if (eSL_SendMessage(E_SL_MSG_REMOVE_SCENE, sizeof(struct _RemoveSceneRequest), &sRemoveSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psRemoveSceneResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to remove scene request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Remove scene sequence number received 0x%02X does not match that sent 0x%02X\n", 
                psRemoveSceneResponse->u8SequenceNo, u8SequenceNo);
            FREE(psRemoveSceneResponse);
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Remove scene %d (Group0x%04X) on Node 0x%04X status: %d\n", 
                psRemoveSceneResponse->u8SceneID, ntohs(psRemoveSceneResponse->u16GroupAddress), 
                psZigbeeNode->u16ShortAddress, psRemoveSceneResponse->u8Status);
    
    eStatus = psRemoveSceneResponse->u8Status;
done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psRemoveSceneResponse);
    return eStatus;
}

teZbStatus eZCB_RecallScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID)
{
    uint8         u8SequenceNo;
    struct _RecallSceneRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } PACKED sRecallSceneRequest;

    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    if (psZigbeeNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send recall scene %d (Group 0x%04X) to 0x%04X\n", 
                u8SceneID, u16GroupAddress, psZigbeeNode->u16ShortAddress);
        
        if (bZCB_EnableAPSAck)
        {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRecallSceneRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);
        
        if (eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_SCENES, 
                &sRecallSceneRequest.u8SourceEndpoint, &sRecallSceneRequest.u8DestinationEndpoint) != E_ZB_OK)
        {
            return E_ZB_ERROR;
        }
    }
    else
    {
        sRecallSceneRequest.u8TargetAddressMode  = E_ZB_ADDRESS_MODE_GROUP;
        sRecallSceneRequest.u16TargetAddress     = htons(u16GroupAddress);
        sRecallSceneRequest.u8DestinationEndpoint= ZB_DEFAULT_ENDPOINT_ZLL;
        
        if (eZigbee_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRecallSceneRequest.u8SourceEndpoint, NULL) != E_ZB_OK)
        {
            return E_ZB_ERROR;
        }
    }

    sRecallSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRecallSceneRequest.u8SceneID        = u8SceneID;
    
    if (eSL_SendMessage(E_SL_MSG_RECALL_SCENE, sizeof(struct _RecallSceneRequest), &sRecallSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    if (psZigbeeNode)
    {
        eStatus = eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        eStatus = E_ZB_OK;
    }
done:
    return eStatus;
}

teZbStatus eZCB_ReadAttributeRequest(tsZigbeeBase *psZigbee_Node, uint16 u16ClusterID, uint8 u8Direction, 
                uint8 u8ManufacturerSpecific, uint16 u16ManufacturerID, uint16 u16AttributeID, void *pvData)
{
    struct _ReadAttributeRequest
    {
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
    
    struct _ReadAttributeResponse
    {
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8Status;
        uint8     u8Type;
        uint16    u16SizeOfAttributesInBytes;
        union
        {
            uint8     u8Data;
            uint16    u16Data;
            uint32    u32Data;
            uint64    u64Data;
        } uData;
    } PACKED *psReadAttributeResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Read Attribute request to 0x%04X\n", psZigbee_Node->u16ShortAddress);
    DBG_vPrintf(DBG_ZCB, "Send Read Cluster ID = 0x%04X\n", u16ClusterID);
    
    if (bZCB_EnableAPSAck)
    {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sReadAttributeRequest.u16TargetAddress      = htons(psZigbee_Node->u16ShortAddress);
    
    if ((eStatus = eZigbee_GetEndpoints(psZigbee_Node, u16ClusterID, 
            &sReadAttributeRequest.u8SourceEndpoint, &sReadAttributeRequest.u8DestinationEndpoint)) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "Can't Find Endpoint\n");
        goto done;
    }
    
    sReadAttributeRequest.u16ClusterID = htons(u16ClusterID);
    sReadAttributeRequest.u8Direction = u8Direction;
    sReadAttributeRequest.u8ManufacturerSpecific = u8ManufacturerSpecific;
    sReadAttributeRequest.u16ManufacturerID = htons(u16ManufacturerID);
    sReadAttributeRequest.u8NumAttributes = 1;
    sReadAttributeRequest.au16Attribute[0] = htons(u16AttributeID);
    
    if (eSL_SendMessage(E_SL_MSG_READ_ATTRIBUTE_REQUEST, sizeof(struct _ReadAttributeRequest), &sReadAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_READ_ATTRIBUTE_RESPONSE, 1000, &u16Length, (void**)&psReadAttributeResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE, "No response to read attribute request\n");
            eStatus = E_ZB_COMMS_FAILED;
            goto done;
        }
        
        if (u8SequenceNo == psReadAttributeResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            ERR_vPrintf(T_TRUE, "Read Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", psReadAttributeResponse->u8SequenceNo, u8SequenceNo);
            FREE(psReadAttributeResponse);
            goto done;
        }
    }

    if (psReadAttributeResponse->u8Status != E_ZB_OK)
    {
        DBG_vPrintf(DBG_ZCB, "Read Attribute respose error status: %d\n", psReadAttributeResponse->u8Status);
        goto done;
    }  
    
    /* Copy the data into the pointer passed to us.
     * We assume that the memory pointed to will be the right size for the data that has been requested!
     */
    DBG_vPrintf(DBG_ZCB, "Copy the data into the pointer passed to us\n");
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
            
            WAR_vPrintf(1, "data:%d\n", psReadAttributeResponse->uData.u8Data);
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
            ERR_vPrintf(T_TRUE,  "Unknown attribute data type (%d) received from node 0x%04X", psReadAttributeResponse->u8Type, psZigbee_Node->u16ShortAddress);
            break;
    }
done:
    FREE(psReadAttributeResponse);   
    return eStatus;
}

teZbStatus eZCB_ManagementLeaveRequest(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren)
{
    struct _sManagementLeavRequest
    {
        uint16    u16TargetShortAddress;
        uint64    u64TargetAddress;
        uint8     u8Rejoin;
        uint8     u8RemoveChildren;
    } PACKED sManagementLeaveRequest;
    
    struct _sManagementLeavResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
    } PACKED *psManagementLeaveResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    
    if (psZigbeeNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send management Leave (u64TargetAddress %llu)\n", 
                    psZigbeeNode->u64IEEEAddress);
        sManagementLeaveRequest.u16TargetShortAddress = 0x0000;
        sManagementLeaveRequest.u8Rejoin = u8Rejoin;
        sManagementLeaveRequest.u8RemoveChildren = u8RemoveChildren;
        sManagementLeaveRequest.u64TargetAddress = Swap64(psZigbeeNode->u64IEEEAddress);
    }

    if (eSL_SendMessage(E_SL_MSG_MANAGEMENT_LEAVE_REQUEST, sizeof(sManagementLeaveRequest), &sManagementLeaveRequest, &u8SequenceNo) != E_SL_OK)
    {
        ERR_vPrintf(T_TRUE,  "eSL_SendMessage Error\n");
        goto done;
    }
    
    while (1)
    {
        if (eSL_MessageWait(E_SL_MSG_MANAGEMENT_LEAVE_RESPONSE, 1000, &u16Length, (void**)&psManagementLeaveResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to Management Leave request\n");
            goto done;
        }
        break;
    }
        
    eStatus = psManagementLeaveResponse->u8Status;
    
    psZigbeeNode->u8DeviceOnline = 0;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
    
    tsZigbeeNodes *psZigbeeNodeT = NULL;
    psZigbeeNodeT = psZigbee_FindNodeByIEEEAddress(psZigbeeNode->u64IEEEAddress);
    if(psZigbeeNodeT) eZigbee_RemoveNode(psZigbeeNodeT);
done:
    //vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    FREE(psManagementLeaveResponse);
    return eStatus;
}

teZbStatus eZCB_ZLL_MoveToLevel(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8OnOff, uint8 u8Level, uint16 u16TransitionTime)
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8OnOff;
        uint8     u8Level;
        uint16    u16TransitionTime;
    } PACKED sLevelMessage;
        
    if (u8Level > 254)
    {
        u8Level = 254;
    }
    
    psSourceEndpoint = psZigbee_NodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_LEVEL_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_LEVEL_CONTROL);
        return E_ZB_ERROR;
    }
    sLevelMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sLevelMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_LEVEL_CONTROL);

        if (psDestinationEndpoint)
        {
            sLevelMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sLevelMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
    }
    else
    {
        sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sLevelMessage.u16TargetAddress      = htons(u16GroupAddress);
        sLevelMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sLevelMessage.u8OnOff               = u8OnOff;
    sLevelMessage.u8Level               = u8Level;
    sLevelMessage.u16TransitionTime     = htons(u16TransitionTime);
    
    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_LEVEL_ONOFF, sizeof(sLevelMessage), &sLevelMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    if (bZCB_EnableAPSAck)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_ZLL_MoveToHueSaturation(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Hue, uint8 u8Saturation, uint16 u16TransitionTime)
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Hue;
        uint8     u8Saturation;
        uint16    u16TransitionTime;
    } PACKED sMoveToHueSaturationMessage;
    
    DBG_vPrintf(DBG_ZCB, "Set Hue %d, Saturation %d\n", u8Hue, u8Saturation);
    
    psSourceEndpoint = psZigbee_NodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }
    sMoveToHueSaturationMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToHueSaturationMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
    }
    else
    {
        sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToHueSaturationMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToHueSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToHueSaturationMessage.u8Hue               = u8Hue;
    sMoveToHueSaturationMessage.u8Saturation        = u8Saturation;
    sMoveToHueSaturationMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE_SATURATION, 
            sizeof(sMoveToHueSaturationMessage), &sMoveToHueSaturationMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    if (bZCB_EnableAPSAck)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_NeighbourTableRequest(int *pStart)
{
    struct _ManagementLQIRequest
    {
        uint16    u16TargetAddress;
        uint8     u8StartIndex;
    } PACKED sManagementLQIRequest;
    
    struct _ManagementLQIResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint8     u8NeighbourTableSize;
        uint8     u8TableEntries;
        uint8     u8StartIndex;
        struct
        {
            uint16    u16ShortAddress;
            uint64    u64PanID;
            uint64    u64IEEEAddress;
            uint8     u8Depth;
            uint8     u8LQI;
            struct
            {
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
    
    DBG_vPrintf(DBG_ZCB, "Send management LQI request to 0x%04X for entries starting at %d\n", 
                                                            u16ShortAddress, sManagementLQIRequest.u8StartIndex);
    
    if (eSL_SendMessage(E_SL_MSG_MANAGEMENT_LQI_REQUEST, sizeof(struct _ManagementLQIRequest), &sManagementLQIRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_MANAGEMENT_LQI_RESPONSE, 1000, &u16Length, (void**)&psManagementLQIResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE, "No response to management LQI requestn\n");
            goto done;
        }
        else if (u8SequenceNo == psManagementLQIResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psManagementLQIResponse->u8SequenceNo, u8SequenceNo);
            FREE(psManagementLQIResponse);
        }
    }
    if(psManagementLQIResponse->u8Status == CZD_NW_STATUS_SUCCESS)
    {
        DBG_vPrintf(DBG_ZCB, "Received management LQI response. Table size: %d, Entry count: %d, start index: %d\n",
                    psManagementLQIResponse->u8NeighbourTableSize,
                    psManagementLQIResponse->u8TableEntries,
                    psManagementLQIResponse->u8StartIndex);
    }
    else
    {
        DBG_vPrintf(DBG_ZCB, "Received error status in management LQI response : 0x%02x\n",psManagementLQIResponse->u8Status);
        goto done;
    }
    
    for (i = 0; i < psManagementLQIResponse->u8TableEntries; i++)
    {        
        psManagementLQIResponse->asNeighbours[i].u16ShortAddress    = ntohs(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);
        psManagementLQIResponse->asNeighbours[i].u64PanID           = be64toh(psManagementLQIResponse->asNeighbours[i].u64PanID);
        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress     = be64toh(psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);

        if ((psManagementLQIResponse->asNeighbours[i].u16ShortAddress >= 0xFFFA) ||
            (psManagementLQIResponse->asNeighbours[i].u64IEEEAddress  == 0))
        {
          /* Illegal short / IEEE address */
          continue;
        }
        
        DBG_vPrintf(DBG_ZCB, "  Entry %02d: Short Address 0x%04X, PAN ID: 0x%016llX, IEEE Address: 0x%016llX\n", i,
                    psManagementLQIResponse->asNeighbours[i].u16ShortAddress,
                    psManagementLQIResponse->asNeighbours[i].u64PanID,
                    psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);
        
        DBG_vPrintf(DBG_ZCB, "    Type: %d, Permit Joining: %d, Relationship: %d, RxOnWhenIdle: %d\n",
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
        
        DBG_vPrintf(DBG_ZCB, "    Depth: %d, LQI: %d\n", 
                    psManagementLQIResponse->asNeighbours[i].u8Depth, 
                    psManagementLQIResponse->asNeighbours[i].u8LQI);

        vZCB_AddNodeIntoNetwork(psManagementLQIResponse->asNeighbours[i].u16ShortAddress, 
            psManagementLQIResponse->asNeighbours[i].u64IEEEAddress, u8MacCapability);

        sleep(1);//avoid compete
    }
    
    if (psManagementLQIResponse->u8TableEntries > 0)
    {
        // We got some entries, so next time request the entries after these.
        *pStart += psManagementLQIResponse->u8TableEntries;
        if (*pStart >= psManagementLQIResponse->u8NeighbourTableSize)/* Make sure we're still in table boundaries */
        {
            *pStart = 0;
        }
    }
    else
    {
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
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Command;
    } PACKED sWindowCoveringDeviceMessage;

    /* Just read control bridge, not need lock */
    psSourceEndpoint = psZigbee_NodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_WINDOW_COVERING_DEVICE);
    if (!psSourceEndpoint)
    {
        ERR_vPrintf(T_TRUE, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_WINDOW_COVERING_DEVICE);
        return E_ZB_ERROR;
    }
    sWindowCoveringDeviceMessage.u8SourceEndpoint = psSourceEndpoint->u8Endpoint;
    
    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sWindowCoveringDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sWindowCoveringDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sWindowCoveringDeviceMessage.u16TargetAddress = htons(psZigbeeNode->u16ShortAddress);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_WINDOW_COVERING_DEVICE);
        if (psDestinationEndpoint)
        {
            sWindowCoveringDeviceMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sWindowCoveringDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
        }
    }
    else
    {
        sWindowCoveringDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_BROADCAST;
        sWindowCoveringDeviceMessage.u16TargetAddress      = htons(E_ZB_BROADCAST_ADDRESS_ALL);
        sWindowCoveringDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
    }
    sWindowCoveringDeviceMessage.u8Command = (uint8)eCommand;
    
    if (eSL_SendMessage(E_SL_MSG_WINDOW_COVERING_DEVICE_OPERATOR, sizeof(sWindowCoveringDeviceMessage), &sWindowCoveringDeviceMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    if(bZCB_EnableAPSAck)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZCB_DoorLockDeviceOperator(tsZigbeeBase *psZigbeeNode, teCLD_DoorLock_CommandID eCommand )
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8         u8SequenceNo;

    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Command;
    } PACKED sDoorLockDeviceMessage;

    /* Just read control bridge, not need lock */
    psSourceEndpoint = psZigbee_NodeFindEndpoint(&sControlBridge.sNode, E_ZB_CLUSTERID_DOOR_LOCK);
    if (!psSourceEndpoint)
    {
        ERR_vPrintf(T_TRUE, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_DOOR_LOCK);
        return E_ZB_ERROR;
    }
    sDoorLockDeviceMessage.u8SourceEndpoint = psSourceEndpoint->u8Endpoint;

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sDoorLockDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sDoorLockDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sDoorLockDeviceMessage.u16TargetAddress = htons(psZigbeeNode->u16ShortAddress);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_DOOR_LOCK);
        if (psDestinationEndpoint)
        {
            sDoorLockDeviceMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sDoorLockDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
        }
    }
    else
    {
        sDoorLockDeviceMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_BROADCAST;
        sDoorLockDeviceMessage.u16TargetAddress      = htons(E_ZB_BROADCAST_ADDRESS_ALL);
        sDoorLockDeviceMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_HA;
    }
    sDoorLockDeviceMessage.u8Command = (uint8)eCommand;

    if(sDoorLockDeviceMessage.u16TargetAddress == 0){/*Coordinator*/
        if (eSL_SendMessage(E_SL_MSG_DOOR_LOCK_SET_DOOR_STATE, sizeof(sDoorLockDeviceMessage), &sDoorLockDeviceMessage, &u8SequenceNo) != E_SL_OK)
        {
            return E_ZB_COMMS_FAILED;
        }
    }else{
        if (eSL_SendMessage(E_SL_MSG_LOCK_UNLOCK_DOOR, sizeof(sDoorLockDeviceMessage), &sDoorLockDeviceMessage, &u8SequenceNo) != E_SL_OK)
        {
            return E_ZB_COMMS_FAILED;
        }
        if(bZCB_EnableAPSAck)
        {
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
        if (eSL_MessageWait(E_SL_MSG_DEFAULT_RESPONSE, 1000, &u16Length, (void**)&psDefaultResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to command sequence number %d received", u8SequenceNo);
            goto done;
        }
        
        if (u8SequenceNo != psDefaultResponse->u8SequenceNo)
        {
            DBG_vPrintf(DBG_ZCB, "Default response sequence number received 0x%02X does not match that sent 0x%02X\n", 
                psDefaultResponse->u8SequenceNo, u8SequenceNo);
            FREE(psDefaultResponse);
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Default response for message sequence number 0x%02X status is %d\n", 
                psDefaultResponse->u8SequenceNo, psDefaultResponse->u8Status);
            eStatus = psDefaultResponse->u8Status;
            break;
        }
    }
done:
    FREE(psDefaultResponse);
    return eStatus;
}

/****************************************************************************/
/***        Local    Functions                                            ***/
/****************************************************************************/
static void vZCB_AddNodeIntoNetwork(uint16 u16ShortAddress, uint64 u64IEEEAddress, uint8 u8MacCapability)
{
    tsZigbeeNodes *psZigbeeNodeTemp = NULL;
    psZigbeeNodeTemp = psZigbee_FindNodeByShortAddress(u16ShortAddress);
    
    if ((NULL != psZigbeeNodeTemp)&&(0 != psZigbeeNodeTemp->sNode.u16DeviceID) )//New Nodes
    {
        DBG_vPrintf(DBG_ZCB, "The Node:0x%04x already in the network\n", psZigbeeNodeTemp->sNode.u16ShortAddress);
        return;
    }
    
    eZigbee_AddNode(u16ShortAddress, u64IEEEAddress, 0x0000, u8MacCapability, &psZigbeeNodeTemp);
    if(u8MacCapability & E_ZB_MAC_CAPABILITY_FFD){ //router, we need get its' device id
        if(0 == psZigbeeNodeTemp->sNode.u16DeviceID){ //unfinished node
            DBG_vPrintf(DBG_ZCB, "eZCB_MatchDescriptorRequest\n");
            usleep(1000);
            if(eZCB_MatchDescriptorRequest(u16ShortAddress, au16ProfileHA,sizeof(au16Cluster) / sizeof(uint16), au16Cluster, 0, NULL, NULL) != E_ZB_OK)
            {
                ERR_vPrintf(DBG_ZCB, "Error sending match descriptor request\n");
            }
        }
    } else { //enddevice, no need device id
        vZCB_InitZigbeeNodeInfo(psZigbeeNodeTemp, E_ZBD_END_DEVICE_DEVICE);
    }
}

static void vZCB_InitZigbeeNodeInfo(tsZigbeeNodes *psZigbeeNode, uint16 u16DeviceID)
{
    DBG_vPrintf(DBG_ZCB, "************vZCB_InitZigbeeNodeInfo\n");
    tsDeviceIDMap *psDeviceIDMap = asDeviceIDMap;
    eLockLock(&psZigbeeNode->mutex);
    psZigbeeNode->sNode.u16DeviceID = u16DeviceID; //update device id
    DBG_vPrintf(DBG_ZCB, "Init Device 0x%04x\n",psZigbeeNode->sNode.u16DeviceID);
    while (((psDeviceIDMap->u16ZigbeeDeviceID != 0) && (psDeviceIDMap->prInitaliseRoutine != NULL)))
    {
        if (psDeviceIDMap->u16ZigbeeDeviceID == psZigbeeNode->sNode.u16DeviceID)
        {
            DBG_vPrintf(DBG_ZCB, "Found Zigbee device type for Zigbee Device type 0x%04X\n", psDeviceIDMap->u16ZigbeeDeviceID);
            psDeviceIDMap->prInitaliseRoutine(psZigbeeNode);
        }
        psDeviceIDMap++;
    }
    eLockunLock(&psZigbeeNode->mutex);
}

static void vZCB_HandleNodeClusterList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8003]ZCB_HandleNodeClusterList\n");
    int iPosition;
    int iCluster = 0;
    struct _tsClusterList
    {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    au16ClusterList[255];
    } PACKED *psClusterList = (struct _tsClusterList *)pvMessage;
    
    psClusterList->u16ProfileID = ntohs(psClusterList->u16ProfileID);
    
    DBG_vPrintf(DBG_ZCB, "Cluster list for endpoint %d, profile ID 0x%4X\n", 
                psClusterList->u8Endpoint, 
                psClusterList->u16ProfileID);
    
    eLockLock(&sControlBridge.mutex); //lock Coordinator Node

    if (eZigbee_NodeAddEndpoint(&sControlBridge.sNode, psClusterList->u8Endpoint, psClusterList->u16ProfileID, NULL) != E_ZB_OK)
    {
        goto done;
    }

    iPosition = sizeof(uint8) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbee_NodeAddCluster(&sControlBridge.sNode, psClusterList->u8Endpoint, ntohs(psClusterList->au16ClusterList[iCluster])) != E_ZB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16);
        iCluster++;
    }
done:
    eLockunLock(&sControlBridge.mutex);
}

static void vZCB_HandleNodeClusterAttributeList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8004]ZCB_HandleNodeClusterAttributeList\n");
    int iPosition;
    int iAttribute = 0;
    struct _tsClusterAttributeList
    {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16ClusterID;
        uint16    au16AttributeList[255];
    } PACKED *psClusterAttributeList = (struct _tsClusterAttributeList *)pvMessage;
    
    psClusterAttributeList->u16ProfileID = ntohs(psClusterAttributeList->u16ProfileID);
    psClusterAttributeList->u16ClusterID = ntohs(psClusterAttributeList->u16ClusterID);
    
    DBG_vPrintf(DBG_ZCB, "Cluster attribute list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n", 
                psClusterAttributeList->u8Endpoint, 
                psClusterAttributeList->u16ClusterID,
                psClusterAttributeList->u16ProfileID);
    
    eLockLock(&sControlBridge.mutex);
    iPosition = sizeof(uint8) + sizeof(uint16) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbee_NodeAddAttribute(&sControlBridge.sNode, psClusterAttributeList->u8Endpoint, 
            psClusterAttributeList->u16ClusterID, ntohs(psClusterAttributeList->au16AttributeList[iAttribute])) != E_ZB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16);
        iAttribute++;
    }
    
done:
    eLockunLock(&sControlBridge.mutex);
}

static void vZCB_HandleNodeCommandIDList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8005]ZCB_HandleNodeCommandIDList\n");
    int iPosition;
    int iCommand = 0;
    struct _tsCommandIDList
    {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16ClusterID;
        uint8     au8CommandList[255];
    } PACKED *psCommandIDList = (struct _tsCommandIDList *)pvMessage;
    
    psCommandIDList->u16ProfileID = ntohs(psCommandIDList->u16ProfileID);
    psCommandIDList->u16ClusterID = ntohs(psCommandIDList->u16ClusterID);
    
    DBG_vPrintf(DBG_ZCB, "Command ID list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n", 
                psCommandIDList->u8Endpoint, 
                psCommandIDList->u16ClusterID,
                psCommandIDList->u16ProfileID);
    
    eLockLock(&sControlBridge.mutex);
    
    iPosition = sizeof(uint8) + sizeof(uint16) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbee_NodeAddCommand(&sControlBridge.sNode, psCommandIDList->u8Endpoint, 
            psCommandIDList->u16ClusterID, psCommandIDList->au8CommandList[iCommand]) != E_ZB_OK)
        {
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
    DBG_vPrintf(DBG_ZCB, "************[0x8006]ZCB_HandleRestartProvisioned\n");
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
    WAR_vPrintf(T_TRUE,  "Control bridge restarted, status %d (%s)\n", psWarmRestart->u8Status, pcStatus);
    return;
}

static void vZCB_HandleRestartFactoryNew(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8007]ZCB_HandleRestartFactoryNew\n");
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
    WAR_vPrintf(T_TRUE,  "Control bridge factory new restart, status %d (%s)", psWarmRestart->u8Status, pcStatus);
    
    eZCB_ConfigureControlBridge();
    return;
}

static void vZCB_HandleNetworkJoined(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8024]ZCB_HandleNetworkJoined\n");
    struct _tsNetworkJoinedFormed
    {
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint64    u64IEEEAddress;
        uint8     u8Channel;
    } PACKED *psMessage = (struct _tsNetworkJoinedFormed *)pvMessage;

    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);

    DBG_vPrintf(DBG_ZCB, "Network %s on channel %d. Control bridge address 0x%04X (0x%016llX)\n", 
                        psMessage->u8Status == 0 ? "joined" : "formed",
                        psMessage->u8Channel, psMessage->u16ShortAddress,
                        (unsigned long long int)psMessage->u64IEEEAddress);

    /* Control bridge joined the network - initialise its data in the network structure */
    eLockLock(&sControlBridge.mutex);

    sControlBridge.sNode.u16DeviceID     = E_ZBD_COORDINATOR;
    sControlBridge.sNode.u16ShortAddress = psMessage->u16ShortAddress;
    sControlBridge.sNode.u64IEEEAddress  = psMessage->u64IEEEAddress;
    sControlBridge.sNode.u8MacCapability = E_ZB_MAC_CAPABILITY_ALT_PAN_COORD|E_ZB_MAC_CAPABILITY_FFD|E_ZB_MAC_CAPABILITY_POWERED|E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE;

    DBG_vPrintf(DBG_ZCB, "Node Joined 0x%04X (0x%016llX)\n", 
                        sControlBridge.sNode.u16ShortAddress, 
                        (unsigned long long int)sControlBridge.sNode.u64IEEEAddress);    

    vZigbee_PrintNode(&sControlBridge.sNode);
    asDeviceIDMap[0].prInitaliseRoutine(&sControlBridge);
    eLockunLock(&sControlBridge.mutex);
}

/** When a new device join network, this func will be called, we can get netaddr macaddr */
static void vZCB_HandleDeviceAnnounce(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x004D]vZCB_HandleDeviceAnnounce\n");
    struct _tsDeviceAnnounce
    {
        uint16    u16ShortAddress;
        uint64    u64IEEEAddress;
        uint8     u8MacCapability;
    } PACKED *psMessage = (struct _tsDeviceAnnounce *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    
    DBG_vPrintf(DBG_ZCB, "Device Joined, Address 0x%04X (0x%016llX). Mac Capability Mask 0x%02X\n", 
                psMessage->u16ShortAddress,(unsigned long long int)psMessage->u64IEEEAddress,psMessage->u8MacCapability);
    
    vZCB_AddNodeIntoNetwork(psMessage->u16ShortAddress, psMessage->u64IEEEAddress, psMessage->u8MacCapability);
        
    return;
}


/** 
* When a router device reset, it will not send device annouce, so we should send 
* match descriptor request for get device information. we can get nwk address.
*/
static void vZCB_HandleMatchDescriptorResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8046]ZCB_HandleMatchDescriptorResponse\n");
    struct _tMatchDescriptorResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint8     u8NumEndpoints;
        uint8     au8Endpoints[255];
    } PACKED *psMatchDescriptorResponse = (struct _tMatchDescriptorResponse *)pvMessage;

    psMatchDescriptorResponse->u16ShortAddress  = ntohs(psMatchDescriptorResponse->u16ShortAddress);
    if (psMatchDescriptorResponse->u8NumEndpoints) /* if endpoint's number is 0, this is a invaild device */
    {
        tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByShortAddress(psMatchDescriptorResponse->u16ShortAddress);
        if((NULL == psZigbeeNode) || (psZigbeeNode->sNode.u16DeviceID != 0)){
            return ;
        }
        eLockLock(&psZigbeeNode->mutex);
        for (int i = 0; i < psMatchDescriptorResponse->u8NumEndpoints; i++) {
            /* Add an endpoint to the device for each response in the match descriptor response */
            eZigbee_NodeAddEndpoint(&psZigbeeNode->sNode, psMatchDescriptorResponse->au8Endpoints[i], 0, NULL);            
        }
        eLockunLock(&psZigbeeNode->mutex);    
        
        for (int i = 0; i < psZigbeeNode->sNode.u32NumEndpoints; i++)/* get profile id, device id, input clusters */
        {
            if (psZigbeeNode->sNode.pasEndpoints[i].u16ProfileID == 0)
            {
                usleep(500);
                if (eZCB_SimpleDescriptorRequest(&psZigbeeNode->sNode, psZigbeeNode->sNode.pasEndpoints[i].u8Endpoint) != E_ZB_OK){
                    ERR_vPrintf(T_TRUE, "Failed to read endpoint simple descriptor - requeue\n");
                    eZigbee_RemoveNode(psZigbeeNode);
                    return ;
                }
            }
        }
    }
}

static void vZCB_HandleSimpleDescriptorResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8043]vZCB_HandleSimpleDescriptorResponse\n");
    struct _tSimpleDescriptorResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint8     u8Length;
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16DeviceID;
        struct
        {
          uint8   u8DeviceVersion :4;
          uint8   u8Reserved :4;
        }PACKED sBitField;
        tsZDClusterList sInputClusters;
    } PACKED *psSimpleDescriptorResponse = (struct _tSimpleDescriptorResponse*)pvMessage;
    
    psSimpleDescriptorResponse->u8Length        = ntohs(psSimpleDescriptorResponse->u8Length);
    psSimpleDescriptorResponse->u16DeviceID     = ntohs(psSimpleDescriptorResponse->u16DeviceID);
    psSimpleDescriptorResponse->u16ProfileID    = ntohs(psSimpleDescriptorResponse->u16ProfileID);
    psSimpleDescriptorResponse->u16ShortAddress = ntohs(psSimpleDescriptorResponse->u16ShortAddress);
    DBG_vPrintf(DBG_ZCB, "Get Simple Desciptor response for Endpoint %d to 0x%04X\n", 
        psSimpleDescriptorResponse->u8Endpoint, psSimpleDescriptorResponse->u16ShortAddress);

    tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByShortAddress(psSimpleDescriptorResponse->u16ShortAddress);
    if((NULL == psZigbeeNode) || (psZigbeeNode->sNode.u16DeviceID != 0)){
        return ;
    }
    if (eZigbee_NodeAddEndpoint(&psZigbeeNode->sNode, 
        psSimpleDescriptorResponse->u8Endpoint, ntohs(psSimpleDescriptorResponse->u16ProfileID), NULL) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "eZigbee_NodeAddEndpoint error\n");
        eZigbee_RemoveNode(psZigbeeNode);
        return ;
    }

    for (int i = 0; i < psSimpleDescriptorResponse->sInputClusters.u8ClusterCount; i++){
        uint16 u16ClusterID = ntohs(psSimpleDescriptorResponse->sInputClusters.au16Clusters[i]);
        if (eZigbee_NodeAddCluster(&psZigbeeNode->sNode, psSimpleDescriptorResponse->u8Endpoint, u16ClusterID) != E_ZB_OK){
            ERR_vPrintf(T_TRUE, "eZigbee_NodeAddCluster error\n");
            eZigbee_RemoveNode(psZigbeeNode);
            return ;
        }
    }
    vZCB_InitZigbeeNodeInfo(psZigbeeNode, psSimpleDescriptorResponse->u16DeviceID);
}

static void vZCB_HandleDoorLockControllerRequest(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x00F2]vZCB_HandleDoorLockControllerRequest\n");
    struct _tDoorLockControllerRequest
    {
        uint8     u8SequenceNo;
        uint8     u8SrcEndpoint;
        uint16    u16ClusterID;
        uint8     u8SrcAddrMode;
        uint16    u16ShortAddress;
        uint8     u8CommandID;
        uint8     u8PasswordLen;
        uint8     auPasswordData[10];
    } PACKED *psDoorLockControllerRequest = (struct _tDoorLockControllerRequest*)pvMessage;
    psDoorLockControllerRequest->u8CommandID     = (psDoorLockControllerRequest->u8CommandID);
    psDoorLockControllerRequest->u8PasswordLen   = (psDoorLockControllerRequest->u8PasswordLen);
    psDoorLockControllerRequest->u16ShortAddress = ntohs(psDoorLockControllerRequest->u16ShortAddress);
    DBG_vPrintf(DBG_ZCB, "Get Door Lock Controller[0x%04X] Request:%d, Password[%d]:%s\n",
                psDoorLockControllerRequest->u16ShortAddress, psDoorLockControllerRequest->u8CommandID,
                psDoorLockControllerRequest->u8PasswordLen, psDoorLockControllerRequest->auPasswordData);

    tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByShortAddress(psDoorLockControllerRequest->u16ShortAddress);
    if((NULL == psZigbeeNode) || (psZigbeeNode->sNode.u16DeviceID != 0)){
        WAR_vPrintf(T_TRUE, "Can't Found This Node:0x%04X\n", psDoorLockControllerRequest->u16ShortAddress);
        return ;
    }
    //TODO:Set Door Lock State
}

static void vZCB_HandleDeviceLeave(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x004D]vZCB_HandleDeviceLeave\n");
    struct _tsLeaveIndication
    {
        uint64    u64IEEEAddress;
        uint8     u8Rejoin;
    } PACKED *psMessage = (struct _tsLeaveIndication *)pvMessage;
    
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);

    tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByIEEEAddress(psMessage->u64IEEEAddress);
    if(NULL == psZigbeeNode){
        ERR_vPrintf(T_TRUE, "Can't find this node in the network!\n");
        return;
    }
    psZigbeeNode->sNode.u8DeviceOnline = 0;
    eZigbeeSqliteUpdateDeviceTable(&psZigbeeNode->sNode, E_SQ_DEVICE_ONLINE);
    
    if(psZigbeeNode) eZigbee_RemoveNode(psZigbeeNode);
    
    return;
}

static void vZCB_HandleAttributeReport(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB, "************[0x8102]vZCB_HandleAttributeReport\n");
    struct _tsAttributeReport {
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8AttributeStatus;
        uint8     u8Type;
        uint16    u16SizeOfAttributesInBytes;
        union {
            uint8     u8Data;
            uint16    u16Data;
            uint32    u32Data;
            uint64    u64Data;
        } uData;
    } PACKED *psMessage = (struct _tsAttributeReport *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);
    psMessage->u16AttributeID   = ntohs(psMessage->u16AttributeID);
    
    DBG_vPrintf( DBG_ZCB, "Attribute report from 0x%04X - Endpoint %d, cluster 0x%04X, attribute 0x%04X.\n", 
                psMessage->u16ShortAddress,
                psMessage->u8Endpoint,
                psMessage->u16ClusterID,
                psMessage->u16AttributeID
            );
    
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
            break;
        
        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            psMessage->uData.u16Data = ntohs(psMessage->uData.u16Data);
            break;
 
        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            psMessage->uData.u32Data = ntohl(psMessage->uData.u32Data);
            break;
 
        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            psMessage->uData.u64Data = be64toh(psMessage->uData.u64Data);
            break;
            
        default:
            ERR_vPrintf(T_TRUE,  "Unknown attribute data type (%d) received from node 0x%04X", psMessage->u8Type, psMessage->u16ShortAddress);
            break;
    }
    tsZigbeeNodes *psZigbeeNode = psZigbee_FindNodeByShortAddress(psMessage->u16ShortAddress);
    if(NULL == psZigbeeNode){
        WAR_vPrintf(T_TRUE, "Can't find this node in network.\n");
        return;
    }
    switch(psMessage->u16ClusterID){
        case(E_ZB_CLUSTERID_ILLUMINANCE):
            if(psZigbeeNode->sNode.u16DeviceID == 0){
                WAR_vPrintf(T_TRUE, "This node is not in network truly\n");
            }else{
                eLockLock(&psZigbeeNode->mutex);
                INF_vPrintf(DBG_ZCB, "update illum attribute to %d\n", psMessage->uData.u16Data);
                psZigbeeNode->sNode.sAttributeValue.u16Illum = psMessage->uData.u16Data;
                eLockunLock(&psZigbeeNode->mutex);
            }
            break;
        case(E_ZB_CLUSTERID_TEMPERATURE):
            if(psZigbeeNode->sNode.u16DeviceID == 0){
                WAR_vPrintf(T_TRUE, "This node is not in network truly\n");
            }else{
                eLockLock(&psZigbeeNode->mutex);
                INF_vPrintf(DBG_ZCB, "update temp attribute to %d\n", psMessage->uData.u16Data);
                psZigbeeNode->sNode.sAttributeValue.u16Temp = psMessage->uData.u16Data;
                eLockunLock(&psZigbeeNode->mutex);
            }
            break;
        case(E_ZB_CLUSTERID_HUMIDITY):
            if(psZigbeeNode->sNode.u16DeviceID == 0){
                WAR_vPrintf(T_TRUE, "This node is not in network truly\n");
            }else{
                eLockLock(&psZigbeeNode->mutex);
                INF_vPrintf(DBG_ZCB, "update humi attribute to %d\n", psMessage->uData.u16Data);
                psZigbeeNode->sNode.sAttributeValue.u16Humi = psMessage->uData.u16Data;
                eLockunLock(&psZigbeeNode->mutex);
            }
            break;
        case(E_ZB_CLUSTERID_BINARY_INPUT_BASIC):
            if(psZigbeeNode->sNode.u16DeviceID == 0){
                WAR_vPrintf(T_TRUE, "This node is not in network truly\n");
            }else{
                eLockLock(&psZigbeeNode->mutex);
                INF_vPrintf(DBG_ZCB, "update binary attribute to %d\n", psMessage->uData.u8Data);
                psZigbeeNode->sNode.sAttributeValue.u8Binary = psMessage->uData.u8Data;
                eLockunLock(&psZigbeeNode->mutex);
            }
            break;
        case(E_ZB_CLUSTERID_POWER):
            if(psZigbeeNode->sNode.u16DeviceID == 0){
                WAR_vPrintf(T_TRUE, "This node is not in network truly\n");
            }else{
                eLockLock(&psZigbeeNode->mutex);
                INF_vPrintf(DBG_ZCB, "update power attribute to %d\n", psMessage->uData.u16Data);
                psZigbeeNode->sNode.sAttributeValue.u16Battery= psMessage->uData.u16Data;
                eLockunLock(&psZigbeeNode->mutex);
            }
            break;
        case(E_ZB_CLUSTERID_DOOR_LOCK):
            if(psZigbeeNode->sNode.u16DeviceID == 0){
                WAR_vPrintf(T_TRUE, "This node is not in network truly\n");
            }else{
                eLockLock(&psZigbeeNode->mutex);
                INF_vPrintf(DBG_ZCB, "update door lock attribute to %d\n", psMessage->uData.u8Data);
                psZigbeeNode->sNode.sAttributeValue.u8State= psMessage->uData.u8Data;
                eLockunLock(&psZigbeeNode->mutex);
            }
            break;
        default:
            WAR_vPrintf(T_TRUE, "unknow cluster id.\n");
    }
    return ;
}


static void vZCB_HandleAddGroupResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
}

static void vZCB_HandleRemoveGroupMembershipResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
}

static void vZCB_HandleRemoveSceneResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
}

/****************************************************************************/
/***                   Set Coordinator                                    ***/
/****************************************************************************/
teZbStatus eZCB_SetDeviceType(teModuleMode eModuleMode)
{
    uint8 u8ModuleMode = eModuleMode;
    DBG_vPrintf(DBG_ZCB, "Writing Module: Set Device Type: %d\n", eModuleMode);    
    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_SET_DEVICETYPE, sizeof(uint8), &u8ModuleMode, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

teZbStatus eZCB_SetChannelMask(uint32 u32ChannelMask)
{
    DBG_vPrintf(DBG_ZCB, "Setting channel mask: 0x%08X", u32ChannelMask);
    u32ChannelMask = htonl(u32ChannelMask);    
    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_SET_CHANNELMASK, sizeof(uint32), &u32ChannelMask, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

teZbStatus eZCB_SetExtendedPANID(uint64 u64PanID)
{
    u64PanID = htobe64(u64PanID);    
    CHECK_RESULT(eSL_SendMessage(E_SL_MSG_SET_EXT_PANID, sizeof(uint64), &u64PanID, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

teZbStatus eZCB_StartNetwork(void)
{
    DBG_vPrintf(DBG_ZCB, "Start network \n");
    CHECK_RESULT (eSL_SendMessage(E_SL_MSG_START_NETWORK, 0, NULL, NULL), E_SL_OK, E_ZB_COMMS_FAILED);
    return E_ZB_OK;
}

static teZbStatus eZCB_ConfigureControlBridge(void)
{
    #define CONFIGURATION_INTERVAL 500000
    usleep(CONFIGURATION_INTERVAL);
    /* Set up configuration */
    switch (eStartMode)
    {
        case(E_START_COORDINATOR):
            DBG_vPrintf(DBG_ZCB, "Starting control bridge as HA coordinator");
            eZCB_SetDeviceType(E_MODE_COORDINATOR);usleep(CONFIGURATION_INTERVAL);
            eZCB_SetChannelMask(eChannel);      usleep(CONFIGURATION_INTERVAL);
            eZCB_SetExtendedPANID(u64PanID);    usleep(CONFIGURATION_INTERVAL);
            eZCB_StartNetwork();                usleep(CONFIGURATION_INTERVAL);
            break;
    
        case (E_START_ROUTER):
            DBG_vPrintf(DBG_ZCB, "Starting control bridge as HA compatible router");
            eZCB_SetDeviceType(E_MODE_HA_COMPATABILITY);usleep(CONFIGURATION_INTERVAL);
            eZCB_SetChannelMask(eChannel);      usleep(CONFIGURATION_INTERVAL);
            eZCB_SetExtendedPANID(u64PanID);    usleep(CONFIGURATION_INTERVAL);
            eZCB_StartNetwork();                usleep(CONFIGURATION_INTERVAL);
            break;
    
        case (E_START_TOUCHLINK):
            DBG_vPrintf(DBG_ZCB, "Starting control bridge as ZLL router");
            eZCB_SetDeviceType(E_MODE_ROUTER);  usleep(CONFIGURATION_INTERVAL);
            eZCB_SetChannelMask(eChannel);      usleep(CONFIGURATION_INTERVAL);
            eZCB_SetExtendedPANID(u64PanID);    usleep(CONFIGURATION_INTERVAL);
            eZCB_StartNetwork();                usleep(CONFIGURATION_INTERVAL);
            break;
            
        default:
            ERR_vPrintf(T_TRUE,  "Unknown module mode\n");
            return E_ZB_ERROR;
    }
    
    return E_ZB_OK;
}


