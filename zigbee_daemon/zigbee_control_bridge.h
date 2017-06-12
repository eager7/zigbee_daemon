/****************************************************************************
 *
 * MODULE:             zigbee - zigbee daemon
 *
 * COMPONENT:          zigbee cordinator interface
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



#ifndef __ZIGBEE_CONTROL_BRIDGE_H__
#define __ZIGBEE_CONTROL_BRIDGE_H__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdint.h>
#include "utils.h"
#include "zigbee_node.h"
#include "zigbee_zcl.h"
#include "zigbee_sqlite.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define ZB_ENDPOINT_ZHA             1   // Zigbee-HA
#define ZB_ENDPOINT_ONOFF           1   // ON/OFF cluster
#define ZB_ENDPOINT_GROUP           1   // GROUP cluster
#define ZB_ENDPOINT_SCENE           1   // SCENE cluster
#define ZB_ENDPOINT_SIMPLE          1   // SimpleDescriptor cluster
#define ZB_ENDPOINT_LAMP            1   // ON/OFF, Color control
#define ZB_ENDPOINT_TUNNEL          1   // For tunnel mesages
#define ZB_ENDPOINT_ATTR            1   // For attrs

/* Default network configuration */
#define CONFIG_DEFAULT_START_MODE       E_START_COORDINATOR
#define CONFIG_DEFAULT_CHANNEL          E_CHANNEL_DEFAULT
#define CONFIG_DEFAULT_PANID            0x1234567812345678ll

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** Enumerated type of allowable channels */
typedef enum
{
    E_CHANNEL_AUTOMATIC     = 0,
    E_CHANNEL_MINIMUM       = 11,
    E_CHANNEL_DEFAULT       = 20,
    E_CHANNEL_MAXIMUM       = 26
} teChannel;

/** Structure of Zigbee Device ID mappings.
 *  When a node joins this structure is used to map the Zigbee device to 
 *  a prInitaliseRoutine to created zigbee special device with data.
 */
typedef struct
{
    uint16                  u16ZigbeeDeviceID;          /**< Zigbee Deive ID */
    tpreDeviceInitialise    prInitaliseRoutine;         /**< Initialisation routine for the zigbee device */
} tsDeviceIDMap;

typedef struct
{
  uint8    u8ClusterCount;
  uint16   au16Clusters[];
}PACKED tsZDClusterList;

/** Default response message */
typedef struct
{
    uint8             u8SequenceNo;           /**< Sequence number of outgoing message */
    uint8             u8Endpoint;             /**< Source endpoint */
    uint16            u16ClusterID;           /**< Source cluster ID */
    uint8             u8CommandID;            /**< Source command ID */
    uint8             u8Status;               /**< Command status */
} PACKED tsSL_Msg_DefaultResponse;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern tsZigbeeNodes sControlBridge;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eZCB_Init(char *cpSerialDevice, uint32 u32BaudRate);
teZbStatus eZCB_Finish(void);
teZbStatus eZCB_EstablishComms(void);
teZbStatus eZCB_SetPermitJoining(uint8 u8Interval);
teZbStatus eZCB_ManagementLeaveRequest(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren);
teZbStatus eZCB_NeighbourTableRequest(int *pStart);
teZbStatus eZCB_MatchDescriptorRequest(uint16 u16TargetAddress, uint16 u16ProfileID, uint8 u8NumInputClusters, uint16 *pau16InputClusters,  uint8 u8NumOutputClusters, uint16 *pau16OutputClusters,uint8 *pu8SequenceNo);
teZbStatus eZCB_IEEEAddressRequest(tsZigbeeBase *psZigbee_Node);
teZbStatus eZCB_GetDefaultResponse(uint8 u8SequenceNo);
teZbStatus eZCB_ZLL_OnOff(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
teZbStatus eZCB_ReadAttributeRequest(tsZigbeeBase *psZigbee_Node, uint16 u16ClusterID,uint8 u8Direction, uint8 u8ManufacturerSpecific, uint16 u16ManufacturerID, uint16 u16AttributeID, void *pvData);
teZbStatus eZCB_AddGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
teZbStatus eZCB_RemoveGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
teZbStatus eZCB_ClearGroupMembership(tsZigbeeBase *psZigbeeNode);
teZbStatus eZCB_StoreScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);
teZbStatus eZCB_RecallScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);
teZbStatus eZCB_RemoveScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);

teZbStatus eZCB_ZLL_MoveToLevel(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8OnOff, uint8 u8Level, uint16 u16TransitionTime);
teZbStatus eZCB_ZLL_MoveToHueSaturation(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Hue, uint8 u8Saturation, uint16 u16TransitionTime);

teZbStatus eZCB_WindowCoveringDeviceOperator(tsZigbeeBase *psZigbeeNode, teCLD_WindowCoveringDevice_CommandID eCommand );
teZbStatus eZCB_ChannelRequest(uint8 *pu8Channel);

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
