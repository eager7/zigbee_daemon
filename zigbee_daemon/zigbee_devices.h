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
teZbStatus eControlBridgeInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eOnOffLightInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eDimmerLightInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eColourLightInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eWindowCoveringInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eTemperatureSensorInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eLightSensorInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eSimpleSensorInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eEndDeviceInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eDoorLockInitialize(tsZigbeeNodes *psZigbeeNode);
teZbStatus eDoorLockControllerInitialize(tsZigbeeNodes *psZigbeeNode);
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
