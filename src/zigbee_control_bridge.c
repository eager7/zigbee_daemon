/****************************************************************************
 *
 * MODULE:             Linux 6LoWPAN Routing daemon
 *
 * COMPONENT:          Interface to module
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>

#include <syslog.h>

#include "zigbee_control_bridge.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZCB 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


static void ZCB_HandleNodeClusterList           (void *pvUser, uint16 u16Length, void *pvMessage);
static void ZCB_HandleNodeClusterAttributeList  (void *pvUser, uint16 u16Length, void *pvMessage);
static void ZCB_HandleNodeCommandIDList         (void *pvUser, uint16 u16Length, void *pvMessage);
static void ZCB_HandleRestartProvisioned        (void *pvUser, uint16 u16Length, void *pvMessage);
static void ZCB_HandleRestartFactoryNew         (void *pvUser, uint16 u16Length, void *pvMessage);
static teZbStatus eZCB_ConfigureControlBridge  (void);

static void ZCB_HandleNetworkJoined             (void *pvUser, uint16 u16Length, void *pvMessage);
static void ZCB_HandleDeviceAnnounce            (void *pvUser, uint16 u16Length, void *pvMessage);
static void ZCB_HandleMatchDescriptorResponse   (void *pvUser, uint16 u16Length, void *pvMessage);
static void ZCB_HandleAttributeReport           (void *pvUser, uint16 u16Length, void *pvMessage);
#if 0
#ifdef CLD_OTA_SERVER
static void ZCB_HandleOTARequest                (void *pvUser, uint16 u16Length, void *pvMessage);
#endif
#endif 

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/** Mode */
teStartMode      eStartMode         = CONFIG_DEFAULT_START_MODE;
teChannel        eChannel           = CONFIG_DEFAULT_CHANNEL;
uint64         u64PanID             = CONFIG_DEFAULT_PANID;

/* Network parameters in use */
teChannel        eChannelInUse      = 0;
uint64         u64PanIDInUse        = 0;
uint16         u16PanIDInUse        = 0;


/* APS Ack enabled by default */
int              bZCB_EnableAPSAck  = 1;
//uint16 au16ProfileZLL = E_ZB_PROFILEID_ZLL;
static uint16 au16ProfileHA = E_ZB_PROFILEID_HA;
static uint16 au16Cluster[] = {
                            E_ZB_CLUSTERID_ONOFF,                   /*Light*/
                            E_ZB_CLUSTERID_BINARY_INPUT_BASIC,      /*binary sensor*/
                            E_ZB_CLUSTERID_TEMPERATURE,             /*tempertuare*/
                            E_ZB_CLUSTERID_ILLUMINANCE              /*light sensor*/
                            };

extern int verbosity;
extern tsDeviceIDMap asDeviceIDMap[];

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static int iFlagAllowHandleReport = 0;

/** Firmware version of the connected device */
uint32 u32ZCB_SoftwareVersion = 0;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eZCB_Init()
{
    DBG_vPrintf(T_TRUE, "eZCB_Init\n");
    
    memset(&sZigbee_Network, 0, sizeof(tsZigbee_Network));
    mLockCreate(&sZigbee_Network.mutex);
    mLockCreate(&sZigbee_Network.sNodes.mutex);
    
    /* Register listeners */
    eSL_AddListener(E_SL_MSG_NODE_CLUSTER_LIST,         ZCB_HandleNodeClusterList,          NULL);
    eSL_AddListener(E_SL_MSG_NODE_ATTRIBUTE_LIST,       ZCB_HandleNodeClusterAttributeList, NULL);
    eSL_AddListener(E_SL_MSG_NODE_COMMAND_ID_LIST,      ZCB_HandleNodeCommandIDList,        NULL);
    eSL_AddListener(E_SL_MSG_NETWORK_JOINED_FORMED,     ZCB_HandleNetworkJoined,            NULL);
    eSL_AddListener(E_SL_MSG_DEVICE_ANNOUNCE,           ZCB_HandleDeviceAnnounce,           NULL);
    eSL_AddListener(E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE, ZCB_HandleMatchDescriptorResponse,  NULL);
    eSL_AddListener(E_SL_MSG_RESTART_PROVISIONED,       ZCB_HandleRestartProvisioned,       NULL);
    eSL_AddListener(E_SL_MSG_RESTART_FACTORY_NEW,       ZCB_HandleRestartFactoryNew,        NULL);
    eSL_AddListener(E_SL_MSG_ATTRIBUTE_REPORT,          ZCB_HandleAttributeReport,          NULL);
#ifdef CLD_OTA_SERVER
    eSL_AddListener(E_SL_MSG_OTA_IMAGE_REQUEST,         ZCB_HandleOTARequest,               NULL);//PCT OTA fun
#endif   
    
    DBG_vPrintf(T_TRUE, "eZCB_Init END\n");
    return E_ZB_OK;
}


teZbStatus eZCB_Finish(void)
{
    DBG_vPrintf(T_TRUE, "eZCB_Finish\n");
    while (sZigbee_Network.sNodes.psNext)
    {
        eZigbee_RemoveNode(sZigbee_Network.sNodes.psNext);
    }
    eZigbee_RemoveNode(&sZigbee_Network.sNodes);
    mLockDestroy(&sZigbee_Network.mutex);
    return E_ZB_OK;
}

teZbStatus eZCB_EstablishComms(void)
{
    if (eSL_SendMessage(E_SL_MSG_GET_VERSION, 0, NULL, NULL) == E_SL_OK)
    {
        uint16 u16Length;
        uint32  *u32Version;
        
        /* Wait 300ms for the versions message to arrive */
        if (eSL_MessageWait(E_SL_MSG_VERSION_LIST, 300, &u16Length, (void**)&u32Version) == E_SL_OK)
        {
            u32ZCB_SoftwareVersion = ntohl(*u32Version);
            DBG_vPrintf(verbosity, "Connected to control bridge version 0x%08x\n", u32ZCB_SoftwareVersion);
            free(u32Version);
            
            DBG_vPrintf(DBG_ZCB, "Reset control bridge\n");
            if (eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL) != E_SL_OK)
            {
                return E_ZB_COMMS_FAILED;
            }
            return E_ZB_OK;
        }
    }
    
    return E_ZB_COMMS_FAILED;
}

teZbStatus eZCB_FactoryNew(void)
{
    teSL_Status eStatus;
    DBG_vPrintf(verbosity, "Factory resetting control bridge\n");

    if ((eStatus = eSL_SendMessage(E_SL_MSG_ERASE_PERSISTENT_DATA, 0, NULL, NULL)) != E_SL_OK)
    {
        if (eStatus == E_SL_NOMESSAGE)
        {
            /* The erase persistent data command could take a while */
            uint16 u16Length;
            tsSL_Msg_Status *psStatus = NULL;
            
            eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 5000, &u16Length, (void**)&psStatus);
            
            if (eStatus == E_SL_OK)
            {
                eStatus = psStatus->eStatus;
                free(psStatus);
            }
            else
            {            
                return E_ZB_COMMS_FAILED;
            }
        }
        else
        {            
            return E_ZB_COMMS_FAILED;
        }
    }
    /* Wait for it to erase itself */
    sleep(1);

    DBG_vPrintf(DBG_ZCB, "Reset control bridge\n");
    if (eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    return E_ZB_OK;
}


