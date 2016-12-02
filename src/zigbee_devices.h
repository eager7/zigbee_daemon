/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP Daemon
 *
 * COMPONENT:          JIP Interface to control bridge
 *
 * REVISION:           $Revision: 37346 $
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


#ifndef  ZIGBEE_DEVICES_H_INCLUDED
#define  ZIGBEE_DEVICES_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "utils.h"
#include "zigbee_network.h"
#include "zigbee_control_bridge.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    S_COOR_OK = 0,
    S_COOR_ERROR,
}tsCoor_Status;
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

/** Initialisation function for the Control bridge JIP device. */
teZbStatus eControlBridgeInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eOnOffLightInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eDimmerLightInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eWarmColdLigthInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eColourLightInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eTemperatureSensorInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eLightSensorInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eSimpleSensorInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eSmartPlugInitalise(tsZigbee_Node *psZigbeeNode);
teZbStatus eDimmerSwitchInitalise(tsZigbee_Node *psZigbeeNode);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* JIP_CONTROLBRIDGE_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

