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
teZbStatus eControlBridgeInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eOnOffLightInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eDimmerLightInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eColourLightInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eWindowCoveringInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eTemperatureSensorInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eLightSensorInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eSimpleSensorInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eEndDeviceInitalise(tsZigbeeNodes *psZigbeeNode);
teZbStatus eDoorLockInitalise(tsZigbeeNodes *psZigbeeNode);

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
