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
#include <json-c/json.h>
#include <zigbee_node.h>
#include "door_lock_controller.h"
#include "zigbee_devices.h"
#include "zigbee_socket.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_DEVICES (verbosity >= 5)
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static teZbStatus eHandleAttributeUpdate(tsZigbeeBase *psZigbeeNode,
                                         uint16 u16ClusterID,
                                         uint16 u16AttributeID,
                                         teZCL_ZCLAttributeType eType,
                                         tuZcbAttributeData uData);
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
    psZigbeeNode->Method.preDeviceAttributeUpdate      = eHandleAttributeUpdate;

    json_object *psJsonDevice = json_object_new_object();
    json_object *psArrayEndpoint = json_object_new_array();
    int i ,j= 0;
    for (i = 0; i < psZigbeeNode->sNode.u32NumEndpoints; ++i) {
        json_object *psJsonEndpoint = json_object_new_object();
        json_object_object_add(psJsonEndpoint, "id", json_object_new_int(psZigbeeNode->sNode.pasEndpoints[i].u8Endpoint));
        json_object *psArrayCluster = json_object_new_array();
        for (j = 0; j < psZigbeeNode->sNode.pasEndpoints[i].u32NumClusters; ++j) {
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

static teZbStatus eHandleAttributeUpdate(tsZigbeeBase *psZigbeeNode,
                                                    uint16 u16ClusterID,
                                                    uint16 u16AttributeID,
                                                    teZCL_ZCLAttributeType eType,
                                                    tuZcbAttributeData uData)
{
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    if(E_ZB_CLUSTERID_POWER == u16ClusterID){
        tuZcbAttributeData uDoorLock;
        switch(eType)
        {
            case(E_ZCL_GINT8):
            case(E_ZCL_UINT8):
            case(E_ZCL_INT8):
            case(E_ZCL_ENUM8):
            case(E_ZCL_BMAP8):
            case(E_ZCL_BOOL):
            case(E_ZCL_OSTRING):
            case(E_ZCL_CSTRING):
                uDoorLock.u8Data = uData.u8Data;
                break;

            case(E_ZCL_LOSTRING):
            case(E_ZCL_LCSTRING):
            case(E_ZCL_STRUCT):
            case(E_ZCL_INT16):
            case(E_ZCL_UINT16):
            case(E_ZCL_ENUM16):
            case(E_ZCL_CLUSTER_ID):
            case(E_ZCL_ATTRIBUTE_ID):
                uDoorLock.u16Data = ntohs(uData.u16Data);
                break;

            case(E_ZCL_UINT24):
            case(E_ZCL_UINT32):
            case(E_ZCL_TOD):
            case(E_ZCL_DATE):
            case(E_ZCL_UTCT):
            case(E_ZCL_BACNET_OID):
                uDoorLock.u32Data = ntohl(uData.u32Data);
                break;

            case(E_ZCL_UINT40):
            case(E_ZCL_UINT48):
            case(E_ZCL_UINT56):
            case(E_ZCL_UINT64):
            case(E_ZCL_IEEE_ADDR):
                uDoorLock.u64Data = be64toh(uData.u64Data);
                break;

            default:
                ERR_vPrintln(T_TRUE,  "Unknown attribute data type (%d) received from node 0x%04X", eType, psZigbeeNode->u16ShortAddress);
                break;
        }
        if(E_CLD_PWRCFG_ATTR_ID_MAINS_VOLTAGE == u16AttributeID){
            INF_vPrintln(DBG_DEVICES, "update door lock power to %d\n", uDoorLock.u16Data);
            psZigbeeNode->sAttributeValue.u16Battery= uDoorLock.u16Data;
                //TODO:Send msg to cloud
                eSocketPowerConfigurationReport((uint8)(uDoorLock.u16Data/30));
        }
    }
    return E_ZB_ERROR;
}
