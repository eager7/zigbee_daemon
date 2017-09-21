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
/* Default network configuration */
#define CONFIG_DEFAULT_START_MODE        E_START_COORDINATOR
#define CONFIG_DEFAULT_PAN_ID            0x1234567812345678ll
#define CONFIG_DEFAULT_ENDPOINT          0x01
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** Enumerated type of allowable channels */
typedef enum
{
    E_CHANNEL_AUTOMATIC     = 0,
    E_CHANNEL_MINIMUM       = 11,
    E_CHANNEL_DEFAULT       = 26,
    E_CHANNEL_MAXIMUM       = 26,
} teChannel;

/** Structure of Zigbee Device ID mappings.
 *  When a node joins this structure is used to map the Zigbee device to 
 *  a prInitaliseRoutine to created zigbee special device with data.
 */
typedef struct
{
    uint16                  u16ZigbeeDeviceID;           /**< Zigbee Deive ID */
    tpreDeviceInitialise    prInitializeRoutine;         /**< Initialisation routine for the zigbee device */
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
extern teChannel     eChannel;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
** Prototype    : eZCB_Init
** Description  : 初始化节点链表以及链表互斥锁，初始化串口线程，注册串口事件的处理函数
** Input        : cpSerialDevice, the serial device's path
 *                u32BaudRate, the serial device's baud rate
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_Init(char *cpSerialDevice, uint32 u32BaudRate);
/*****************************************************************************
** Prototype    : eZCB_Finish
** Description  : 停止串口线程，注销初始化时注册的串口处理函数
** Input        : none
** Output       : none
** Return Value : return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_Finish(void);
/*****************************************************************************
** Prototype    : eZCB_EstablishComm
** Description  : 通过读取版本号来确认协调器是否已经正常连接并启动，当收到版本号时认为协调器已
 * 正常启动，此时发送复位命令给协调器，让协调器重启以便将cluster列表，attribute列表和命令列表
 * 发送到主机，然后注册到协调器结构体上，作为后续操作的前提条件使用。
** Input        : none
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_COMMS_FAILED

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_EstablishComm(void);
/*****************************************************************************
** Prototype    : eZCB_SetPermitJoining
** Description  : 通过发送E_SL_MSG_PERMIT_JOINING_REQUEST命令来打开ZigBee网络，从而
 * 让ZigBee的设备可以加入网络，此命令带一个时间参数，决定了网络打开时长。
** Input        : u8Interval, 网络打开时间
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_SetPermitJoining(uint8 u8Interval);
/*****************************************************************************
** Prototype    : eZCB_ManagementLeaveRequest
** Description  : 通过发送E_SL_MSG_MANAGEMENT_LEAVE_REQUEST命令来将一个节点踢出网络，
 * 节点在退出网络时会发送确认命令给协调器，协调器将返回E_SL_MSG_MANAGEMENT_LEAVE_RESPONSE
 * 命令给主机，我们需要等待此条命令确认设备离网，但是此命令经常收不到。
** Input        : psZigbeeNode, 要离网的设备
 *                u8Rejoin，设备是否立即重新请求入网
 *                u8RemoveChildren，是否移除此节点下面的子节点
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_ManagementLeaveRequest(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren);
/*****************************************************************************
** Prototype    : eZCB_NeighbourTableRequest
** Description  : 通过发送 E_SL_MSG_MANAGEMENT_LQI_REQUEST 命令来获取协调器的路由表数据，
 * 协调器将会返回 E_SL_MSG_MANAGEMENT_LQI_RESPONSE 消息，并携带路由表中的节点信息，在协调器
 * 重启后路由表中的数据可以重建，因此我们可以根据此表中的数据来重新将网络中节点加入链表，路由器节点
 * 和终端节点的加入方式不同，具体可见下面的函数。
** Input        : pStart, 自增的数值
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_NeighbourTableRequest(int *pStart);
/*****************************************************************************
** Prototype    : eZCB_MatchDescriptorRequest
** Description  : 通过发送 E_SL_MSG_MATCH_DESCRIPTOR_REQUEST 命令来获取一个设备的匹配描述符
 * 协调器会首先发送相应的cluster，如果节点注册了此cluster，那么它就会回应自己的地址，具有此cluster的
 * 端点等信息。
** Input        : u16TargetAddress, 发送的节点目标，可以是广播，那么所以具有对应cluster的节点都会回复，
 * 通常是具体的节点地址，这样可以节省网络资源。
 *                u16ProfileID, 请求匹配的ZigBee协议栈profile，通常为HA协议栈
 *                u8NumInputClusters, 请求的cluster中server cluster的数量
 *                pau16InputClusters, server cluster列表
 *                u8NumOutputClusters, 请求的cluster中client cluster的数量
 *                pau16OutputClusters, client cluster列表
 *                pu8SequenceNo, 应用层序列号
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_MatchDescriptorRequest(uint16 u16TargetAddress,
                                       uint16 u16ProfileID,
                                       uint8 u8NumInputClusters,
                                       uint16 *pau16InputClusters,
                                       uint8 u8NumOutputClusters,
                                       uint16 *pau16OutputClusters,
                                       uint8 *pu8SequenceNo);
/*****************************************************************************
** Prototype    : eZCB_SimpleDescriptorRequest
** Description  : 通过发送 E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST 命令来获取一个设备的简单描述符，
 * 简单描述符中包含设备端点的信息，即端点号，端点的ProfileID，DeviceID，DeviceVersion，输入输出Cluster等。
** Input        : psZigbee_Node, 节点信息
 *                u8Endpoint, 请求简单描述符的端点
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_SimpleDescriptorRequest(tsZigbeeBase *psZigbee_Node, uint8 u8Endpoint);
/*****************************************************************************
** Prototype    : eZCB_IEEEAddressRequest
** Description  : 通过发送 E_SL_MSG_IEEE_ADDRESS_REQUEST 命令来获取一个设备的MAC地址，
 * 设备会回复一个 E_SL_MSG_IEEE_ADDRESS_RESPONSE 的消息。
** Input        : psZigbee_Node, 要请求的节点的信息，通常使用它的网络地址获取MAC地址。
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_IEEEAddressRequest(tsZigbeeBase *psZigbee_Node);
/*****************************************************************************
** Prototype    : eZCB_GetDefaultResponse
** Description  : 通常一条ZigBee命令会要求对方回应一个默认响应，以确保消息送达，如果在应用层
 * 需要获取此消息那么就需要等待 E_SL_MSG_DEFAULT_RESPONSE 的返回。
** Input        : u8SequenceNo, 要等待返回消息的序列号
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_GetDefaultResponse(uint8 u8SequenceNo);
/*****************************************************************************
** Prototype    : eZCB_ZLL_OnOff
** Description  : 设置具有ON/OFF cluster的属性的值
** Input        : psZigbeeNode, 要设置的节点地址
 *                u16GroupAddress, 如果上面是组播或者广播，那么这个值就是组的ID
 *                u8Mode, 是开灯还是关灯,0-OFF,1-ON
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_ZLL_OnOff(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
/*****************************************************************************
** Prototype    : eZCB_ReadAttributeRequest
** Description  : 读取一个cluster中指定Attribute的值
** Input        : psZigbeeNode, 节点地址
 *                u16ClusterID, cluster的ID
 *                u8Direction, 0：客户端读取服务端，1：服务端读取客户端
 *                u8ManufacturerSpecific, 是否是特定cluster
 *                u16ManufacturerID, 制造商ID
 *                u16AttributeID, cluster下attribute的ID
 *                pvData, 存储数据的指针
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_ReadAttributeRequest(tsZigbeeBase *psZigbee_Node,
                                     uint16 u16ClusterID,
                                     uint8 u8Direction,
                                     uint8 u8ManufacturerSpecific,
                                     uint16 u16ManufacturerID,
                                     uint16 u16AttributeID,
                                     void *pvData);
/*****************************************************************************
** Prototype    : eZCB_XXXGroupMembership
** Description  : 设置一个节点的组，可以是添加，删除，或者直接清空
** Input        : psZigbeeNode, 节点地址
 *                u16GroupAddress, 组的ID
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_AddGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
teZbStatus eZCB_RemoveGroupMembership(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
teZbStatus eZCB_ClearGroupMembership(tsZigbeeBase *psZigbeeNode);
/*****************************************************************************
** Prototype    : eZCB_XXXScene
** Description  : 设置一个节点的场景，可以是添加，删除，或者直接清空
** Input        : psZigbeeNode, 节点地址
 *                u16GroupAddress, 组的ID
 *                u8SceneID, 场景ID
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_StoreScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);
teZbStatus eZCB_RecallScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);
teZbStatus eZCB_RemoveScene(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SceneID);
/*****************************************************************************
** Prototype    : eZCB_ZLL_MoveToLevel
** Description  : 设置灯的亮度
** Input        : psZigbeeNode, 节点地址
 *                u16GroupAddress, 组的ID，可以设置一组灯的亮度
 *                u8OnOff, 开还是关
 *                u8Level，亮度值
 *                u16TransitionTime，从当前亮度到目标亮度需要多长时间
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_ZLL_MoveToLevel(tsZigbeeBase *psZigbeeNode,
                                uint16 u16GroupAddress,
                                uint8 u8OnOff,
                                uint8 u8Level,
                                uint16 u16TransitionTime);
/*****************************************************************************
** Prototype    : eZCB_ZLL_MoveToHueSaturation
** Description  : 设置灯的颜色
** Input        : psZigbeeNode, 节点地址
 *                u16GroupAddress, 组的ID，可以设置一组灯的亮度
 *                u8Hue, Hue值
 *                u8Saturation，饱和度值
 *                u16TransitionTime，从当前亮度到目标亮度需要多长时间
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_ZLL_MoveToHueSaturation(tsZigbeeBase *psZigbeeNode,
                                        uint16 u16GroupAddress,
                                        uint8 u8Hue,
                                        uint8 u8Saturation,
                                        uint16 u16TransitionTime);
/*****************************************************************************
** Prototype    : eZCB_WindowCoveringDeviceOperator
** Description  : 设置窗帘
** Input        : psZigbeeNode, 节点地址
 *                eCommand, 控制窗帘的指令，开关停
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_WindowCoveringDeviceOperator(tsZigbeeBase *psZigbeeNode, teCLD_WindowCovering_CommandID eCommand );
/*****************************************************************************
** Prototype    : eZCB_DoorLockDeviceOperator
** Description  : 控制门锁
** Input        : psZigbeeNode, 节点地址
 *                eCommand, 控制门锁的指令，开，关，获取log
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_DoorLockDeviceOperator(tsZigbeeBase *psZigbeeNode, teCLD_DoorLock_CommandID eCommand );
/*****************************************************************************
** Prototype    : eZCB_ChannelRequest
** Description  : 获取协调器信道信息
** Input        : none
** Output       : pu8Channel， 存储信道值
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_ChannelRequest(uint8 *pu8Channel);
/*****************************************************************************
** Prototype    : eZCB_ResetNetwork
** Description  : 清空协调器的网络信息
** Input        : psZigbeeNode, 节点信息
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_ResetNetwork(tsZigbeeBase *psZigbeeNode);
/*****************************************************************************
** Prototype    : eZCB_SetDoorLockPassword
** Description  : 设置门锁临时密码
** Input        : psZigbeeNode, 节点信息
 *                sDoorLockPayload，门锁临时密码
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_SetDoorLockPassword(tsZigbeeBase *psZigbeeNode, uint8 u8PasswordId, uint8 u8Command,
                                    uint8 u8PasswordLen,
                                    const char *psPassword);
/*****************************************************************************
** Prototype    : eZCB_DeviceRecognition
** Description  :
** Input        : u64MacAddress
** Output       : none
** Return Value : Success, return E_ZB_OK, otherwise, return E_ZB_ERROR

** History      :
** Date         : 2017/9/21
** Author       : PCT
*****************************************************************************/
teZbStatus eZCB_DeviceRecognition(uint64 u64MacAddress);

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
