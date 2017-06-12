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
typedef enum
{
    /* Zigbee ZCL status codes */
    E_ZB_OK                            = 0x00,
    E_ZB_ERROR                         = 0x01,
    
    /* ZCB internal status codes */
    E_ZB_ERROR_NO_MEM                  = 0x10,
    E_ZB_COMMS_FAILED                  = 0x11,
    E_ZB_UNKNOWN_NODE                  = 0x12,
    E_ZB_UNKNOWN_ENDPOINT              = 0x13,
    E_ZB_UNKNOWN_CLUSTER               = 0x14,
    E_ZB_REQUEST_NOT_ACTIONED          = 0x15,
    
    /* Zigbee ZCL status codes */
    E_ZB_NOT_AUTHORISED                = 0x7E, 
    E_ZB_RESERVED_FIELD_NZERO          = 0x7F,
    E_ZB_MALFORMED_COMMAND             = 0x80,
    E_ZB_UNSUP_CLUSTER_COMMAND         = 0x81,
    E_ZB_UNSUP_GENERAL_COMMAND         = 0x82,
    E_ZB_UNSUP_MANUF_CLUSTER_COMMAND   = 0x83,
    E_ZB_UNSUP_MANUF_GENERAL_COMMAND   = 0x84,
    E_ZB_INVALID_FIELD                 = 0x85,
    E_ZB_UNSUP_ATTRIBUTE               = 0x86,
    E_ZB_INVALID_VALUE                 = 0x87,
    E_ZB_READ_ONLY                     = 0x88,
    E_ZB_INSUFFICIENT_SPACE            = 0x89,
    E_ZB_DUPLICATE_EXISTS              = 0x8A,
    E_ZB_NOT_FOUND                     = 0x8B,
    E_ZB_UNREPORTABLE_ATTRIBUTE        = 0x8C,
    E_ZB_INVALID_DATA_TYPE             = 0x8D,
    E_ZB_INVALID_SELECTOR              = 0x8E,
    E_ZB_WRITE_ONLY                    = 0x8F,
    E_ZB_INCONSISTENT_STARTUP_STATE    = 0x90,
    E_ZB_DEFINED_OUT_OF_BAND           = 0x91,
    E_ZB_INCONSISTENT                  = 0x92,
    E_ZB_ACTION_DENIED                 = 0x93,
    E_ZB_TIMEOUT                       = 0x94,
    
    E_ZB_HARDWARE_FAILURE              = 0xC0,
    E_ZB_SOFTWARE_FAILURE              = 0xC1,
    E_ZB_CALIBRATION_ERROR             = 0xC2,
} teZbStatus;  

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
typedef void (*tprAttributeUpdate)(tsZigbeeBase *psZigbeeNode, uint16 u16ClusterID, uint16 u16AttributeID, teZCL_ZCLAttributeType eType, tuZcbAttributeData uData);
/* Closures */
typedef teZbStatus (*tpreDeviceSetWindowCovering)(tsZigbeeBase *psZigbeeNode, teCLD_WindowCoveringDevice_CommandID eCommand);

typedef struct
{
    /* Coordinator */
    tpreCoordinatorReset            preCoordinatorReset;
    tpreCoordinatorPermitJoin       preCoordinatorPermitJoin;
    tpreCoordinatorGetChannel       preCoordinatorGetChannel;
    tpreDeviceRemoveNetwork         preDeviceRemoveNetwork;
    tpreDeviceAddBind               preDeviceAddBind;
    tpreDeviceRemoveBind            preDeviceRemoveBind;
    /* Base */
    tpreDeviceSetAttribute          preDeviceSetAttribute;
    tpreDeviceGetAttribute          preDeviceGetAttribute;
    tpreDeviceAddGroup              preDeviceAddGroup;
    tpreDeviceRemoveGroup           preDeviceRemoveGroup;
    tpreDeviceClearGroup            preDeviceClearGroup;
    tpreDeviceAddSence              preDeviceAddSence;
    tpreDeviceSetSence              preDeviceSetSence;
    tpreDeviceGetSence              preDeviceGetSence;
    tpreDeviceRemoveSence           preDeviceRemoveSence;
    /* ZLL */
    tpreDeviceSetOnOff              preDeviceSetOnOff;
    tpreDeviceGetOnOff              preDeviceGetOnOff;
    tpreDeviceSetLightColour        preDeviceSetLightColour;
    tpreDeviceGetLightColour        preDeviceGetLightColour;
    tpreDeviceSetLevel              preDeviceSetLevel;
    tpreDeviceGetLevel              preDeviceGetLevel;
    /* Sensor */
    tpreDeviceGetSensorValue        preDeviceGetSensorValue;
    /* Closures */
    tpreDeviceSetWindowCovering     preDeviceSetWindowCovering;
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