teZbStatus eZCB_SetExtendedPANID(uint64 u64PanID)
{
    u64PanID = htobe64(u64PanID);
    
    if (eSL_SendMessage(E_SL_MSG_SET_EXT_PANID, sizeof(uint64), &u64PanID, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}


teZbStatus eZCB_SetChannelMask(uint32 u32ChannelMask)
{
    DBG_vPrintf(verbosity, "Setting channel mask: 0x%08X", u32ChannelMask);
    
    u32ChannelMask = htonl(u32ChannelMask);
    
    if (eSL_SendMessage(E_SL_MSG_SET_CHANNELMASK, sizeof(uint32), &u32ChannelMask, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}


teZbStatus eZCB_SetInitialSecurity(uint8 u8State, uint8 u8Sequence, uint8 u8Type, uint8 *pu8Key)
{
    uint8 au8Buffer[256];
    uint32 u32Position = 0;
    
    au8Buffer[u32Position] = u8State;
    u32Position += sizeof(uint8);
    
    au8Buffer[u32Position] = u8Sequence;
    u32Position += sizeof(uint8);
    
    au8Buffer[u32Position] = u8Type;
    u32Position += sizeof(uint8);
    
    switch (u8Type)
    {
        case (1):
            memcpy(&au8Buffer[u32Position], pu8Key, 16);
            u32Position += 16;
            break;
        default:
            ERR_vPrintf(T_TRUE, "Uknown key type %d\n", u8Type);
            return E_ZB_ERROR;
    }
    
    if (eSL_SendMessage(E_SL_MSG_SET_SECURITY, u32Position, au8Buffer, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    return E_ZB_OK;
}

teZbStatus eZCB_SetDeviceType(teModuleMode eModuleMode)
{
    uint8 u8ModuleMode = eModuleMode;
    
    if (verbosity >= LOG_DEBUG)
    {
        DBG_vPrintf(verbosity, "Writing Module: Set Device Type: %d\n", eModuleMode);
    }
    
    if (eSL_SendMessage(E_SL_MSG_SET_DEVICETYPE, sizeof(uint8), &u8ModuleMode, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }

    return E_ZB_OK;
}

teZbStatus eZCB_StartNetwork(void)
{
    DBG_vPrintf(DBG_ZCB, "Start network \n");
    if (eSL_SendMessage(E_SL_MSG_START_NETWORK, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}

teZbStatus eZCB_SetPermitJoining(uint8 u8Interval)
{
    struct _PermitJoiningMessage
    {
        uint16    u16TargetAddress;
        uint8     u8Interval;
        uint8     u8TCSignificance;
    } __attribute__((__packed__)) sPermitJoiningMessage;
    
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


/** Get permit joining status of the control bridge */
teZbStatus eZCB_GetPermitJoining(uint8 *pu8Status)
{   
    struct _GetPermitJoiningMessage
    {
        uint8     u8Status;
    } __attribute__((__packed__)) *psGetPermitJoiningResponse;
    uint16 u16Length;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    if (eSL_SendMessage(E_SL_MSG_GET_PERMIT_JOIN, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    /* Wait 1 second for the message to arrive */
    if (eSL_MessageWait(E_SL_MSG_GET_PERMIT_JOIN_RESPONSE, 1000, &u16Length, (void**)&psGetPermitJoiningResponse) != E_SL_OK)
    {
        if (verbosity > LOG_INFO)
        {
            DBG_vPrintf(verbosity, "No response to permit joining request\n");
        }
        goto done;
    }
    
    if (pu8Status)
    {
        *pu8Status = psGetPermitJoiningResponse->u8Status;
    }
        
    DBG_vPrintf(DBG_ZCB, "Permit joining Status: %d\n", psGetPermitJoiningResponse->u8Status);

    free(psGetPermitJoiningResponse);
    eStatus = E_ZB_OK;
done:
    return eStatus;
}


teZbStatus eZCB_SetWhitelistEnabled(uint8 bEnable)
{
    uint8 u8Enable = bEnable ? 1 : 0;
    
    DBG_vPrintf(DBG_ZCB, "%s whitelisting\n", bEnable ? "Enable" : "Disable");
    if (eSL_SendMessage(E_SL_MSG_NETWORK_WHITELIST_ENABLE, 1, &u8Enable, NULL) != E_SL_OK)
    {
        DBG_vPrintf(DBG_ZCB, "Failed\n");
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}


teZbStatus eZCB_AuthenticateDevice(uint64 u64IEEEAddress, uint8 *pau8LinkKey, 
                                    uint8 *pau8NetworkKey, uint8 *pau8MIC,
                                    uint64 *pu64TrustCenterAddress, uint8 *pu8KeySequenceNumber)
{
    struct _AuthenticateRequest
    {
        uint64    u64IEEEAddress;
        uint8     au8LinkKey[16];
    } __attribute__((__packed__)) sAuthenticateRequest;
    
    struct _AuthenticateResponse
    {
        uint64    u64IEEEAddress;
        uint8     au8NetworkKey[16];
        uint8     au8MIC[4];
        uint64    u64TrustCenterAddress;
        uint8     u8KeySequenceNumber;
        uint8     u8Channel;
        uint16    u16PanID;
        uint64    u64PanID;
    } __attribute__((__packed__)) *psAuthenticateResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    sAuthenticateRequest.u64IEEEAddress = htobe64(u64IEEEAddress);
    memcpy(sAuthenticateRequest.au8LinkKey, pau8LinkKey, 16);
    
    if (eSL_SendMessage(E_SL_MSG_AUTHENTICATE_DEVICE_REQUEST, sizeof(struct _AuthenticateRequest), &sAuthenticateRequest, &u8SequenceNo) == E_SL_OK)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_AUTHENTICATE_DEVICE_RESPONSE, 1000, &u16Length, (void**)&psAuthenticateResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                DBG_vPrintf(verbosity, "No response to authenticate request\n");
            }
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Got authentication data for device 0x%016llX\n", (unsigned long long int)u64IEEEAddress);
            
            psAuthenticateResponse->u64TrustCenterAddress = be64toh(psAuthenticateResponse->u64TrustCenterAddress);
            psAuthenticateResponse->u16PanID = ntohs(psAuthenticateResponse->u16PanID);
            psAuthenticateResponse->u64PanID = be64toh(psAuthenticateResponse->u64PanID);
            
            DBG_vPrintf(DBG_ZCB, "Trust center address: 0x%016llX\n", (unsigned long long int)psAuthenticateResponse->u64TrustCenterAddress);
            DBG_vPrintf(DBG_ZCB, "Key sequence number: %02d\n", psAuthenticateResponse->u8KeySequenceNumber);
            DBG_vPrintf(DBG_ZCB, "Channel: %02d\n", psAuthenticateResponse->u8Channel);
            DBG_vPrintf(DBG_ZCB, "Short PAN: 0x%04X\n", psAuthenticateResponse->u16PanID);
            DBG_vPrintf(DBG_ZCB, "Extended PAN: 0x%016llX\n", (unsigned long long int)psAuthenticateResponse->u64PanID);
                        
            memcpy(pau8NetworkKey, psAuthenticateResponse->au8NetworkKey, 16);
            memcpy(pau8MIC, psAuthenticateResponse->au8MIC, 4);
            memcpy(pu64TrustCenterAddress, &psAuthenticateResponse->u64TrustCenterAddress, 8);
            memcpy(pu8KeySequenceNumber, &psAuthenticateResponse->u8KeySequenceNumber, 1);
            
            eStatus = E_ZB_OK;
        }
    }

    free(psAuthenticateResponse);
    return eStatus;
}


/** Initiate Touchlink */
teZbStatus eZCB_ZLL_Touchlink(void)
{
    DBG_vPrintf(DBG_ZCB, "Initiate Touchlink\n");
    if (eSL_SendMessage(E_SL_MSG_INITIATE_TOUCHLINK, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}

/** Initiate Match descriptor request */
teZbStatus eZCB_MatchDescriptorRequest(uint16 u16TargetAddress, uint16 u16ProfileID, 
                                        uint8 u8NumInputClusters, uint16 *pau16InputClusters, 
                                        uint8 u8NumOutputClusters, uint16 *pau16OutputClusters,
                                        uint8 *pu8SequenceNo)
{
    uint8 au8Buffer[256];
    uint32 u32Position = 0;
    int i;
    
    DBG_vPrintf(DBG_ZCB, "Send Match Desciptor request for profile ID 0x%04X to 0x%04X\n", u16ProfileID, u16TargetAddress);

    u16TargetAddress = htons(u16TargetAddress);
    memcpy(&au8Buffer[u32Position], &u16TargetAddress, sizeof(uint16));
    u32Position += sizeof(uint16);
    
    u16ProfileID = htons(u16ProfileID);
    memcpy(&au8Buffer[u32Position], &u16ProfileID, sizeof(uint16));
    u32Position += sizeof(uint16);
    
    au8Buffer[u32Position] = u8NumInputClusters;
    u32Position++;
    
    DBG_vPrintf(DBG_ZCB, "  Input Cluster List:\n");
    
    for (i = 0; i < u8NumInputClusters; i++)
    {
        uint16 u16ClusterID = htons(pau16InputClusters[i]);
        DBG_vPrintf(DBG_ZCB, "    0x%04X\n", pau16InputClusters[i]);
        memcpy(&au8Buffer[u32Position], &u16ClusterID , sizeof(uint16));
        u32Position += sizeof(uint16);
    }
    
    DBG_vPrintf(DBG_ZCB, "  Output Cluster List:\n");
    
    au8Buffer[u32Position] = u8NumOutputClusters;
    u32Position++;
    
    for (i = 0; i < u8NumOutputClusters; i++)
    {
        uint16 u16ClusterID = htons(pau16OutputClusters[i] );
        DBG_vPrintf(DBG_ZCB, "    0x%04X\n", pau16OutputClusters[i]);
        memcpy(&au8Buffer[u32Position], &u16ClusterID , sizeof(uint16));
        u32Position += sizeof(uint16);
    }

    if (eSL_SendMessage(E_SL_MSG_MATCH_DESCRIPTOR_REQUEST, u32Position, au8Buffer, pu8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }

    return E_ZB_OK;
}

teZbStatus eZCB_SendMatchDescriptorRequest(void)
{
    if(eZCB_MatchDescriptorRequest(E_ZB_BROADCAST_ADDRESS_RXONWHENIDLE, au16ProfileHA,
                                sizeof(au16Cluster) / sizeof(uint16), au16Cluster, 
                                0, NULL, NULL) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "Error sending match descriptor request\n");
        return E_ZB_ERROR;
    }

    return E_ZB_OK;
}

teZbStatus eZCB_NeighbourTableRequest(tsZigbee_Node *psZigbeeNode)
{
    struct _ManagementLQIRequest
    {
        uint16    u16TargetAddress;
        uint8     u8StartIndex;
    } __attribute__((__packed__)) sManagementLQIRequest;
    
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
            } __attribute__((__packed__)) sBitmap;
        } __attribute__((__packed__)) asNeighbours[255];
    } __attribute__((__packed__)) *psManagementLQIResponse = NULL;

    uint16 u16ShortAddress;
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    int i;
    
    u16ShortAddress = psZigbeeNode->u16ShortAddress;
    
    /* Unlock the node during this process, because it can take time, and we don't want to be holding a node lock when 
     * attempting to lock the list of nodes - that leads to deadlocks with the JIP server thread. */
    //eUtils_LockUnlock(&psZigbeeNode->sLock);
    
    sManagementLQIRequest.u16TargetAddress = htons(u16ShortAddress);
    sManagementLQIRequest.u8StartIndex      = psZigbeeNode->u8LastNeighbourTableIndex;
    
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
            if (verbosity > LOG_INFO)
            {
                DBG_vPrintf(verbosity, "No response to management LQI requestn\n");
            }
            goto done;
        }
        else if (u8SequenceNo == psManagementLQIResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psManagementLQIResponse->u8SequenceNo, u8SequenceNo);
            free(psManagementLQIResponse);
            psManagementLQIResponse = NULL;
        }
    }
    if(NULL != psManagementLQIResponse)
    {
        DBG_vPrintf(DBG_ZCB, "Received management LQI response. Table size: %d, Entry count: %d, start index: %d\n",
                    psManagementLQIResponse->u8NeighbourTableSize,
                    psManagementLQIResponse->u8TableEntries,
                    psManagementLQIResponse->u8StartIndex);
    }
    
    for (i = 0; i < psManagementLQIResponse->u8TableEntries; i++)
    {
        tsZigbee_Node *ZigbeeNodeTemp;
        //tsZcbEvent *psEvent;
        
        psManagementLQIResponse->asNeighbours[i].u16ShortAddress    = ntohs(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);
        psManagementLQIResponse->asNeighbours[i].u64PanID           = be64toh(psManagementLQIResponse->asNeighbours[i].u64PanID);
        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress     = be64toh(psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);
        
        DBG_vPrintf(DBG_ZCB, "  Entry %02d: Short Address 0x%04X, PAN ID: 0x%016llX, IEEE Address: 0x%016llX\n", i,
                    psManagementLQIResponse->asNeighbours[i].u16ShortAddress,
                    psManagementLQIResponse->asNeighbours[i].u64PanID,
                    psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);
        
        DBG_vPrintf(DBG_ZCB, "    Type: %d, Permit Joining: %d, Relationship: %d, RxOnWhenIdle: %d\n",
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uDeviceType,
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uPermitJoining,
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uRelationship,
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability);
        
        DBG_vPrintf(DBG_ZCB, "    Depth: %d, LQI: %d\n", 
                    psManagementLQIResponse->asNeighbours[i].u8Depth, 
                    psManagementLQIResponse->asNeighbours[i].u8LQI);
        
        ZigbeeNodeTemp = psZigbee_FindNodeByShortAddress(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);
        
        if (!ZigbeeNodeTemp)
        {
            DBG_vPrintf(DBG_ZCB, "New Node 0x%04X in neighbour table\n", psManagementLQIResponse->asNeighbours[i].u16ShortAddress);

            if ((eStatus = eZigbee_AddNode(psManagementLQIResponse->asNeighbours[i].u16ShortAddress, 
                                        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress, 
                                        0x0000, psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability ? E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE : 0, NULL)) != E_ZB_OK)
            {
                DBG_vPrintf(DBG_ZCB, "Error adding node to network\n");
                break;
            }
        }
    }
    
    if (psManagementLQIResponse->u8TableEntries > 0)
    {
        // We got some entries, so next time request the entries after these.
        psZigbeeNode->u8LastNeighbourTableIndex += psManagementLQIResponse->u8TableEntries;
    }
    else
    {
        // No more valid entries.
        psZigbeeNode->u8LastNeighbourTableIndex = 0;
    }

    eStatus = E_ZB_OK;
done:
    psZigbeeNode = psZigbee_FindNodeByShortAddress(u16ShortAddress);
    mLockLock(&psZigbeeNode->mutex);
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    mLockUnlock(&psZigbeeNode->mutex);
    free(psManagementLQIResponse);
    return eStatus;
}


teZbStatus eZCB_IEEEAddressRequest(tsZigbee_Node *psZigbee_Node)
{
    struct _IEEEAddressRequest
    {
        uint16    u16TargetAddress;
        uint16    u16ShortAddress;
        uint8     u8RequestType;
        uint8     u8StartIndex;
    } __attribute__((__packed__)) sIEEEAddressRequest;
    
    struct _IEEEAddressResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint64    u64IEEEAddress;
        uint16    u16ShortAddress;
        uint8     u8NumAssociatedDevices;
        uint8     u8StartIndex;
        uint16    au16DeviceList[255];
    } __attribute__((__packed__)) *psIEEEAddressResponse = NULL;

    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send IEEE Address request to 0x%04X\n", psZigbee_Node->u16ShortAddress);
    
    sIEEEAddressRequest.u16TargetAddress    = htons(psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u16ShortAddress     = htons(psZigbee_Node->u16ShortAddress);
    sIEEEAddressRequest.u8RequestType       = 0;
    sIEEEAddressRequest.u8StartIndex        = 0;
    
    if (eSL_SendMessage(E_SL_MSG_IEEE_ADDRESS_REQUEST, sizeof(struct _IEEEAddressRequest), &sIEEEAddressRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_IEEE_ADDRESS_RESPONSE, 5000, &u16Length, (void**)&psIEEEAddressResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                DBG_vPrintf(verbosity, "No response to IEEE address request\n");
            }
            goto done;
        }
        if (u8SequenceNo == psIEEEAddressResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psIEEEAddressResponse->u8SequenceNo, u8SequenceNo);
            free(psIEEEAddressResponse);
            psIEEEAddressResponse = NULL;
        }
    }
    if(NULL != psIEEEAddressResponse)
    {
        psZigbee_Node->u64IEEEAddress = be64toh(psIEEEAddressResponse->u64IEEEAddress);
    }
    
    DBG_vPrintf(DBG_ZCB, "Short address 0x%04X has IEEE Address 0x%016llX\n", 
        psZigbee_Node->u16ShortAddress, (unsigned long long int)psZigbee_Node->u64IEEEAddress);
    eStatus = E_ZB_OK;

done:
    vZigbee_NodeUpdateComms(psZigbee_Node, eStatus);
    free(psIEEEAddressResponse);
    return eStatus;
}


teZbStatus eZCB_NodeDescriptorRequest(tsZigbee_Node *psZigbee_Node)
{
    struct _NodeDescriptorRequest
    {
        uint16    u16TargetAddress;
    } __attribute__((__packed__)) sNodeDescriptorRequest;
    
    struct _tNodeDescriptorResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint16    u16ManufacturerID;
        uint16    u16MaxRxLength;
        uint16    u16MaxTxLength;
        uint16    u16ServerMask;
        uint8     u8DescriptorCapability;
        uint8     u8MacCapability;
        uint8     u8MaxBufferSize;
        uint16    u16Bitfield;
    } __attribute__((__packed__)) *psNodeDescriptorResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Node Descriptor request to 0x%04X\n", psZigbee_Node->u16ShortAddress);
    
    sNodeDescriptorRequest.u16TargetAddress     = htons(psZigbee_Node->u16ShortAddress);
    
    if (eSL_SendMessage(E_SL_MSG_NODE_DESCRIPTOR_REQUEST, 
                        sizeof(struct _NodeDescriptorRequest), &sNodeDescriptorRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1) 
    {
        /* Wait 1 second for the node message to arrive */
        if (eSL_MessageWait(E_SL_MSG_NODE_DESCRIPTOR_RESPONSE, 5000, &u16Length, (void**)&psNodeDescriptorResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                DBG_vPrintf(verbosity, "No response to node descriptor request\n");
            }
            goto done;
        }
    
        if (u8SequenceNo == psNodeDescriptorResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            free(psNodeDescriptorResponse);
            psNodeDescriptorResponse = NULL;
        }
    }
    if(NULL != psNodeDescriptorResponse)
        psZigbee_Node->u8MacCapability = psNodeDescriptorResponse->u8MacCapability;
    
    DBG_PrintNode(psZigbee_Node);
    eStatus = E_ZB_OK;
done:
    vZigbee_NodeUpdateComms(psZigbee_Node, eStatus);
    free(psNodeDescriptorResponse);
    return eStatus;
}


teZbStatus eZCB_SimpleDescriptorRequest(tsZigbee_Node *psZigbee_Node, uint8 u8Endpoint)
{
    struct _SimpleDescriptorRequest
    {
        uint16    u16TargetAddress;
        uint8     u8Endpoint;
    } __attribute__((__packed__)) sSimpleDescriptorRequest;
    
    struct _tSimpleDescriptorResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint8     u8Length;
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16DeviceID;
        uint8     u8Bitfields;
        uint8     u8InputClusterCount;
        /* Input Clusters */
        /* uint8     u8OutputClusterCount;*/
        /* Output Clusters */
    } __attribute__((__packed__)) *psSimpleDescriptorResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    int iPosition, i;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Simple Desciptor request for Endpoint %d to 0x%04X\n", u8Endpoint, psZigbee_Node->u16ShortAddress);
    
    sSimpleDescriptorRequest.u16TargetAddress       = htons(psZigbee_Node->u16ShortAddress);
    sSimpleDescriptorRequest.u8Endpoint             = u8Endpoint;
    
    if (eSL_SendMessage(E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST, 
        sizeof(struct _SimpleDescriptorRequest), &sSimpleDescriptorRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1) 
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE, 5000, &u16Length, (void**)&psSimpleDescriptorResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                DBG_vPrintf(verbosity, "No response to simple descriptor request\n");
            }
            goto done;
        }
    
        if (u8SequenceNo == psSimpleDescriptorResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            free(psSimpleDescriptorResponse);
            psSimpleDescriptorResponse = NULL;
        }
    }
    
    /* Set device ID */
    if(NULL != psSimpleDescriptorResponse)
        psZigbee_Node->u16DeviceID = ntohs(psSimpleDescriptorResponse->u16DeviceID);
    
    if (eZigbee_NodeAddEndpoint(psZigbee_Node, psSimpleDescriptorResponse->u8Endpoint, 
            ntohs(psSimpleDescriptorResponse->u16ProfileID), NULL) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "eZigbee_NodeAddEndpoint error\n");
        eStatus = E_ZB_ERROR;
        goto done;
    }

    iPosition = sizeof(struct _tSimpleDescriptorResponse);
    for (i = 0; (i < psSimpleDescriptorResponse->u8InputClusterCount) && (iPosition < u16Length); i++)
    {
        uint16 *psClusterID = (uint16 *)&((uint8*)psSimpleDescriptorResponse)[iPosition];
        if (eZigbee_NodeAddCluster(psZigbee_Node, psSimpleDescriptorResponse->u8Endpoint, ntohs(*psClusterID)) != E_ZB_OK)
        {
            ERR_vPrintf(T_TRUE, "eZigbee_NodeAddCluster error\n");
            eStatus = E_ZB_ERROR;
            goto done;
        }
        iPosition += sizeof(uint16);
    }
    eStatus = E_ZB_OK;
done:
    vZigbee_NodeUpdateComms(psZigbee_Node, eStatus);
    free(psSimpleDescriptorResponse);
    return eStatus;
}


