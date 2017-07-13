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

    json_object *psJsonDevice = json_object_new_object();
    json_object *psArrayEndpoint = json_object_new_array();
    for (int i = 0; i < psZigbeeNode->sNode.u32NumEndpoints; ++i) {
        json_object *psJsonEndpoint = json_object_new_object();
        json_object_object_add(psJsonEndpoint, "id", json_object_new_int(psZigbeeNode->sNode.pasEndpoints[i].u8Endpoint));
        json_object *psArrayCluster = json_object_new_array();
        for (int j = 0; j < psZigbeeNode->sNode.pasEndpoints[i].u32NumClusters; ++j) {
            json_object *psJsonCluster = json_object_new_object();
            json_object_object_add(psJsonCluster, "id", json_object_new_int(psZigbeeNode->sNode.pasEndpoints[i].pasClusters[j].u16ClusterID));
            json_object_array_add(psArrayCluster, psJsonCluster);
        }
        json_object_object_add(psJsonEndpoint, "cluster", psArrayCluster);
        json_object_array_add(psArrayEndpoint, psJsonEndpoint);
    }
    json_object_object_add(psJsonDevice, "endpoint", psArrayEndpoint);

    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress,
                              psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName,
                              psZigbeeNode->sNode.u8MacCapability, json_object_get_string(psJsonDevice));
    json_object_put(psJsonDevice);

    return E_ZB_OK;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

