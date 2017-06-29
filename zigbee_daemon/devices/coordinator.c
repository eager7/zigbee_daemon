/****************************************************************************
 *
 * MODULE:             Zigbee - door lock controller
 *
 * COMPONENT:          zigbee devices  interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2017-06-29 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2017. All rights reserved
 *
 ***************************************************************************/
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "door_lock_controller.h"
#include "zigbee_devices.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_DEVICES (verbosity >= 5)
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eControlBridgeInitialize(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eControlBridgeInitialize\n");
    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s", "CoorDinator");
    psZigbeeNode->Method.preCoordinatorPermitJoin = eZigbee_SetPermitJoining;
    psZigbeeNode->Method.preCoordinatorGetChannel = eZigbee_GetChannel;
    psZigbeeNode->Method.preDeviceSetDoorLock = eZCB_DoorLockDeviceOperator;
    psZigbeeNode->Method.preDeviceSetDoorLockPassword = eZCB_SetDoorLockPassword;
    psZigbeeNode->Method.preZCB_ResetNetwork = eZigbeeDeviceResetNetwork;
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);
    //sleep(1);psZigbeeNode->Method.preCoordinatorPermitJoin(30);
    return E_ZB_OK;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