teZbStatus eZCB_ReadAttributeRequest(tsZigbee_Node *psZigbee_Node, uint16 u16ClusterID,
                                      uint8 u8Direction, uint8 u8ManufacturerSpecific, uint16 u16ManufacturerID,
                                      uint16 u16AttributeID, void *pvData)
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
    } __attribute__((__packed__)) sReadAttributeRequest;
    
    struct _ReadAttributeResponse
    {
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;//PCT
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8Status;
        uint8     u8Type;
        union
        {
            uint8     u8Data;
            uint16    u16Data;
            uint32    u32Data;
            uint64    u64Data;
        } uData;
    } __attribute__((__packed__)) *psReadAttributeResponse = NULL;
    
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
    
    if (eSL_SendMessage(E_SL_MSG_READ_ATTRIBUTE_REQUEST, 
            sizeof(struct _ReadAttributeRequest), &sReadAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    DBG_vPrintf(DBG_ZCB, "Wait 2.5 second for the message to arrive\n");
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_READ_ATTRIBUTE_RESPONSE, 2500, &u16Length, (void**)&psReadAttributeResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                DBG_vPrintf(verbosity, "No response to read attribute request\n");
            }
            eStatus = E_ZB_COMMS_FAILED;
            goto done;
        }
        
        if (u8SequenceNo == psReadAttributeResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            ERR_vPrintf(T_TRUE, "Read Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", 
                    psReadAttributeResponse->u8SequenceNo, u8SequenceNo);
            free(psReadAttributeResponse);
            psReadAttributeResponse = NULL;
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
    if(E_ZB_OK == eStatus)
        vZigbee_NodeUpdateClusterComms(psZigbee_Node, u16ClusterID);
done:
    vZigbee_NodeUpdateComms(psZigbee_Node, eStatus);
    
    free(psReadAttributeResponse);   
    return eStatus;
}


teZbStatus eZCB_WriteAttributeRequest(tsZigbee_Node *psZigbee_Node, uint16 u16ClusterID,
                                      uint8 u8Direction, uint8 u8ManufacturerSpecific, uint16 u16ManufacturerID,
                                      uint16 u16AttributeID, teZCL_ZCLAttributeType eType, void *pvData)
{
    struct _WriteAttributeRequest
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
        uint16    u16AttributeID;
        uint8     u8Type;
        union
        {
            uint8     u8Data;
            uint16    u16Data;
            uint32    u32Data;
            uint64    u64Data;
        } uData;
    } __attribute__((__packed__)) sWriteAttributeRequest;
#if   0  
    struct _WriteAttributeResponse
    {
        /**\todo handle default response properly */
        uint8     au8ZCLHeader[3];
        uint16    u16MessageType;
        
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8Status;
        uint8     u8Type;
        union
        {
            uint8     u8Data;
            uint16    u16Data;
            uint32    u32Data;
            uint64    u64Data;
        } uData;
    } __attribute__((__packed__)) *psWriteAttributeResponse = NULL;
#endif
    
    struct _DataIndication
    {
        /**\todo handle data indication properly */
        uint8     u8ZCBStatus;
        uint16    u16ProfileID;
        uint16    u16ClusterID;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8SourceAddressMode;
        uint16    u16SourceShortAddress; /* OR uint64 u64IEEEAddress */
        uint8     u8DestinationAddressMode;
        uint16    u16DestinationShortAddress; /* OR uint64 u64IEEEAddress */
        
        uint8     u8FrameControl;
        uint8     u8SequenceNo;
        uint8     u8Command;
        uint8     u8Status;
        uint16    u16AttributeID;
    } __attribute__((__packed__)) *psDataIndication = NULL;

    
    uint16 u16Length = sizeof(struct _WriteAttributeRequest) - sizeof(sWriteAttributeRequest.uData);
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Write Attribute request to 0x%04X\n", psZigbee_Node->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sWriteAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sWriteAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sWriteAttributeRequest.u16TargetAddress      = htons(psZigbee_Node->u16ShortAddress);
    
    if ((eStatus = eZigbee_GetEndpoints(psZigbee_Node, u16ClusterID, 
            &sWriteAttributeRequest.u8SourceEndpoint, &sWriteAttributeRequest.u8DestinationEndpoint)) != E_ZB_OK)
    {
        goto done;
    }
    
    sWriteAttributeRequest.u16ClusterID             = htons(u16ClusterID);
    sWriteAttributeRequest.u8Direction              = u8Direction;
    sWriteAttributeRequest.u8ManufacturerSpecific   = u8ManufacturerSpecific;
    sWriteAttributeRequest.u16ManufacturerID        = htons(u16ManufacturerID);
    sWriteAttributeRequest.u8NumAttributes          = 1;
    sWriteAttributeRequest.u16AttributeID           = htons(u16AttributeID);
    sWriteAttributeRequest.u8Type                   = (uint8)eType;
    
    switch(eType)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            memcpy(&sWriteAttributeRequest.uData.u8Data, pvData, sizeof(uint8));
			u16Length += sizeof(uint8);
            break;
        
        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            memcpy(&sWriteAttributeRequest.uData.u16Data, pvData, sizeof(uint16));
            sWriteAttributeRequest.uData.u16Data = ntohs(sWriteAttributeRequest.uData.u16Data);
			u16Length += sizeof(uint16);
            break;
 
        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            memcpy(&sWriteAttributeRequest.uData.u32Data, pvData, sizeof(uint32));
            sWriteAttributeRequest.uData.u32Data = ntohl(sWriteAttributeRequest.uData.u32Data);
			u16Length += sizeof(uint32);
            break;
 
        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            memcpy(&sWriteAttributeRequest.uData.u64Data, pvData, sizeof(uint64));
            sWriteAttributeRequest.uData.u64Data = be64toh(sWriteAttributeRequest.uData.u64Data);
			u16Length += sizeof(uint64);
            break;
            
        default:
            ERR_vPrintf(T_TRUE,  "Unknown attribute data type (%d)", eType);
            return E_ZB_ERROR;
    }
    
    if (eSL_SendMessage(E_SL_MSG_WRITE_ATTRIBUTE_REQUEST, u16Length, &sWriteAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        /**\todo handle data indication here for now - BAD Idea! Implement a general case handler in future! */
        if (eSL_MessageWait(E_SL_MSG_DATA_INDICATION, 1000, &u16Length, (void**)&psDataIndication) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                DBG_vPrintf(verbosity, "No response to write attribute request\n");
            }
            eStatus = E_ZB_COMMS_FAILED;
            goto done;
        }
        
        DBG_vPrintf(DBG_ZCB, "Got data indication\n");
        
        if (u8SequenceNo == psDataIndication->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Write Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", 
                psDataIndication->u8SequenceNo, u8SequenceNo);
            free(psDataIndication);
            psDataIndication = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Got write attribute response\n");
    
    eStatus = psDataIndication->u8Status;

done:
    vZigbee_NodeUpdateComms(psZigbee_Node, eStatus);
    free(psDataIndication);
    return eStatus;
}


