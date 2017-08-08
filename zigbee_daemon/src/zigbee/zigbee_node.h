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
    //struct timeval  sLastSuccessful;        /**< Time of last successful communications */
    uint16          u16SequentialFailures;  /**< Number of sequential failures */
} tsComm;                                  /**< Structure containing communications statistics */

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
    tsComm              sComm;
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
    char *              psInfo;
    struct dl_list      list;
} tsZigbeeBase;

typedef struct {float H,S,V;}tsHSV;
typedef struct {uint8 R,G,B;}tsRGB;
/* Coordinator */
typedef teZbStatus (*tpreCoordinatorReset)(tsZigbeeBase *psZigbeeNode);
typedef teZbStatus (*tpreCoordinatorPermitJoin)(uint8 time);
typedef teZbStatus (*tpreCoordinatorSearchDevices)(void);
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
typedef teZbStatus (*tpreDeviceSetDoorLockPassword)(tsZigbeeBase *psZigbeeNode, tsCLD_DoorLock_Payload *psDoorLockPayload);

/**
 * Zigbee设备节点的回调函数，在设备初始化时对回调函数赋值，并不是所有回调都会被初始化，
 * 因此调用前需要先判断是否为空。
 * */
typedef struct
{
    /* Coordinator */
    tpreCoordinatorReset            preCoordinatorReset;
    tpreCoordinatorPermitJoin       preCoordinatorPermitJoin;
    tpreCoordinatorSearchDevices    preCoordinatorSearchDevices;
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
/*****************************************************************************
** Prototype    : eZigbee_AddNode
** Description  : 在设备列表中添加一个节点，添加前需要先检测是否存在，如果存在更新节点数据
** Input        : u16ShortAddress, 节点网络地址，必须存在的参数
 *                u64IEEEAddress, Mac Address
 *                u16DeviceID
 *                u8MacCapability,设备节点类型，供电类型等
** Output       : ppsZCBNode，返回新添加的节点指针
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZigbeeAddNode(uint16 u16ShortAddress, uint64 u64IEEEAddress, uint16 u16DeviceID, uint8 u8MacCapability,
                          tsZigbeeNodes **ppsZCBNode);
/*****************************************************************************
** Prototype    : eZigbee_RemoveNode
** Description  : 在设备列表中移除一个节点
** Input        : psZigbeeNode, 节点指针
** Output       : none
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZigbeeRemoveNode(tsZigbeeNodes *psZigbeeNode);
teZbStatus eZigbeeRemoveAllNodes(void);
/*****************************************************************************
** Prototype    : eZigbee_NodeAddEndpoint
** Description  : 对节点添加端点数据，用于后续发送命令时查询
** Input        : psZigbeeNode, 节点指针
 *                u8Endpoint, 端点号，如果存在，则更新数据
 *                u16ProfileID，协议版本
** Output       : ppsEndpoint，返回新添加的端点指针
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZigbeeNodeAddEndpoint(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ProfileID,
                                  tsNodeEndpoint **ppsEndpoint);
/*****************************************************************************
** Prototype    : eZigbee_NodeAddCluster
** Description  : 对端点添加Cluster，在后续对设备进行控制时需要检查设备上是否有此cluster
 * 的支持，目前只添加了input cluster
** Input        : psZigbeeNode, 节点指针
 *                u8Endpoint, 端点号，如果存在，则更新数据
 *                u16ClusterID，Cluster ID
** Output       : none
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZigbeeNodeAddCluster(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID);
/*****************************************************************************
** Prototype    : eZigbeeNodeAddAttribute
** Description  : 对指定端点指定cluster添加attribute，现在只用在了协调器上，记录了协调器
 * 支持哪些属性，在直接控制属性值时可以去查询然后修改，目前没有查询
** Input        : psZigbeeNode, 节点指针
 *                u8Endpoint, 端点号，如果存在，则更新数据
 *                u16ClusterID，Cluster ID
** Output       : none
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZigbeeNodeAddAttribute(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID,
                                   uint16 u16AttributeID);
/*****************************************************************************
** Prototype    : eZigbeeNodeAddCommand
** Description  : 对指定端点指定cluster添加attribute支持的命令，现在只用在了协调器上，
 * 记录了协调器支持哪些命令，在直接控制属性值时可以去查询然后修改，目前没有查询
** Input        : psZigbeeNode, 节点指针
 *                u8Endpoint, 端点号，如果存在，则更新数据
 *                u16ClusterID，Cluster ID
** Output       : none
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZigbeeNodeAddCommand(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint8 u8CommandID);
/*****************************************************************************
** Prototype    : eZigbeeGetEndpoints
** Description  : 获取一个设备指定cluster的端点信息，得到cluster所在的端点号
** Input        : psZigbeeNode, 节点指针
 *                u16ClusterID，Cluster ID
** Output       : pu8Src，源地址端点号，也就是协调器端点号
 *                pu8Dst，目的地址端点号
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZigbeeGetEndpoints(tsZigbeeBase *psZigbee_Node, teZigbee_ClusterID eClusterID, uint8 *pu8Src, uint8 *pu8Dst);
/*****************************************************************************
** Prototype    : psZigbeeNodeFindEndpoint
** Description  : 检查节点是否支持Cluster，如果支持返回cluster所在的endpoint结构体
** Input        : psZigbeeNode, 节点指针
 *                u16ClusterID，Cluster ID
** Output       : none
** Return Value : 成功，返回cluster所在的endpoint指针，否则返回NULL

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
tsNodeEndpoint *psZigbeeNodeFindEndpoint(tsZigbeeBase *psZigbeeNode, uint16 u16ClusterID);
tsZigbeeNodes *psZigbeeFindNodeByShortAddress(uint16 u16ShortAddress);
tsZigbeeNodes *psZigbeeFindNodeByIEEEAddress(uint64 u64IEEEAddress);
void vZigbeePrintNode(tsZigbeeBase *psNode);
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
