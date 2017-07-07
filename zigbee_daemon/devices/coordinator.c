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
#include <zigbee_zcl.h>
#include "door_lock_controller.h"
#include "zigbee_devices.h"
#include "zigbee_sqlite.h"
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
/***        Local Functions                                               ***/
/****************************************************************************/
static teZbStatus eHandleCoordinatorAttributeUpdate(tsZigbeeBase *psZigbeeNode,
                                                    uint16 u16ClusterID,
                                                    uint16 u16AttributeID,
                                                    teZCL_ZCLAttributeType eType,
                                                    tuZcbAttributeData uData)
{
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    if(E_ZB_CLUSTERID_DOOR_LOCK == u16ClusterID){
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
        if(E_CLD_DOOR_LOCK_ATTR_ID_LOCK_STATE == u16AttributeID){
            INF_vPrintln(DBG_DEVICES, "update door lock attribute to %d\n", uDoorLock.u8Data);
            psZigbeeNode->sAttributeValue.u8State= uDoorLock.u8Data;
            if(uDoorLock.u8Data == E_CLD_DOOR_LOCK_LOCK_STATE_UNLOCK){
                //TODO:Send msg to cloud
            }
        }
    }
    return E_ZB_ERROR;
}

static teZbStatus eZigbeeDeviceSetDoorLockPassword(tsZigbeeBase *psZigbeeNode, tsCLD_DoorLock_Payload *psDoorLockPayload)
{
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);

    if(psDoorLockPayload->u8AvailableNum == 0){
        eZigbeeSqliteDelDoorLockPassword(psDoorLockPayload->u8PasswordID);
        eZCB_SetDoorLockPassword(psZigbeeNode, psDoorLockPayload->u8PasswordID, T_FALSE, psDoorLockPayload->u8PasswordLen, psDoorLockPayload->psPassword);
    } else {
        //TODO:Store Password into SQL
        eZigbeeSqliteAddDoorLockPassword(psDoorLockPayload->u8PasswordID, 0, psDoorLockPayload->u8AvailableNum,
                                         psDoorLockPayload->u32TimeStart, psDoorLockPayload->u32TimeEnd, psDoorLockPayload->u8PasswordLen,
                                         psDoorLockPayload->psPassword);
        //eZCB_SetDoorLockPassword(psZigbeeNode, psDoorLockPayload->u8PasswordID, T_TRUE, psDoorLockPayload->u8PasswordLen, psDoorLockPayload->psPassword);
    }
    return E_ZB_OK;
}

static teZbStatus eZigbeeCoordinatorSearchDevices()
{
    static int iStart= 0;
    for (int i = 0; i < 5; ++i) {
        eZCB_NeighbourTableRequest(&iStart);
        iStart++;
        sleep(1);
    }
    return E_ZB_OK;
}
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eControlBridgeInitialize(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eControlBridgeInitialize\n");
    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s", "CoorDinator");
    psZigbeeNode->Method.preCoordinatorPermitJoin       = eZigbee_SetPermitJoining;
    psZigbeeNode->Method.preCoordinatorSearchDevices    = eZigbeeCoordinatorSearchDevices;
    psZigbeeNode->Method.preCoordinatorGetChannel       = eZigbee_GetChannel;
    psZigbeeNode->Method.preZCB_ResetNetwork            = eZigbeeDeviceResetNetwork;

    psZigbeeNode->Method.preDeviceSetDoorLock           = eZCB_DoorLockDeviceOperator;
    psZigbeeNode->Method.preDeviceSetDoorLockPassword   = eZigbeeDeviceSetDoorLockPassword;
    //psZigbeeNode->Method.preDeviceAttributeUpdate       = eHandleCoordinatorAttributeUpdate;

    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress,
                              psZigbeeNode->sNode.u16ShortAddress,
                              psZigbeeNode->sNode.u16DeviceID,
                              psZigbeeNode->sNode.auDeviceName,
                              psZigbeeNode->sNode.u8MacCapability);

    //TODO:将还未失效的临时密码发送给协调器
    sleep(1);
    eZCB_DoorLockDeviceOperator(&sControlBridge.sNode, E_CLD_DOOR_LOCK_DEVICE_CMD_LOCK);
    return E_ZB_OK;
}