teZbStatus eZCB_GetDefaultResponse(uint8 u8SequenceNo)
{
    uint16 u16Length;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    tsSL_Msg_DefaultResponse *psDefaultResponse = NULL;

    while (1)
    {
        /* Wait 1 second for a default response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_DEFAULT_RESPONSE, 2500, &u16Length, (void**)&psDefaultResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to command sequence number %d received", u8SequenceNo);
            goto done;
        }
        
        if (u8SequenceNo != psDefaultResponse->u8SequenceNo)
        {
            DBG_vPrintf(DBG_ZCB, "Default response sequence number received 0x%02X does not match that sent 0x%02X\n", 
                psDefaultResponse->u8SequenceNo, u8SequenceNo);
            free(psDefaultResponse);
            psDefaultResponse = NULL;
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
    free(psDefaultResponse);
    return eStatus;
}


teZbStatus eZCB_AddGroupMembership(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress)
{
    struct _AddGroupMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
    } __attribute__((__packed__)) sAddGroupMembershipRequest;
    
    struct _sAddGroupMembershipResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
    } __attribute__((__packed__)) *psAddGroupMembershipResponse = NULL;
    
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

    if (eSL_SendMessage(E_SL_MSG_ADD_GROUP_REQUEST, 
            sizeof(struct _AddGroupMembershipRequest), &sAddGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
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
            free(psAddGroupMembershipResponse);
            psAddGroupMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Add group membership 0x%04X on Node 0x%04X status: %d\n", 
        u16GroupAddress, psZigbeeNode->u16ShortAddress, psAddGroupMembershipResponse->u8Status);
    
    eStatus = psAddGroupMembershipResponse->u8Status;

done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psAddGroupMembershipResponse);
    return eStatus;
}


teZbStatus eZCB_RemoveGroupMembership(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress)
{
    struct _RemoveGroupMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
    } __attribute__((__packed__)) sRemoveGroupMembershipRequest;
    
    struct _sRemoveGroupMembershipResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
    } __attribute__((__packed__)) *psRemoveGroupMembershipResponse = NULL;
    
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

    if (eSL_SendMessage(E_SL_MSG_REMOVE_GROUP_REQUEST, 
            sizeof(struct _RemoveGroupMembershipRequest), &sRemoveGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
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
            free(psRemoveGroupMembershipResponse);
            psRemoveGroupMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Remove group membership 0x%04X on Node 0x%04X status: %d\n", 
            u16GroupAddress, psZigbeeNode->u16ShortAddress, psRemoveGroupMembershipResponse->u8Status);
    
    eStatus = psRemoveGroupMembershipResponse->u8Status;

done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psRemoveGroupMembershipResponse);
    return eStatus;
}


teZbStatus eZCB_GetGroupMembership(tsZigbee_Node *psZigbeeNode)
{
    struct _GetGroupMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8GroupCount;
        uint16    au16GroupList[0];
    } __attribute__((__packed__)) sGetGroupMembershipRequest;
    
    struct _sGetGroupMembershipResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Capacity;
        uint8     u8GroupCount;
        uint16    au16GroupList[255];
    } __attribute__((__packed__)) *psGetGroupMembershipResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    int i;
    
    DBG_vPrintf(DBG_ZCB, "Send get group membership request to 0x%04X\n", psZigbeeNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sGetGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sGetGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sGetGroupMembershipRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);
    
    if (eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_GROUPS, 
            &sGetGroupMembershipRequest.u8SourceEndpoint, &sGetGroupMembershipRequest.u8DestinationEndpoint) != E_ZB_OK)
    {
        return E_ZB_ERROR;
    }
    
    sGetGroupMembershipRequest.u8GroupCount     = 0;

    if (eSL_SendMessage(E_SL_MSG_GET_GROUP_MEMBERSHIP_REQUEST, 
            sizeof(struct _GetGroupMembershipRequest), &sGetGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE, 1000, &u16Length, (void**)&psGetGroupMembershipResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to group membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Get group membership sequence number received 0x%02X does not match that sent 0x%02X\n", 
                    psGetGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            free(psGetGroupMembershipResponse);
            psGetGroupMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Node 0x%04X is in %d/%d groups\n", psZigbeeNode->u16ShortAddress,
                psGetGroupMembershipResponse->u8GroupCount, 
                psGetGroupMembershipResponse->u8GroupCount + psGetGroupMembershipResponse->u8Capacity);

    if (eZigbee_NodeClearGroups(psZigbeeNode) != E_ZB_OK)
    {
        goto done;
    }
    
    for(i = 0; i < psGetGroupMembershipResponse->u8GroupCount; i++)
    {
        psGetGroupMembershipResponse->au16GroupList[i] = ntohs(psGetGroupMembershipResponse->au16GroupList[i]);
        DBG_vPrintf(DBG_ZCB, "  Group ID 0x%04X\n", psGetGroupMembershipResponse->au16GroupList[i]);
        if ((eStatus = eZigbee_NodeAddGroup(psZigbeeNode, psGetGroupMembershipResponse->au16GroupList[i])) != E_ZB_OK)
        {
            goto done;
        }
    }
    eStatus = E_ZB_OK;
done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psGetGroupMembershipResponse);
    return eStatus;
}


teZbStatus eZCB_ClearGroupMembership(tsZigbee_Node *psZigbeeNode)
{
    struct _ClearGroupMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
    } __attribute__((__packed__)) sClearGroupMembershipRequest;
    
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


teZbStatus eZCB_RemoveScene(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID)
{
    struct _RemoveSceneRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } __attribute__((__packed__)) sRemoveSceneRequest;
    
    struct _sStoreSceneResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } __attribute__((__packed__)) *psRemoveSceneResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;


    if (psZigbeeNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send remove scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n", 
                    u8SceneID, u16GroupAddress, sRemoveSceneRequest.u8DestinationEndpoint, psZigbeeNode->u16ShortAddress);
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
    sRemoveSceneRequest.u8SceneID        = u8SceneID;

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
            free(psRemoveSceneResponse);
            psRemoveSceneResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Remove scene %d (Group0x%04X) on Node 0x%04X status: %d\n", 
                psRemoveSceneResponse->u8SceneID, ntohs(psRemoveSceneResponse->u16GroupAddress), 
                psZigbeeNode->u16ShortAddress, psRemoveSceneResponse->u8Status);
    
    eStatus = psRemoveSceneResponse->u8Status;
done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psRemoveSceneResponse);
    return eStatus;
}


teZbStatus eZCB_StoreScene(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID)
{
    struct _StoreSceneRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } __attribute__((__packed__)) sStoreSceneRequest;
    
    struct _sStoreSceneResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint16    u16GroupAddress;
        uint8     u8SceneID;
    } __attribute__((__packed__)) *psStoreSceneResponse = NULL;
    
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
            free(psStoreSceneResponse);
            psStoreSceneResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Store scene %d (Group0x%04X) on Node 0x%04X status: %d\n", 
                psStoreSceneResponse->u8SceneID, ntohs(psStoreSceneResponse->u16GroupAddress), 
                ntohs(psZigbeeNode->u16ShortAddress), psStoreSceneResponse->u8Status);
    
    eStatus = psStoreSceneResponse->u8Status;
done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psStoreSceneResponse);
    return eStatus;
}


teZbStatus eZCB_RecallScene(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID)
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
    } __attribute__((__packed__)) sRecallSceneRequest;

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
       // DBG_vPrintf(DBG_ZCB, "Send recall scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n", 
                //u8SceneID, u16GroupAddress, sRecallSceneRequest.u8DestinationEndpoint, u16GroupAddress);
        
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


teZbStatus eZCB_GetSceneMembership(tsZigbee_Node *psZigbeeNode, 
    uint16 u16GroupAddress, uint8 *pu8NumScenes, uint8 **pau8Scenes)
{
    if (!pu8NumScenes)
    {
        return E_ZB_ERROR_NO_MEM;
    }
    struct _GetSceneMembershipRequest
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16GroupAddress;
    } __attribute__((__packed__)) sGetSceneMembershipRequest;
    
    struct _sGetSceneMembershipResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint8     u8Status;
        uint8     u8Capacity;
        uint16    u16GroupAddress;
        uint8     u8NumScenes;
        uint8     au8Scenes[255];
    } __attribute__((__packed__)) *psGetSceneMembershipResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send get scene membership for group 0x%04X to 0x%04X\n", 
                u16GroupAddress, psZigbeeNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sGetSceneMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sGetSceneMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sGetSceneMembershipRequest.u16TargetAddress     = htons(psZigbeeNode->u16ShortAddress);
    
    if (eZigbee_GetEndpoints(psZigbeeNode, E_ZB_CLUSTERID_SCENES, 
            &sGetSceneMembershipRequest.u8SourceEndpoint, &sGetSceneMembershipRequest.u8DestinationEndpoint) != E_ZB_OK)
    {
        return E_ZB_ERROR;
    }
    
    sGetSceneMembershipRequest.u16GroupAddress  = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_SCENE_MEMBERSHIP_REQUEST, 
            sizeof(struct _GetSceneMembershipRequest), &sGetSceneMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the response to arrive */
        if (eSL_MessageWait(E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE, 1000, &u16Length, (void**)&psGetSceneMembershipResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to get scene membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Get scene membership sequence number received 0x%02X does not match that sent 0x%02X\n", 
                psGetSceneMembershipResponse->u8SequenceNo, u8SequenceNo);
            free(psGetSceneMembershipResponse);
            psGetSceneMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Scene membership for group 0x%04X on Node 0x%04X status: %d\n", 
                ntohs(psGetSceneMembershipResponse->u16GroupAddress), psZigbeeNode->u16ShortAddress, psGetSceneMembershipResponse->u8Status);
    
    eStatus = psGetSceneMembershipResponse->u8Status;
    
    if (eStatus == E_ZB_OK)
    {
        int i;
        DBG_vPrintf(DBG_ZCB, "Node 0x%04X, group 0x%04X is in %d scenes\n", 
                psZigbeeNode->u16ShortAddress, ntohs(psGetSceneMembershipResponse->u16GroupAddress), psGetSceneMembershipResponse->u8NumScenes);
    
        *pu8NumScenes = psGetSceneMembershipResponse->u8NumScenes;

        *pau8Scenes = realloc(*pau8Scenes, *pu8NumScenes * sizeof(uint8));
        if (!(*pau8Scenes))
        {
            return E_ZB_ERROR_NO_MEM;
        }
        
        for (i = 0; i < psGetSceneMembershipResponse->u8NumScenes; i++)
        {
            DBG_vPrintf(DBG_ZCB, "  Scene 0x%02X\n", psGetSceneMembershipResponse->au8Scenes[i]);
            (*pau8Scenes)[i] = psGetSceneMembershipResponse->au8Scenes[i];
        }
    }
    
done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psGetSceneMembershipResponse);
    return eStatus;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static void ZCB_HandleNodeClusterList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(DBG_ZCB,"Handle 0x8003 message, ZCB_HandleNodeClusterList\n");
    int iPosition;
    int iCluster = 0;
    struct _tsClusterList
    {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    au16ClusterList[255];
    } __attribute__((__packed__)) *psClusterList = (struct _tsClusterList *)pvMessage;
    
    psClusterList->u16ProfileID = ntohs(psClusterList->u16ProfileID);
    
    DBG_vPrintf(DBG_ZCB, "Cluster list for endpoint %d, profile ID 0x%4X\n", 
                psClusterList->u8Endpoint, 
                psClusterList->u16ProfileID);
    
    mLockLock(&sZigbee_Network.sNodes.mutex);

    if (eZigbee_NodeAddEndpoint(&sZigbee_Network.sNodes, psClusterList->u8Endpoint, psClusterList->u16ProfileID, NULL) != E_ZB_OK)
    {
        goto done;
    }

    iPosition = sizeof(uint8) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbee_NodeAddCluster(&sZigbee_Network.sNodes, psClusterList->u8Endpoint, ntohs(psClusterList->au16ClusterList[iCluster])) != E_ZB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16);
        iCluster++;
    }
    
    //DBG_PrintNode(&tsZigbee_Network.sNodes);
