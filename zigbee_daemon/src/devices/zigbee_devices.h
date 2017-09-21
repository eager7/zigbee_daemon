/****************************************************************************
 *
 * MODULE:             zigbee - zigbee daemon
 *
 * COMPONENT:          zigbee devices interface
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



#ifndef __ZIGBEE_DEVICES_H__
#define __ZIGBEE_DEVICES_H__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdint.h>
#include "utils.h"
#include "serial_link.h"
#include "zigbee_sqlite.h"
#include "zigbee_control_bridge.h"
#include "door_lock_controller.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

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
** Prototype    : eXXXXInitialize
** Description  : Initialize a device, set the name, address and callback func
** Input        : psZigbeeNode, the structure of node
** Output       : none
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eOnOffLightInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eDimmerLightInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eColourLightInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eWindowCoveringInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eTemperatureSensorInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eLightSensorInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eSimpleSensorInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eEndDeviceInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eZigbeeDeviceSetDoorLockPassword(tsZigbeeBase *psZigbeeNode, tsCLD_DoorLock_Payload *psDoorLockPayload);


teZbStatus eZigbee_SetPermitJoining(uint8 u8time );
teZbStatus eZigbee_GetChannel(uint8 *pu8Channel );
teZbStatus eZigbeeDeviceSetOnOff(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
teZbStatus eZigbeeDeviceGetOnOff(tsZigbeeBase *psZigbeeNode, uint8 *u8Mode);
teZbStatus eZigbeeDeviceAddGroup(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
teZbStatus eZigbeeDeviceRemoveGroup(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
teZbStatus eZigbeeDeviceClearGroup(tsZigbeeBase *psZigbeeNode);
teZbStatus eZigbeeDeviceAddSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
teZbStatus eZigbeeDeviceRemoveSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
teZbStatus eZigbeeDeviceCallSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
teZbStatus eZigbeeDeviceGetSence(tsZigbeeBase *psZigbeeNode, uint16 *u16SenceId);
teZbStatus eZigbeeDeviceRemoveNetwork(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren);
teZbStatus eZigbeeDeviceSetLevel(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Level, uint16 u16TransitionTime);
teZbStatus eZigbeeDeviceGetLevel(tsZigbeeBase *psZigbeeNode, uint8 *u8Level);
teZbStatus eZigbeeDeviceGetLightColour(tsZigbeeBase *psZigbeeNode, tsRGB *psRGB);
teZbStatus eZigbeeDeviceSetLightColour(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, tsRGB sRGB, uint16 u16TransitionTime);
teZbStatus eZigbeeDeviceSetClosuresState(tsZigbeeBase *psZigbeeNode, teCLD_WindowCovering_CommandID eCommand);
teZbStatus eZigbeeDeviceGetSensorValue(tsZigbeeBase *psZigbeeNode, uint16 *u16SensorValue, teZigbee_ClusterID eClusterId);
teZbStatus eZigbeeDeviceResetNetwork(tsZigbeeBase *psZigbeeNode);
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
