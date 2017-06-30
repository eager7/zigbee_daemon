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
#include <zigbee_node.h>
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
teZbStatus eDoorLockControllerInitialize(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eDoorLockControllerInitialize\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "DoorLockController", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;

    psZigbeeNode->Method.preDeviceSetDoorLock          = eZCB_DoorLockDeviceOperator;

    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress,
                              psZigbeeNode->sNode.u16ShortAddress,
                              psZigbeeNode->sNode.u16DeviceID,
                              psZigbeeNode->sNode.auDeviceName,
                              psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