done:
    mLockUnlock(&sZigbee_Network.sNodes.mutex);
}


static void ZCB_HandleNodeClusterAttributeList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x8004 message, ZCB_HandleNodeClusterAttributeList\n");
    int iPosition;
    int iAttribute = 0;
    struct _tsClusterAttributeList
    {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16ClusterID;
        uint16    au16AttributeList[255];
    } __attribute__((__packed__)) *psClusterAttributeList = (struct _tsClusterAttributeList *)pvMessage;
    
    psClusterAttributeList->u16ProfileID = ntohs(psClusterAttributeList->u16ProfileID);
    psClusterAttributeList->u16ClusterID = ntohs(psClusterAttributeList->u16ClusterID);
    
    DBG_vPrintf(DBG_ZCB, "Cluster attribute list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n", 
                psClusterAttributeList->u8Endpoint, 
                psClusterAttributeList->u16ClusterID,
                psClusterAttributeList->u16ProfileID);
    
    mLockLock(&sZigbee_Network.sNodes.mutex);

    iPosition = sizeof(uint8) + sizeof(uint16) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbee_NodeAddAttribute(&sZigbee_Network.sNodes, psClusterAttributeList->u8Endpoint, 
            psClusterAttributeList->u16ClusterID, ntohs(psClusterAttributeList->au16AttributeList[iAttribute])) != E_ZB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16);
        iAttribute++;
    }    

    //DBG_PrintNode(&tsZigbee_Network.sNodes);
    
done:
    mLockUnlock(&sZigbee_Network.sNodes.mutex);
}


static void ZCB_HandleNodeCommandIDList(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x8005 message, ZCB_HandleNodeClusterAttributeList\n");
    int iPosition;
    int iCommand = 0;
    struct _tsCommandIDList
    {
        uint8     u8Endpoint;
        uint16    u16ProfileID;
        uint16    u16ClusterID;
        uint8     au8CommandList[255];
    } __attribute__((__packed__)) *psCommandIDList = (struct _tsCommandIDList *)pvMessage;
    
    psCommandIDList->u16ProfileID = ntohs(psCommandIDList->u16ProfileID);
    psCommandIDList->u16ClusterID = ntohs(psCommandIDList->u16ClusterID);
    
    DBG_vPrintf(DBG_ZCB, "Command ID list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n", 
                psCommandIDList->u8Endpoint, 
                psCommandIDList->u16ClusterID,
                psCommandIDList->u16ProfileID);
    
    mLockLock(&sZigbee_Network.sNodes.mutex);
    
    iPosition = sizeof(uint8) + sizeof(uint16) + sizeof(uint16);
    while(iPosition < u16Length)
    {
        if (eZigbee_NodeAddCommand(&sZigbee_Network.sNodes, psCommandIDList->u8Endpoint, 
            psCommandIDList->u16ClusterID, psCommandIDList->au8CommandList[iCommand]) != E_ZB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint8);
        iCommand++;
    }    

    //DBG_PrintNode(&tsZigbee_Network.sNodes);
done:
    mLockUnlock(&sZigbee_Network.sNodes.mutex);
}

static void ZCB_HandleRestartProvisioned(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x8006 message, ZCB_HandleRestartProvisioned\n");
    const char *pcStatus = NULL;
    
    struct _tsWarmRestart
    {
        uint8     u8Status;
    } __attribute__((__packed__)) *psWarmRestart = (struct _tsWarmRestart *)pvMessage;

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
    DBG_vPrintf(T_TRUE,  "Control bridge restarted, status %d (%s)\n", psWarmRestart->u8Status, pcStatus);
    return;
}

static void ZCB_HandleRestartFactoryNew(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x8007 message, ZCB_HandleRestartFactoryNew\n");
    const char *pcStatus = NULL;
    
    struct _tsWarmRestart
    {
        uint8     u8Status;
    } __attribute__((__packed__)) *psWarmRestart = (struct _tsWarmRestart *)pvMessage;

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
    ERR_vPrintf(T_TRUE,  "Control bridge factory new restart, status %d (%s)", psWarmRestart->u8Status, pcStatus);
    
    eZCB_ConfigureControlBridge();
    return;
}



static void ZCB_HandleNetworkJoined(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x8024 message, ZCB_HandleNetworkJoined\n");
    struct _tsNetworkJoinedFormed
    {
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint64    u64IEEEAddress;
        uint8     u8Channel;
    } __attribute__((__packed__)) *psMessage = (struct _tsNetworkJoinedFormed *)pvMessage;

    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    
    DBG_vPrintf(verbosity, "Network %s on channel %d. Control bridge address 0x%04X (0x%016llX)\n", 
                psMessage->u8Status == 0 ? "joined" : "formed",
                psMessage->u8Channel,
                psMessage->u16ShortAddress,
                (unsigned long long int)psMessage->u64IEEEAddress);

    /* Control bridge joined the network - initialise its data in the network structure */
    mLockLock(&sZigbee_Network.sNodes.mutex);
    
    sZigbee_Network.sNodes.u16DeviceID     = E_ZB_DEVICEID_CONTROLBRIDGE;
    sZigbee_Network.sNodes.u16ShortAddress = psMessage->u16ShortAddress;
    sZigbee_Network.sNodes.u64IEEEAddress  = psMessage->u64IEEEAddress;
    
    DBG_vPrintf(DBG_ZCB, "Node Joined 0x%04X (0x%016llX)\n", 
                sZigbee_Network.sNodes.u16ShortAddress, 
                (unsigned long long int)sZigbee_Network.sNodes.u64IEEEAddress);    

#ifdef ZIGBEE_SQLITE
    if(eZigbeeSqliteAddNewDevice(psMessage->u64IEEEAddress, psMessage->u16ShortAddress, E_ZB_DEVICEID_CONTROLBRIDGE, "NULL", "NULL"))
    {
        ERR_vPrintf(T_TRUE, "eZigbeeSqliteAddNewDevice:0x%04x Error\n", E_ZB_DEVICEID_CONTROLBRIDGE);     
    }
#endif
    asDeviceIDMap[0].prInitaliseRoutine(&sZigbee_Network.sNodes);
    iFlagAllowHandleReport = 1;
    if(eZCB_MatchDescriptorRequest(E_ZB_BROADCAST_ADDRESS_RXONWHENIDLE, au16ProfileHA,
                                sizeof(au16Cluster) / sizeof(uint16), au16Cluster, 
                                0, NULL, NULL) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "Error sending match descriptor request\n");
    }
    mLockUnlock(&sZigbee_Network.sNodes.mutex);
    
    return;
}



