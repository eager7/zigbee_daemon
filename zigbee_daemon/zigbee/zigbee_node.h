/****************************************************************************
 *
 * MODULE:             zigbee - zigbee daemon
 *
 * COMPONENT:          zigbee nodes interface
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



#ifndef __ZIGBEE_NODE_H__
#define __ZIGBEE_NODE_H__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdint.h>
#include <signal.h>
#include "utils.h"
#include "serial_link.h"
#include "zigbee_zcl.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define ZIGBEE_NODE_FAILED_TIMES 3      //if a node's communication failed 3 times, it already left.
#define ZIGBEE_NODE_FAILED_INTERVAL 15  //if a node's communication failed 15 seconds, it already left.
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** Enumerated type of statuses - This fits in with the Zigbee ZCL status codes */

/** Union type for all Zigbee attribute data types */
typedef union
{
    uint8  u8Data;
    uint16 u16Data;
    uint32 u32Data;
    uint64 u64Data;
} tuZcbAttributeData;

/** Structure representing a cluster on a node */
typedef struct
{
    uint16 u16ClusterID;        /* Cluster id */
    uint32 u32NumAttributes;    /* the number of cluster's attributes */
    uint16 *pau16Attributes;    /* attributes' list */
    uint32 u32NumCommands;      /* the number of cluster support commands */
    uint8  *pau8Commands;       /* commands' list */
} tsNodeCluster;

typedef struct
{
    uint8          u8Endpoint;
    uint16         u16ProfileID;    /* HA or ZLL or SmartEnery */
    uint32         u32NumClusters;  /* Node include clusters */
    tsNodeCluster *pasClusters;     /* Cluster Entry */
} tsNodeEndpoint;

typedef struct
{
    struct timeval  sLastSuccessful;        /**< Time of last successful communications */
    uint16          u16SequentialFailures;  /**< Number of sequential failures */
} tsComms;                                  /**< Structure containing communications statistics */

typedef struct
{
    volatile sig_atomic_t  bOnOff;
    volatile sig_atomic_t  u8Level;
    volatile sig_atomic_t  u8State;
    volatile sig_atomic_t  u16Illum;
    volatile sig_atomic_t  u16Temp;
    volatile sig_atomic_t  u16Humi;
    volatile sig_atomic_t  u8Binary;
    volatile sig_atomic_t  u16Battery;
}tsAttributeValue;

typedef struct   /* Zigbee Node base attribute */
{ 
    tsComms             sComms;
    char                auDeviceName[MIBF];
    uint16              u16DeviceID;
    uint8               u8DeviceOnline;
    uint16              u16ShortAddress;
    uint64              u64IEEEAddress;
    uint8               u8MacCapability;
    tsAttributeValue    sAttributeValue;
    
    uint32              u32NumEndpoints;
    tsNodeEndpoint      *pasEndpoints;
    
    uint32              u32NumGroups;
    uint16              *pau16Groups;
    struct dl_list      list;
} tsZigbeeBase;

typedef struct {float H,S,V;}tsHSV;
typedef struct {uint8 R,G,B;}tsRGB;
/* Coordinator */
typedef teZbStatus (*tpreCoordinatorReset)(tsZigbeeBase *psZigbeeNode);
typedef teZbStatus (*tpreCoordinatorPermitJoin)(uint8 time);
typedef teZbStatus (*tpreCoordinatorGetChannel)(uint8 *pu8Channel);
typedef teZbStatus (*tpreDeviceRemoveNetwork)(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren);
typedef teZbStatus (*tpreDeviceAddBind)(tsZigbeeBase *psSrcZigbeeNode, tsZigbeeBase *psDesZigbeeNode, uint16 u16ClusterID);
typedef teZbStatus (*tpreDeviceRemoveBind)(tsZigbeeBase *psSrcZigbeeNode, tsZigbeeBase *psDesZigbeeNode, uint16 u16ClusterID);
typedef teZbStatus (*tpreZCB_ResetNetwork)(tsZigbeeBase *psZigbeeNode);

/* Base */
typedef teZbStatus (*tpreDeviceSetAttribute)(tsZigbeeBase *psZigbeeNode, teZigbee_ClusterID u16ClusterID);
typedef teZbStatus (*tpreDeviceGetAttribute)(tsZigbeeBase *psZigbeeNode, teZigbee_ClusterID u16ClusterID);
typedef teZbStatus (*tpreDeviceAddGroup)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupID);
typedef teZbStatus (*tpreDeviceRemoveGroup)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupID);
typedef teZbStatus (*tpreDeviceClearGroup)(tsZigbeeBase *psZigbeeNode);
typedef teZbStatus (*tpreDeviceAddSence)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceID);
typedef teZbStatus (*tpreDeviceRemoveSence)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SencepID);
typedef teZbStatus (*tpreDeviceSetSence)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SencepID);
typedef teZbStatus (*tpreDeviceGetSence)(tsZigbeeBase *psZigbeeNode, uint16 *u16SencepID);
/* ZLL */
typedef teZbStatus (*tpreDeviceSetOnOff)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
typedef teZbStatus (*tpreDeviceGetOnOff)(tsZigbeeBase *psZigbeeNode, uint8 *u8Mode);
typedef teZbStatus (*tpreDeviceSetLightColour)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, tsRGB sRGB, uint16 u16TransitionTime);
typedef teZbStatus (*tpreDeviceGetLightColour)(tsZigbeeBase *psZigbeeNode, tsRGB *psRGB);
typedef teZbStatus (*tpreDeviceSetLevel)(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Level, uint16 u16TransitionTime);
typedef teZbStatus (*tpreDeviceGetLevel)(tsZigbeeBase *psZigbeeNode, uint8 *u8Level);