static void ZCB_HandleDeviceAnnounce(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x004D message, ZCB_HandleDeviceAnnounce\n");
    tsZigbee_Node *psZigbeeNode;
    struct _tsDeviceAnnounce
    {
        uint16    u16ShortAddress;
        uint64    u64IEEEAddress;
        uint8     u8MacCapability;
    } __attribute__((__packed__)) *psMessage = (struct _tsDeviceAnnounce *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    
    DBG_vPrintf(verbosity, "Device Joined, Address 0x%04X (0x%016llX). Mac Capability Mask 0x%02X\n", 
                psMessage->u16ShortAddress,(unsigned long long int)psMessage->u64IEEEAddress,psMessage->u8MacCapability);
    teZbStatus eZbStatus;
    if ((eZbStatus = eZigbee_AddNode(psMessage->u16ShortAddress, psMessage->u64IEEEAddress, 0, psMessage->u8MacCapability, &psZigbeeNode)) == E_ZB_OK)
    {
        if(eZCB_MatchDescriptorRequest(psMessage->u16ShortAddress, au16ProfileHA,
                                    sizeof(au16Cluster) / sizeof(uint16), au16Cluster, 0, NULL, NULL) != E_ZB_OK)
        {
            ERR_vPrintf(DBG_PCT, "Error sending match descriptor request\n");
        }
    }
    else
    {
        DBG_vPrintf(verbosity,"eZigbee_AddNode return:%d\n", eZbStatus);
    }
    
    return;
}

static void eZCB_HandleDeviceJoin(tsZigbee_Node *ZigbeeMatchNode)
{
    DBG_vPrintf(verbosity, "eZCB_HandleDeviceJoin\n");
    if(0 != ZigbeeMatchNode->u16DeviceID)
    {
        DBG_vPrintf(verbosity, "\tDevice is already in the network\n");
        return;
    }
    if(!ZigbeeMatchNode->u64IEEEAddress)
    {
        DBG_vPrintf(verbosity, "Requesting new endpoint MAC Address\n");
        if (eZCB_IEEEAddressRequest(ZigbeeMatchNode) != E_ZB_OK)
        {
            ERR_vPrintf(T_TRUE, "Error retrieving IEEE Address of node 0x%04X - requeue\n", ZigbeeMatchNode->u16ShortAddress);
            if (iZigbee_DeviceTimedOut(ZigbeeMatchNode))
            {
                DBG_vPrintf(verbosity, "Zigbee node 0x%04X removed from network (no response to IEEE Address).\n", ZigbeeMatchNode->u16ShortAddress);
                mLockLock(&sZigbee_Network.mutex);
                if (eZigbee_RemoveNode(ZigbeeMatchNode) != E_ZB_OK)
                {
                    ERR_vPrintf(T_TRUE, "Error removing node from ZCB\n");
                    mLockUnlock(&sZigbee_Network.mutex);
                    return;
                }
                mLockUnlock(&sZigbee_Network.mutex);
            }
            else
            {
                return;
            }
        }
    
    }
    int i = 0;
    for (i = 0; i < ZigbeeMatchNode->u32NumEndpoints; i++)
    {
        if (ZigbeeMatchNode->pasEndpoints[i].u16ProfileID == 0)
        {
            DBG_vPrintf(verbosity, "Requesting new endpoint[%d] simple descriptor\n",i);
            if (eZCB_SimpleDescriptorRequest(ZigbeeMatchNode, ZigbeeMatchNode->pasEndpoints[i].u8Endpoint) != E_ZB_OK)
            {
                ERR_vPrintf(T_TRUE, "Failed to read endpoint simple descriptor - requeue\n");
                if (iZigbee_DeviceTimedOut(ZigbeeMatchNode))
                {
                    DBG_vPrintf(verbosity, "Zigbee node 0x%04X removed from network (no response to IEEE Address).\n", ZigbeeMatchNode->u16ShortAddress);
                    mLockLock(&sZigbee_Network.mutex);
                    if (eZigbee_RemoveNode(ZigbeeMatchNode) != E_ZB_OK)
                    {
                        ERR_vPrintf(T_TRUE, "Error removing node from ZCB\n");
                        mLockUnlock(&sZigbee_Network.mutex);
                        return;
                    }
                    mLockUnlock(&sZigbee_Network.mutex);
                }
                else
                {
                    return;
                }
            }
        }
    }
    teZbStatus eStatus = eZCB_AddGroupMembership(ZigbeeMatchNode, 0xf00f);
    if ( (eStatus != E_ZB_OK) && (eStatus != E_ZB_DUPLICATE_EXISTS) && (eStatus != E_ZB_UNKNOWN_CLUSTER))
    {
        ERR_vPrintf(verbosity, "Failed to add device into group 0xf00f - requeue\n");
        if (iZigbee_DeviceTimedOut(ZigbeeMatchNode))
        {
            DBG_vPrintf(verbosity, "Zigbee node 0x%04X removed from network (no response to IEEE Address).\n", ZigbeeMatchNode->u16ShortAddress);
            mLockLock(&sZigbee_Network.mutex);
            if (eZigbee_RemoveNode(ZigbeeMatchNode) != E_ZB_OK)
            {
                ERR_vPrintf(T_TRUE, "Error removing node from ZCB\n");
                mLockUnlock(&sZigbee_Network.mutex);
                return;
            }
        }
        else
        {
            return;
        }
    }
    
    DBG_vPrintf(verbosity, "Init Device 0x%04x\n",ZigbeeMatchNode->u16DeviceID);
    tsDeviceIDMap *psDeviceIDMap = asDeviceIDMap;
    while (((psDeviceIDMap->u16ZigbeeDeviceID != 0) &&
    (psDeviceIDMap->prInitaliseRoutine != NULL)))
    {
        if (psDeviceIDMap->u16ZigbeeDeviceID == ZigbeeMatchNode->u16DeviceID)
        {
            DBG_vPrintf(verbosity, "Found JIP device type 0x%08X for Zigbee Device type 0x%04X\n",
            psDeviceIDMap->u32JIPDeviceID, psDeviceIDMap->u16ZigbeeDeviceID);
            DBG_vPrintf(verbosity, "Calling Zigbee Initlise Function\n");
            psDeviceIDMap->prInitaliseRoutine(ZigbeeMatchNode);
        }
    
        /* Next device map */
        psDeviceIDMap++;
    }
    return;
}

static void ZCB_HandleMatchDescriptorResponse(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x8046 message, ZCB_HandleMatchDescriptorResponse\n");
    tsZigbee_Node *psZigbeeNode;
    struct _tMatchDescriptorResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
        uint16    u16ShortAddress;
        uint8     u8NumEndpoints;
        uint8     au8Endpoints[255];
    } __attribute__((__packed__)) *psMatchDescriptorResponse = (struct _tMatchDescriptorResponse *)pvMessage;

    psMatchDescriptorResponse->u16ShortAddress  = ntohs(psMatchDescriptorResponse->u16ShortAddress);
//if (psMatchDescriptorResponse->u8NumEndpoints)
//{
    // Device has matching endpoints
    if (eZigbee_AddNode(psMatchDescriptorResponse->u16ShortAddress, 0, 0, 0, &psZigbeeNode) == E_ZB_OK)
    {
        mLockLock(&psZigbeeNode->mutex);
        int i;
        int iEventQuene = 0;
        for (i = 0; i < psMatchDescriptorResponse->u8NumEndpoints; i++)
        {
            /* Add an endpoint to the device for each response in the match descriptor response */
            if (eZigbee_NodeAddEndpoint(psZigbeeNode, psMatchDescriptorResponse->au8Endpoints[i], 0, NULL) != E_ZB_OK)            
            {
                ERR_vPrintf(DBG_PCT, "eZCB_NodeAddEndpoint error\n");
                iEventQuene = 1;
            }
        }
        if(!iEventQuene)
        {
            eZCB_HandleDeviceJoin(psZigbeeNode);
        }
        mLockUnlock(&psZigbeeNode->mutex);          
    }
//}
    return;
}
     
static void ZCB_HandleAttributeReport(void *pvUser, uint16 u16Length, void *pvMessage)
{
    DBG_vPrintf(verbosity,"Handle 0x8102 message, ZCB_HandleAttributeReport\n");

    struct _tsAttributeReport
    {
        uint8     u8SequenceNo;
        uint16    u16ShortAddress;
        uint8     u8Endpoint;
        uint16    u16ClusterID;
        uint16    u16AttributeID;
        uint8     u8AttributeStatus;
        uint8     u8Type;
        tuZcbAttributeData uData;
    } __attribute__((__packed__)) *psMessage = (struct _tsAttributeReport *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);
    psMessage->u16AttributeID   = ntohs(psMessage->u16AttributeID);
    
    INF_vPrintf(DBG_ZCB, "Attribute report from 0x%04X - Endpoint %d, cluster 0x%04X, attribute %d.\n", 
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

    DBG_vPrintf(verbosity, "E_ZCB_EVENT_ATTRIBUTE_REPORT\n");
    if(iFlagAllowHandleReport)
    {
        DBG_vPrintf(verbosity, "psZigbee_FindNodeByShortAddress 0x%04x\n", psMessage->u16ShortAddress);
        tsZigbee_Node *pZigbee_Node = psZigbee_FindNodeByShortAddress(psMessage->u16ShortAddress);
        if (!pZigbee_Node)
        {
            ERR_vPrintf(T_TRUE, "No JIP device for Zigbee node \n");
            //retry search the device
            if(eZCB_MatchDescriptorRequest(E_ZB_BROADCAST_ADDRESS_RXONWHENIDLE, au16ProfileHA,
                                        sizeof(au16Cluster) / sizeof(uint16), au16Cluster, 
                                        0, NULL, NULL) != E_ZB_OK)
            {
                ERR_vPrintf(T_TRUE, "Error sending match descriptor request\n");
            }
        }
        else
        {
            mLockLock(&pZigbee_Node->mutex);
            //DBG_vPrintf(verbosity, "Update Device's Attribute\n");
            if(NULL != pZigbee_Node->Method.DeviceAttributeUpdate)
            {
                pZigbee_Node->Method.DeviceAttributeUpdate(pZigbee_Node,
                    psMessage->u16ClusterID,
                    psMessage->u16AttributeID,
                    psMessage->u8Type,
                    psMessage->uData);
            }
            else
            {
                ERR_vPrintf(T_TRUE, "Update Device's Attribute Fun Can't Find\n");
            }
            mLockUnlock(&pZigbee_Node->mutex);
        }
    }
    return;
}

static teZbStatus eZCB_ConfigureControlBridge(void)
{
    #define CONFIGURATION_INTERVAL 500000
    /* Set up configuration */
    switch (eStartMode)
    {
        case(E_START_COORDINATOR):
            DBG_vPrintf(verbosity, "Starting control bridge as HA coordinator");
            eZCB_SetDeviceType(E_MODE_COORDINATOR);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetChannelMask(eChannel);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetExtendedPANID(u64PanID);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_StartNetwork();
            usleep(CONFIGURATION_INTERVAL);
            break;
    
        case (E_START_ROUTER):
            DBG_vPrintf(verbosity, "Starting control bridge as HA compatible router");
            eZCB_SetDeviceType(E_MODE_HA_COMPATABILITY);
            usleep(CONFIGURATION_INTERVAL);

            eZCB_SetChannelMask(eChannel);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetExtendedPANID(u64PanID);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_StartNetwork();
            usleep(CONFIGURATION_INTERVAL);
            break;
    
        case (E_START_TOUCHLINK):
            DBG_vPrintf(verbosity, "Starting control bridge as ZLL router");
            eZCB_SetDeviceType(E_MODE_ROUTER);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetChannelMask(eChannel);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetExtendedPANID(u64PanID);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_StartNetwork();
            usleep(CONFIGURATION_INTERVAL);
            break;
            
        default:
            ERR_vPrintf(T_TRUE,  "Unknown module mode\n");
            return E_ZB_ERROR;
    }
    
    return E_ZB_OK;
}

#define Swap64(ll) (((ll) >> 56) |                          \
                    (((ll) & 0x00ff000000000000) >> 40) |   \
                    (((ll) & 0x0000ff0000000000) >> 24) |   \
                    (((ll) & 0x000000ff00000000) >> 8)  |   \
                    (((ll) & 0x00000000ff000000) << 8)  |   \
                    (((ll) & 0x0000000000ff0000) << 24) |   \
                    (((ll) & 0x000000000000ff00) << 40) |   \
                    (((ll) << 56)))  

teZbStatus eZCB_BindRequest(tsZigbee_Node *psZigbeeNode, uint64 u64DestinationAddress, uint16 u16BindCluster)
{
    struct _BindRequest
    {
        uint64    u64TargetAddress;//Light Sensor
        uint8     u8SourceEndpoint;
        uint16    u16BindCluster;
        
        uint8     u8DestinationAddressMode;//Lamp 
        uint64    u64DestinationAddress;
        uint8     u8DestinationEndpoint;
        
    } __attribute__((__packed__)) sBindRequest;
    
    struct _sBindResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
    } __attribute__((__packed__)) *psBindResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    
    if (psZigbeeNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send bind cluster (Cluster 0x%04X)(u64TargetAddress %llu)(u64DestinationAddress %llu)\n", 
                    u16BindCluster, psZigbeeNode->u64IEEEAddress, u64DestinationAddress & (~0x1000000000000000));
        sBindRequest.u8DestinationAddressMode   = E_ZB_ADDRESS_MODE_IEEE;

        if(u64DestinationAddress & 0x1000000000000000)//Color Light
        {
            sBindRequest.u64TargetAddress     = be64toh(psZigbeeNode->u64IEEEAddress);
            sBindRequest.u64DestinationAddress     = be64toh(u64DestinationAddress  & (~0x1000000000000000));
            sBindRequest.u8SourceEndpoint = 1;
            sBindRequest.u8DestinationEndpoint = 2;
        }
        else
        {
            sBindRequest.u64TargetAddress     = be64toh(psZigbeeNode->u64IEEEAddress);
            sBindRequest.u64DestinationAddress     = be64toh(u64DestinationAddress);
            sBindRequest.u8SourceEndpoint = 1;
            sBindRequest.u8DestinationEndpoint = 1;
        }
    }
    DBG_vPrintf(DBG_ZCB,"u8SourceEndpoint %d u8DestinationEndpoint %d\n",sBindRequest.u8SourceEndpoint,sBindRequest.u8DestinationEndpoint);
    sBindRequest.u16BindCluster  = htons(u16BindCluster);

    if (eSL_SendMessage(E_SL_MSG_BIND, sizeof(struct _BindRequest), &sBindRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_BIND_RESPONSE, 1000, &u16Length, (void**)&psBindResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to add bind request");
            goto done;
        }
        break;
    }
        
    eStatus = psBindResponse->u8Status;
done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psBindResponse);
    return eStatus;
}

teZbStatus eZCB_UnBindRequest(tsZigbee_Node *psZigbeeNode, uint64 u64DestinationAddress, uint16 u16BindCluster)
{
    struct _BindRequest
    {
        uint64    u64TargetAddress;//Light Sensor
        uint8     u8SourceEndpoint;
        uint16    u16BindCluster;
        
        uint8     u8DestinationAddressMode;//Lamp 
        uint64    u64DestinationAddress;
        uint8     u8DestinationEndpoint;
        
    } __attribute__((__packed__)) sBindRequest;
    
    struct _sBindResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
    } __attribute__((__packed__)) *psBindResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    
    if (psZigbeeNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send bind cluster (Cluster 0x%04X)(u64TargetAddress %llu)(u64DestinationAddress %llu)\n", 
                    u16BindCluster, psZigbeeNode->u64IEEEAddress, u64DestinationAddress & (~0x1000000000000000));
        sBindRequest.u8DestinationAddressMode   = E_ZB_ADDRESS_MODE_IEEE;

        if(u64DestinationAddress & 0x1000000000000000)//Color Light
        {
            sBindRequest.u64TargetAddress     = be64toh(psZigbeeNode->u64IEEEAddress);
            sBindRequest.u64DestinationAddress     = be64toh(u64DestinationAddress  & (~0x1000000000000000));
            sBindRequest.u8SourceEndpoint = 1;
            sBindRequest.u8DestinationEndpoint = 2;
        }
        else
        {
            sBindRequest.u64TargetAddress     = be64toh(psZigbeeNode->u64IEEEAddress);
            sBindRequest.u64DestinationAddress     = be64toh(u64DestinationAddress);
            sBindRequest.u8SourceEndpoint = 1;
            sBindRequest.u8DestinationEndpoint = 1;
        }
    }
    DBG_vPrintf(DBG_ZCB,"u8SourceEndpoint %d u8DestinationEndpoint %d\n",
        sBindRequest.u8SourceEndpoint,sBindRequest.u8DestinationEndpoint);
    sBindRequest.u16BindCluster  = htons(u16BindCluster);

    if (eSL_SendMessage(E_SL_MSG_UNBIND, sizeof(struct _BindRequest), &sBindRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_UNBIND_RESPONSE, 1000, &u16Length, (void**)&psBindResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to add bind request");
            goto done;
        }
        break;
    }
        
    eStatus = psBindResponse->u8Status;
done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psBindResponse);
    return eStatus;
}

teZbStatus eZCB_ManagementLeaveRequest(tsZigbee_Node *psZigbeeNode)
{
    struct _BindRequest
    {
        uint16    u16TargetShortAddress;
        uint64    u64TargetAddress;
        uint8     u8Flags;
    } __attribute__((__packed__)) sManagementLeaveRequest;
    
    struct _sBindResponse
    {
        uint8     u8SequenceNo;
        uint8     u8Status;
    } __attribute__((__packed__)) *psManagementLeaveResponse = NULL;
    
    uint16 u16Length;
    uint8 u8SequenceNo;
    teZbStatus eStatus = E_ZB_COMMS_FAILED;

    
    if (psZigbeeNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send management Leave (u64TargetAddress %llu)\n", 
                    psZigbeeNode->u64IEEEAddress);
        sManagementLeaveRequest.u16TargetShortAddress = 0x0000;
        sManagementLeaveRequest.u8Flags = 0x02;
        sManagementLeaveRequest.u64TargetAddress = Swap64(psZigbeeNode->u64IEEEAddress);
    }

    if (eSL_SendMessage(E_SL_MSG_MANAGEMENT_LEAVE_REQUEST, sizeof(sManagementLeaveRequest), &sManagementLeaveRequest, &u8SequenceNo) != E_SL_OK)
    {
        ERR_vPrintf(T_TRUE,  "eSL_SendMessage Error\n");
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_LEAVE_CONFIRMATION, 1000, &u16Length, (void**)&psManagementLeaveResponse) != E_SL_OK)
        {
            ERR_vPrintf(T_TRUE,  "No response to Management Leave request\n");
            goto done;
        }
        break;
    }
        
    eStatus = psManagementLeaveResponse->u8Status;
done:
    vZigbee_NodeUpdateComms(psZigbeeNode, eStatus);
    free(psManagementLeaveResponse);
    return eStatus;
}