/* Sensor */
typedef teZbStatus (*tpreDeviceGetSensorValue)(tsZigbeeBase *psZigbeeNode, uint16 *u16SensorValue, teZigbee_ClusterID eClusterId);
typedef teZbStatus (*tprDeviceAttributeUpdate)(tsZigbeeBase *psZigbeeNode, uint16 u16ClusterID, uint16 u16AttributeID, teZCL_ZCLAttributeType eType, tuZcbAttributeData uData);
/* Closures */
typedef teZbStatus (*tpreDeviceSetWindowCovering)(tsZigbeeBase *psZigbeeNode, teCLD_WindowCovering_CommandID eCommand);
typedef teZbStatus (*tpreDeviceSetDoorLock)(tsZigbeeBase *psZigbeeNode, teCLD_DoorLock_CommandID eCommand);
typedef teZbStatus (*tpreDeviceSetDoorLockPassword)(tsZigbeeBase *psZigbeeNode, tsCLD_DoorLock_Payload sDoorLockPayload);

typedef struct
{
    /* Coordinator */
    tpreCoordinatorReset            preCoordinatorReset;
    tpreCoordinatorPermitJoin       preCoordinatorPermitJoin;
    tpreCoordinatorGetChannel       preCoordinatorGetChannel;
    tpreDeviceRemoveNetwork         preDeviceRemoveNetwork;
    tpreDeviceAddBind               preDeviceAddBind;
    tpreDeviceRemoveBind            preDeviceRemoveBind;
    tpreZCB_ResetNetwork            preZCB_ResetNetwork;

    /* Base */
    tpreDeviceSetAttribute          preDeviceSetAttribute;
    tpreDeviceGetAttribute          preDeviceGetAttribute;
    tpreDeviceAddGroup              preDeviceAddGroup;
    tpreDeviceRemoveGroup           preDeviceRemoveGroup;
    tpreDeviceClearGroup            preDeviceClearGroup;
    tpreDeviceAddSence              preDeviceAddScene;
    tpreDeviceSetSence              preDeviceSetScene;
    tpreDeviceGetSence              preDeviceGetScene;
    tpreDeviceRemoveSence           preDeviceRemoveScene;
    /* ZLL */
    tpreDeviceSetOnOff              preDeviceSetOnOff;
    tpreDeviceGetOnOff              preDeviceGetOnOff;
    tpreDeviceSetLightColour        preDeviceSetLightColour;
    tpreDeviceGetLightColour        preDeviceGetLightColour;
    tpreDeviceSetLevel              preDeviceSetLevel;
    tpreDeviceGetLevel              preDeviceGetLevel;
    /* Sensor */
    tpreDeviceGetSensorValue        preDeviceGetSensorValue;
    tprDeviceAttributeUpdate        preDeviceAttributeUpdate;
    /* Closures */
    tpreDeviceSetWindowCovering     preDeviceSetWindowCovering;
    tpreDeviceSetDoorLock           preDeviceSetDoorLock;
    tpreDeviceSetDoorLockPassword   preDeviceSetDoorLockPassword;
} tsZigbeeCallback;

typedef struct
{
    struct dl_list      list;
    pthread_mutex_t     mutex;         
    tsZigbeeBase        sNode;
    tsZigbeeCallback    Method;
} tsZigbeeNodes;

typedef teZbStatus (*tpreDeviceInitialise)(tsZigbeeNodes *psZigbeeNode);
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eZigbee_AddNode(uint16 u16ShortAddress, uint64 u64IEEEAddress, uint16 u16DeviceID, uint8 u8MacCapability, tsZigbeeNodes **ppsZCBNode);
teZbStatus eZigbee_RemoveNode(tsZigbeeNodes *psZigbeeNode);
teZbStatus eZigbee_RemoveAllNodes(void);
teZbStatus eZigbee_NodeAddEndpoint(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ProfileID, tsNodeEndpoint **ppsEndpoint);
teZbStatus eZigbee_NodeAddCluster(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID);
teZbStatus eZigbee_NodeAddAttribute(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint16 u16AttributeID);
teZbStatus eZigbee_NodeAddCommand(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint8 u8CommandID); 
teZbStatus eZigbee_GetEndpoints(tsZigbeeBase *psZigbee_Node, teZigbee_ClusterID eClusterID, uint8 *pu8Src, uint8 *pu8Dst);

tsNodeEndpoint *psZigbee_NodeFindEndpoint(tsZigbeeBase *psZigbeeNode, uint16 u16ClusterID);
tsZigbeeNodes *psZigbee_FindNodeByShortAddress(uint16 u16ShortAddress);
tsZigbeeNodes *psZigbee_FindNodeByIEEEAddress(uint64 u64IEEEAddress);


void vZigbee_PrintNode(tsZigbeeBase *psNode);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
#if defined __cplusplus
}
#endif

#endif /* __SERIAL_H__ */