teZbStatus eZBZLL_OnOff(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode)
{
    tsZigbee_Node   *psControlBridge;
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8         u8SequenceNo;
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Mode;
    } __attribute__((__packed__)) sOnOffMessage;
    
    DBG_vPrintf(DBG_ZCB, "On/Off (Set Mode=%d)\n", u8Mode);
    
    if (u8Mode > 2)
    {
        /* Illegal value */
        return E_ZB_ERROR;
    }
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    mLockLock(&psControlBridge->mutex);
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_ONOFF);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_ONOFF);
        return E_ZB_ERROR;
    }

    sOnOffMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);
    
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
        sOnOffMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_ONOFF);

        if (psDestinationEndpoint)
        {
            sOnOffMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
    }
    else
    {
        sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sOnOffMessage.u16TargetAddress      = htons(u16GroupAddress);
        sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sOnOffMessage.u8Mode = u8Mode;
    
    if (eSL_SendMessage(E_SL_MSG_ONOFF, sizeof(sOnOffMessage), &sOnOffMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    if (psZigbeeNode)
    {
        psZigbeeNode->u8LightMode = u8Mode;
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZBZLL_OnOffCheck(tsZigbee_Node *psZigbeeNode, uint8 u8Mode)
{
    teZbStatus eStatus;
    uint8 u8OnOffStatus;
    
    if ((eStatus = eZBZLL_OnOff(psZigbeeNode, 0, u8Mode)) != E_ZB_OK)
    {
        return eStatus;
    }
    
    if (u8Mode < 2)
    {
        /* Can't check the staus of a toggle command */
        
        /* Wait 100ms */
        usleep(100*1000);
        
        if ((eStatus = eZCB_ReadAttributeRequest(psZigbeeNode, 
                E_ZB_CLUSTERID_ONOFF, 0, 0, 0, E_ZB_ATTRIBUTEID_ONOFF_ONOFF, &u8OnOffStatus)) != E_ZB_OK)
        {
            return eStatus;
        }
        
        DBG_vPrintf(DBG_ZCB, "On Off attribute read as: 0x%02X\n", u8OnOffStatus);
        
        if (u8OnOffStatus != u8Mode)
        {
            return E_ZB_REQUEST_NOT_ACTIONED;
        }
    }
    return E_ZB_OK;
}

teZbStatus eZBZLL_MoveToLevel(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8OnOff, uint8 u8Level, uint16 u16TransitionTime)
{
    tsZigbee_Node          *psControlBridge;
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
    } __attribute__((__packed__)) sLevelMessage;
    
    DBG_vPrintf(DBG_ZCB, "Set Level %d\n", u8Level);
    
    if (u8Level > 254)
    {
        u8Level = 254;
    }
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    mLockLock(&psControlBridge->mutex);
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_LEVEL_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_LEVEL_CONTROL);
        return E_ZB_ERROR;
    }

    sLevelMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);

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
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_LEVEL_CONTROL);

        if (psDestinationEndpoint)
        {
            sLevelMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sLevelMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
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
   // sOnOffMessage.u8Gradient            = u8Gradient;
    
    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_LEVEL_ONOFF, sizeof(sLevelMessage), &sLevelMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    if (psZigbeeNode)
    {
        psZigbeeNode->u8LightLevel = u8Level;
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}     

teZbStatus eZBZLL_MoveToLevelCheck(tsZigbee_Node *psZigbeeNode, uint8 u8OnOff, uint8 u8Level, uint16 u16TransitionTime)
{
    teZbStatus eStatus;
    uint8 u8CurrentLevel;
    
    if ((eStatus = eZCB_ReadAttributeRequest(psZigbeeNode, 
            E_ZB_CLUSTERID_LEVEL_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL, &u8CurrentLevel)) != E_ZB_OK)
    {
        return eStatus;
    }
    
    DBG_vPrintf(DBG_ZCB, "Current Level attribute read as: 0x%02X\n", u8CurrentLevel);
    
    if (u8CurrentLevel == u8Level)
    {
        /* Level is already set */
        /* This is a guard for transition times that are outside of the JIP 300ms retry window */
        return E_ZB_OK;
    }
    
    if (u8Level > 254)
    {
        u8Level = 254;
    }
    
    if ((eStatus = eZBZLL_MoveToLevel(psZigbeeNode, 0, u8OnOff, u8Level, u16TransitionTime)) != E_ZB_OK)
    {
        return eStatus;
    }
    
    /* Wait the transition time */
    usleep(u16TransitionTime * 100000);

    if ((eStatus = eZCB_ReadAttributeRequest(psZigbeeNode, 
            E_ZB_CLUSTERID_LEVEL_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL, &u8CurrentLevel)) != E_ZB_OK)
    {
        return eStatus;
    }
    
    DBG_vPrintf(DBG_ZCB, "Current Level attribute read as: 0x%02X\n", u8CurrentLevel);
    
    if (u8CurrentLevel != u8Level)
    {
        return E_ZB_REQUEST_NOT_ACTIONED;
    }
    return E_ZB_OK;
}

teZbStatus eZBZLL_MoveToHue(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Hue, uint16 u16TransitionTime)
{
    tsZigbee_Node          *psControlBridge;
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
        uint8     u8Direction;
        uint16    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToHueMessage;
    
    DBG_vPrintf(DBG_ZCB, "Set Hue %d\n", u8Hue);
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    
    mLockLock(&psControlBridge->mutex);
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }

    sMoveToHueMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToHueMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToHueMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToHueMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToHueMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToHueMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
    }
    else
    {
        sMoveToHueMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToHueMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToHueMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToHueMessage.u8Hue               = u8Hue;
    sMoveToHueMessage.u8Direction         = 0;
    sMoveToHueMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE, sizeof(sMoveToHueMessage), &sMoveToHueMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    if (psZigbeeNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZBZLL_MoveToSaturation(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Saturation, uint16 u16TransitionTime)
{
    tsZigbee_Node          *psControlBridge;
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Saturation;
        uint16    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToSaturationMessage;
    
    DBG_vPrintf(DBG_ZCB, "Set Saturation %d\n", u8Saturation);
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    mLockLock(&psControlBridge->mutex);
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }

    sMoveToSaturationMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToSaturationMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToSaturationMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
    }
    else
    {
        sMoveToSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToSaturationMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToSaturationMessage.u8Saturation        = u8Saturation;
    sMoveToSaturationMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_SATURATION, 
            sizeof(sMoveToSaturationMessage), &sMoveToSaturationMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    if (psZigbeeNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZBZLL_MoveToHueSaturation(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Hue, uint8 u8Saturation, uint16 u16TransitionTime)
{
    tsZigbee_Node          *psControlBridge;
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
    } __attribute__((__packed__)) sMoveToHueSaturationMessage;
    
    DBG_vPrintf(DBG_ZCB, "Set Hue %d, Saturation %d\n", u8Hue, u8Saturation);
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    mLockLock(&psControlBridge->mutex);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }

    sMoveToHueSaturationMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);

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
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
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
    
    if (psZigbeeNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZBZLL_MoveToColour(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16X, uint16 u16Y, uint16 u16TransitionTime)
{
    tsZigbee_Node          *psControlBridge;
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16X;
        uint16    u16Y;
        uint16    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToColourMessage;
    
    DBG_vPrintf(DBG_ZCB, "Set X %d, Y %d\n", u16X, u16Y);
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    mLockLock(&psControlBridge->mutex);
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }

    sMoveToColourMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToColourMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToColourMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToColourMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToColourMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToColourMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
    }
    else
    {
        sMoveToColourMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToColourMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToColourMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToColourMessage.u16X                = htons(u16X);
    sMoveToColourMessage.u16Y                = htons(u16Y);
    sMoveToColourMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR, sizeof(sMoveToColourMessage), &sMoveToColourMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    if (psZigbeeNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}

teZbStatus eZBZLL_MoveToColourTemperature(tsZigbee_Node *psZigbeeNode, 
    uint16 u16GroupAddress, uint16 u16ColourTemperature, uint16 u16TransitionTime)
{
    tsZigbee_Node          *psControlBridge;
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint16    u16ColourTemperature;
        uint16    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToColourTemperatureMessage;
    
    DBG_vPrintf(DBG_ZCB, "Set colour temperature %d\n", u16ColourTemperature);
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    mLockLock(&psControlBridge->mutex);
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }

    sMoveToColourTemperatureMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToColourTemperatureMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToColourTemperatureMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
    }
    else
    {
        sMoveToColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToColourTemperatureMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToColourTemperatureMessage.u16ColourTemperature    = htons(u16ColourTemperature);
    sMoveToColourTemperatureMessage.u16TransitionTime       = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE, 
            sizeof(sMoveToColourTemperatureMessage), &sMoveToColourTemperatureMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    if (psZigbeeNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
}


teZbStatus eZBZLL_MoveColourTemperature(tsZigbee_Node *psZigbeeNode,
    uint16 u16GroupAddress, uint8 u8Mode, uint16 u16Rate, uint16 u16ColourTemperatureMin, uint16 u16ColourTemperatureMax)
{
    tsZigbee_Node          *psControlBridge;
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    uint8             u8SequenceNo;
    
    struct
    {
        uint8     u8TargetAddressMode;
        uint16    u16TargetAddress;
        uint8     u8SourceEndpoint;
        uint8     u8DestinationEndpoint;
        uint8     u8Mode;
        uint16    u16Rate;
        uint16    u16ColourTemperatureMin;
        uint16    u16ColourTemperatureMax;
    } __attribute__((__packed__)) sMoveColourTemperatureMessage;
    
    DBG_vPrintf(DBG_ZCB, "Move colour temperature\n");
    
    psControlBridge = psZigbee_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZB_ERROR;
    }
    psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    mLockLock(&psControlBridge->mutex);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZCB, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZB_ERROR;
    }

    sMoveColourTemperatureMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    mLockUnlock(&psControlBridge->mutex);

    if (psZigbeeNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveColourTemperatureMessage.u16TargetAddress      = htons(psZigbeeNode->u16ShortAddress);
        
        mLockLock(&psZigbeeNode->mutex);
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveColourTemperatureMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        mLockUnlock(&psZigbeeNode->mutex);
    }
    else
    {
        sMoveColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveColourTemperatureMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveColourTemperatureMessage.u8Mode                    = u8Mode;
    sMoveColourTemperatureMessage.u16Rate                   = htons(u16Rate);
    sMoveColourTemperatureMessage.u16ColourTemperatureMin   = htons(u16ColourTemperatureMin);
    sMoveColourTemperatureMessage.u16ColourTemperatureMax   = htons(u16ColourTemperatureMax);
    
    if (eSL_SendMessage(E_SL_MSG_MOVE_COLOUR_TEMPERATURE, 
            sizeof(sMoveColourTemperatureMessage), &sMoveColourTemperatureMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    
    if (psZigbeeNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZB_OK;
    }
    
}

#ifdef OTA_SERVER

teZbStatus sendOtaImageNotify(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint8 u8SrcEndPoint, 
                                uint8 u8DstEndPoint, uint8 u8NotifyType, uint32 u32FileVersion, 
                                uint16 u16ImageType, uint16 u16ManuCode, uint8 u8Jitter)
{ 
    uint8             u8SequenceNo;
    struct
    {
        uint8  u8DstAddrMode;
        uint16 u16ShortAddr;
        uint8  u8SrcEndPoint;
        uint8  u8DstEndPoint;
        uint8  u8NotifyType;
        uint32 u32FileVersion;
        uint16 u16ImageType;
        uint16 u16ManuCode;
        uint8  u8Jitter;
    } __attribute__((__packed__)) sOTA_ImageNotifyCommand;

    sOTA_ImageNotifyCommand.u8SrcEndPoint  = u8SrcEndPoint;
    sOTA_ImageNotifyCommand.u8DstEndPoint  = u8DstEndPoint;
    sOTA_ImageNotifyCommand.u8NotifyType   = u8NotifyType;
    sOTA_ImageNotifyCommand.u32FileVersion = u32FileVersion;
    sOTA_ImageNotifyCommand.u16ImageType   = u16ImageType;
    sOTA_ImageNotifyCommand.u16ManuCode    = u16ManuCode;
    sOTA_ImageNotifyCommand.u8Jitter       = u8Jitter;

    if (eSL_SendMessage(E_SL_MSG_IMAGE_NOTIFY, 
            sizeof(sOTA_ImageNotifyCommand), &sOTA_ImageNotifyCommand, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}


teZbStatus sendOtaEndResponse(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint8 u8SrcEndPoint, 
                                uint8 u8DstEndPoint, uint8 u8SeqNbr, uint32 u32UpgradeTime, 
                                uint32 u32CurrentTime, uint32 u32FileVersion, uint16 u16ImageType,
                                uint16 u16ManuCode)
{
    uint8 u8SequenceNo;
    struct
    {
        uint8  u8DstAddrMode;
        uint16 u16ShortAddr;
        uint8  u8SrcEndPoint;
        uint8  u8DstEndPoint;
        uint8  u8SeqNbr;
        uint32 u32UpgradeTime;
        uint32 u32CurrentTime;
        uint32 u32FileVersion;
        uint16 u16ImageType;
        uint16 u16ManuCode;
    } __attribute__((__packed__)) sUpgradeResponsePayload;

    sUpgradeResponsePayload.u8DstAddrMode   = u8DstAddrMode;
    sUpgradeResponsePayload.u16ShortAddr    = u16ShortAddr;
    sUpgradeResponsePayload.u8SrcEndPoint   = u8SrcEndPoint;
    sUpgradeResponsePayload.u8DstEndPoint   = u8DstEndPoint;
    sUpgradeResponsePayload.u8SeqNbr        = u8SeqNbr;
    sUpgradeResponsePayload.u32UpgradeTime  = u32UpgradeTime;
    sUpgradeResponsePayload.u32CurrentTime  = u32CurrentTime;
    sUpgradeResponsePayload.u32FileVersion  = u32FileVersion;
    sUpgradeResponsePayload.u16ImageType    = u16ImageType;
    sUpgradeResponsePayload.u16ManuCode     = u16ManuCode;
    
    if (eSL_SendMessage(E_SL_MSG_UPGRADE_END_RESPONSE, 
            sizeof(sUpgradeResponsePayload), &sUpgradeResponsePayload, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}

teZbStatus sendOtaBlock(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint8 u8SrcEndPoint, 
                        uint8 u8DstEndPoint, uint8 u8SeqNbr, uint8 u8Status, uint32 u32FileOffset, 
                        uint32 u32FileVersion, uint16 u16ImageType, uint16 u16ManuCode, 
                        uint8 u8DataSize, char *au8Data)
{
    uint8 u8SequenceNo;
    struct
    {
        uint8  u8DstAddrMode;
        uint16 u16ShortAddr;
        uint8  u8SrcEndPoint;
        uint8  u8DstEndPoint;
        uint8  u8SeqNbr;
        uint8  u8Status;       
        uint32 u32FileOffset;      
        uint32 u32FileVersion;
        uint16 u16ImageType;
        uint16 u16ManuCode;
        uint8  u8DataSize;
        char   *au8Data;
    } __attribute__((__packed__)) sImageBlockResponsePayload;

    sImageBlockResponsePayload.u8DstAddrMode    = u8DstAddrMode;
    sImageBlockResponsePayload.u16ShortAddr     = u16ShortAddr;
    sImageBlockResponsePayload.u8SrcEndPoint    = u8SrcEndPoint;
    sImageBlockResponsePayload.u8DstEndPoint    = u8DstEndPoint;
    sImageBlockResponsePayload.u8SeqNbr         = u8SeqNbr;
    sImageBlockResponsePayload.u8Status         = u8Status;       
    sImageBlockResponsePayload.u32FileOffset    = u32FileOffset;      
    sImageBlockResponsePayload.u32FileVersion   = u32FileVersion;
    sImageBlockResponsePayload.u16ImageType     = u16ImageType;
    sImageBlockResponsePayload.u16ManuCode      = u16ManuCode;
    sImageBlockResponsePayload.u8DataSize       = u8DataSize;

    sImageBlockResponsePayload.au8Data = (char*)malloc(u8DataSize);
    if(NULL == sImageBlockResponsePayload.au8Data)
    {
        return E_ZB_ERROR;
    }
    memcpy(sImageBlockResponsePayload.au8Data, au8Data, u8DataSize);

    if (eSL_SendMessage(E_SL_MSG_BLOCK_SEND, 
            sizeof(sImageBlockResponsePayload), &sImageBlockResponsePayload, &u8SequenceNo) != E_SL_OK)
    {
        free(sImageBlockResponsePayload.au8Data);
        return E_ZB_COMMS_FAILED;
    }
    free(sImageBlockResponsePayload.au8Data);
    return E_ZB_OK;
}

teZbStatus sendOtaLoadNewImage(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint32 u32FileIdentifier, 
                                uint16 u16HeaderVersion, uint16 u16HeaderLength, uint16 u16HeaderControlField, 
                                uint16 u16ManufacturerCode, uint16 u16ImageType, uint32 u32FileVersion, 
                                uint16 u16StackVersion, char *au8HeaderString, uint32 u32TotalImage, 
                                uint8 u8SecurityCredVersion, uint64 u64UpgradeFileDest, uint16 u16MinimumHwVersion, 
                                uint16 u16MaxHwVersion)
{
    uint8 u8SequenceNo;
    struct
    {
        uint8  u8DstAddrMode;
        uint16 u16ShortAddr;
        uint32 u32FileIdentifier;
        uint16 u16HeaderVersion;
        uint16 u16HeaderLength;
        uint16 u16HeaderControlField;
        uint16 u16ManufacturerCode;
        uint16 u16ImageType;
        uint32 u32FileVersion;
        uint16 u16StackVersion;
        char   au8HeaderString[32];
        uint32 u32TotalImage;
        uint8  u8SecurityCredVersion;
        uint64 u64UpgradeFileDest;
        uint16 u16MinimumHwVersion;
        uint16 u16MaxHwVersion;
    } __attribute__((__packed__)) sCoProcessorOTAHeader;

    sCoProcessorOTAHeader.u8DstAddrMode         = u8DstAddrMode;
    sCoProcessorOTAHeader.u16ShortAddr          = u16ShortAddr;
    sCoProcessorOTAHeader.u32FileIdentifier     = u32FileIdentifier;
    sCoProcessorOTAHeader.u16HeaderVersion      = u16HeaderVersion;
    sCoProcessorOTAHeader.u16HeaderLength       = u16HeaderLength;
    sCoProcessorOTAHeader.u16HeaderControlField = u16HeaderControlField;
    sCoProcessorOTAHeader.u16ManufacturerCode   = u16ManufacturerCode;
    sCoProcessorOTAHeader.u16ImageType          = u16ImageType;
    sCoProcessorOTAHeader.u32FileVersion        = u32FileVersion;
    sCoProcessorOTAHeader.u16StackVersion       = u16StackVersion;
    memcpy(sCoProcessorOTAHeader.au8HeaderString, au8HeaderString, 32);
    sCoProcessorOTAHeader.u32TotalImage         = u32TotalImage;
    sCoProcessorOTAHeader.u8SecurityCredVersion = u8SecurityCredVersion;
    sCoProcessorOTAHeader.u64UpgradeFileDest    = u64UpgradeFileDest;
    sCoProcessorOTAHeader.u16MinimumHwVersion   = u16MinimumHwVersion;
    sCoProcessorOTAHeader.u16MaxHwVersion       = u16MaxHwVersion;

    if (eSL_SendMessage(E_SL_MSG_LOAD_NEW_IMAGE, 
            sizeof(sCoProcessorOTAHeader), &sCoProcessorOTAHeader, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZB_COMMS_FAILED;
    }
    return E_ZB_OK;
}

#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
