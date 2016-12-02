/****************************************************************************
 *
 * MODULE:             Zigbee - JIP Daemon
 *
 * COMPONENT:          JIP Interface to control bridge
 *
 * REVISION:           $Revision: 43420 $
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


/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include <syslog.h>

#include "zigbee_devices.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define MAX_TEMP_VALUE 40
#define MAX_HUMI_VALUE 80
#define MAX_ILLU_VALUE 1000

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static teZbStatus Zigbee_SetPermitJoining(uint8 u8time );
//light
static teZbStatus ZigbeeDeviceSetOnOff(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
static teZbStatus ZigbeeDeviceGetOnOff(tsZigbee_Node *psZigbeeNode, uint8 *u8Mode);
static teZbStatus ZigbeeDeviceSetLightColour(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint32 u32HueSatTarget, uint16 u16TransitionTime);
static teZbStatus ZigbeeDeviceGetLightColour(tsZigbee_Node *psZigbeeNode, uint32 *u32HueSatTarget);
static teZbStatus ZigbeeDeviceSetLevel(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Level, uint16 u16TransitionTime);
static teZbStatus ZigbeeDeviceGetLevel(tsZigbee_Node *psZigbeeNode, uint8 *u8Level);
//genenic
static teZbStatus ZigbeeDeviceAddGroup(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress);
static teZbStatus ZigbeeDeviceRemoveGroup(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress);
static teZbStatus ZigbeeDeviceClearGroup(tsZigbee_Node *psZigbeeNode);
static teZbStatus ZigbeeDeviceAddSence(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
static teZbStatus ZigbeeDeviceRemoveSence(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
static teZbStatus ZigbeeDeviceSetSence(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
static teZbStatus ZigbeeDeviceGetSence(tsZigbee_Node *psZigbeeNode, uint16 *u16SenceId);
//Sensor
static teZbStatus ZigbeeDeviceGetTempValue(tsZigbee_Node *psZigbeeNode, uint16 *u16TempValue);
static teZbStatus ZigbeeDeviceGetHumiValue(tsZigbee_Node *psZigbeeNode, uint16 *u16HumiValue);
static teZbStatus ZigbeeDeviceGetPowerValue(tsZigbee_Node *psZigbeeNode, uint16 *u16PowerValue);
static teZbStatus ZigbeeDeviceGetIlluValue(tsZigbee_Node *psZigbeeNode, uint16 *u16IlluValue);
static teZbStatus ZigbeeDeviceGetSimpleValue(tsZigbee_Node *psZigbeeNode, uint16 *u16SimpleValue);
//management
static teZbStatus ZigbeeDeviceRemoveNetwork(tsZigbee_Node *psZigbeeNode);
static teZbStatus ZigbeeDeviceAddBind(tsZigbee_Node *psSrcZigbeeNode, tsZigbee_Node *psDesZigbeeNode, uint16 u16ClusterID);
static teZbStatus ZigbeeDeviceRemoveBind(tsZigbee_Node *psSrcZigbeeNode, tsZigbee_Node *psDesZigbeeNode, uint16 u16ClusterID);
static void ZigbeeDeviceAttributeUpdate(tsZigbee_Node *psZigbeeNode, uint16 u16ClusterID, 
                                                    uint16 u16AttributeID, teZCL_ZCLAttributeType eType, tuZcbAttributeData uData);
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

teZbStatus eControlBridgeInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eControlBridgeInitalise\n");
    //tsZigbee_Node *psCoorNode = (tsZigbee_Node *)psZigbeeNode ;

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s", "CoorDinator");
    psZigbeeNode->Method.CoordinatorPermitJoin = Zigbee_SetPermitJoining;
    //psZigbeeNode->Method.CoordinatorPermitJoin(200);
        
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
    eZigbeeSqliteAddNewCluster(E_ZIB_ATTRIBUTE_POWER, psZigbeeNode->u64IEEEAddress, 0, (uint64)time((time_t*)NULL), "NULL");
#endif
    return E_ZB_OK;
}

teZbStatus eOnOffLightInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eOnOffLightInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "OnOffLight", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceSetOnOff             = ZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.DeviceGetOnOff             = ZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;

    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;
    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
    eZigbeeSqliteAddNewCluster(E_ZIB_ATTRIBUTE_ONOFF, psZigbeeNode->u64IEEEAddress, 0, (uint64)time((time_t*)NULL), "NULL");
    eZigbeeSqliteAddNewCluster(E_ZIB_ATTRIBUTE_LEVEL, psZigbeeNode->u64IEEEAddress, 0, (uint64)time((time_t*)NULL), "NULL");
#endif
    return E_ZB_OK;
}

teZbStatus eSmartPlugInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eSmartPlugInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "SmartPlug", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceSetOnOff             = ZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.DeviceGetOnOff             = ZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;
    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

teZbStatus eDimmerLightInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eDimmerLightInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "DimmerLight", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceSetOnOff             = ZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.DeviceGetOnOff             = ZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.DeviceSetLevel             = ZigbeeDeviceSetLevel;
    psZigbeeNode->Method.DeviceGetLevel             = ZigbeeDeviceGetLevel;
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;

    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

teZbStatus eDimmerSwitchInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eDimmerSwitchInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "DimmerSwitch", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;

    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

teZbStatus eWarmColdLigthInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eWarmColdLigthInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "W&CLight", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceSetOnOff             = ZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.DeviceGetOnOff             = ZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.DeviceSetLevel             = ZigbeeDeviceSetLevel;
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;
    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

teZbStatus eColourLightInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eColourLightInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "ColourLight", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceSetOnOff             = ZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.DeviceGetOnOff             = ZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.DeviceSetLevel             = ZigbeeDeviceSetLevel;
    psZigbeeNode->Method.DeviceGetLevel             = ZigbeeDeviceGetLevel;
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceSetLightColour       = ZigbeeDeviceSetLightColour;
    psZigbeeNode->Method.DeviceGetLightColour       = ZigbeeDeviceGetLightColour;
    
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;
    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

teZbStatus eTemperatureSensorInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eTemperatureSensorInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "Temp&HumiSensor", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceGetTemperature       = ZigbeeDeviceGetTempValue;
    psZigbeeNode->Method.DeviceGetHumidity          = ZigbeeDeviceGetHumiValue;
    psZigbeeNode->Method.DeviceGetPower             = ZigbeeDeviceGetPowerValue;
    psZigbeeNode->Method.DeviceAttributeUpdate      = ZigbeeDeviceAttributeUpdate;
    
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;
    psZigbeeNode->Method.DeviceAddBind              = ZigbeeDeviceAddBind;
    psZigbeeNode->Method.DeviceRemoveBind           = ZigbeeDeviceRemoveBind;

    psZigbeeNode->Method.DeviceAttributeUpdate      = ZigbeeDeviceAttributeUpdate;
    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

teZbStatus eLightSensorInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eLightSensorInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "LightSensor", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceGetPower             = ZigbeeDeviceGetPowerValue;
    psZigbeeNode->Method.DeviceAttributeUpdate      = ZigbeeDeviceAttributeUpdate;
    psZigbeeNode->Method.DeviceGetIlluminance       = ZigbeeDeviceGetIlluValue;
    
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;
    psZigbeeNode->Method.DeviceAddBind              = ZigbeeDeviceAddBind;
    psZigbeeNode->Method.DeviceRemoveBind           = ZigbeeDeviceRemoveBind;

    psZigbeeNode->Method.DeviceAttributeUpdate      = ZigbeeDeviceAttributeUpdate;
        
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

teZbStatus eSimpleSensorInitalise(tsZigbee_Node *psZigbeeNode)
{
    NOT_vPrintf(verbosity, "eSimpleSensorInitalise\n");

    snprintf(psZigbeeNode->device_name, sizeof(psZigbeeNode->device_name), "%s-%04X", "SimpleSensor", psZigbeeNode->u16ShortAddress);
    psZigbeeNode->Method.DeviceAddGroup             = ZigbeeDeviceAddGroup;
    psZigbeeNode->Method.DeviceRemoveGroup          = ZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.DeviceAddSence             = ZigbeeDeviceAddSence;
    psZigbeeNode->Method.DeviceRemoveSence          = ZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.DeviceGetPower             = ZigbeeDeviceGetPowerValue;
    psZigbeeNode->Method.DeviceAttributeUpdate      = ZigbeeDeviceAttributeUpdate;
    psZigbeeNode->Method.DeviceGetSimple            = ZigbeeDeviceGetSimpleValue;
    
    psZigbeeNode->Method.DeviceSetSence             = ZigbeeDeviceSetSence;
    psZigbeeNode->Method.DeviceGetSence             = ZigbeeDeviceGetSence;
    psZigbeeNode->Method.DeviceClearGroup           = ZigbeeDeviceClearGroup;
    psZigbeeNode->Method.DeviceRemoveNetwork        = ZigbeeDeviceRemoveNetwork;
    psZigbeeNode->Method.DeviceAddBind              = ZigbeeDeviceAddBind;
    psZigbeeNode->Method.DeviceRemoveBind           = ZigbeeDeviceRemoveBind;

    psZigbeeNode->Method.DeviceAttributeUpdate      = ZigbeeDeviceAttributeUpdate;
    
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_NAME);
    psZigbeeNode->u8DeviceOnline = 1;
    eZigbeeSqliteUpdateDeviceTable(psZigbeeNode, E_SQ_DEVICE_ONLINE);
#endif
    return E_ZB_OK;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
static teZbStatus Zigbee_SetPermitJoining(uint8 u8time )
{
    teZbStatus ZbStatus = E_ZB_OK;
        
    mLockLock(&sZigbee_Network.mutex);
    if(E_ZB_OK != eZCB_SetPermitJoining(u8time))
    {
        ZbStatus = E_ZB_ERROR;
    }
    mLockUnlock(&sZigbee_Network.mutex);

    return ZbStatus;
}

static teZbStatus ZigbeeDeviceSetOnOff(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceSetOnOff\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }

    teZbStatus ZbStatus;
    
    if(0 != u16GroupAddress)
    {
        ZbStatus = eZBZLL_OnOff(NULL, u16GroupAddress, u8Mode);
    }
    else
    {
        ZbStatus = eZBZLL_OnOff(psZigbeeNode, 0, u8Mode);
    }
    
    
    return ZbStatus;
}
static teZbStatus ZigbeeDeviceGetOnOff(tsZigbee_Node *psZigbeeNode, uint8 *u8Mode)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetOnOff\n");
    if((NULL == psZigbeeNode)||(NULL == u8Mode))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTERID_ONOFF))
    {
        DBG_vPrintf(verbosity, "Get Value from memory\n");
        *u8Mode = psZigbeeNode->u8LightMode;
    }
    else
    {
        if (eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_ONOFF, 0, 0, 0, E_ZB_ATTRIBUTEID_ONOFF_ONOFF, u8Mode) != E_ZB_OK)
        {
            ERR_vPrintf(T_TRUE, "eZCB_ReadAttributeRequest OnOff Time Out\n");
            ZbStatus = E_ZB_TIMEOUT;
        }
        else
        {
            psZigbeeNode->u8LightMode = *u8Mode;
        }
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceSetLightColour(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint32 u32HueSatTarget , uint16 u16TransitionTime)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceSetLightColor\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    uint16 u16TargetHue        = ((u32HueSatTarget) >> 8) & 0xFFFF;
    uint8  u8TargetSaturation  = ((u32HueSatTarget) >> 0) & 0xFF;

    if (u16TargetHue >= 3600)
    {
        /* Out of bounds */
        return E_ZB_INVALID_VALUE;
    }
    
    // Convert to Zigbee space
    u16TargetHue = ((int)u16TargetHue * 0xFEFF) / 3600;
    u8TargetSaturation =  (u8TargetSaturation * 254) / 255;

    if(0 != u16GroupAddress)
    {
        ZbStatus = eZBZLL_MoveToHueSaturation(NULL, u16GroupAddress, u16TargetHue >> 8, u8TargetSaturation, u16TransitionTime);
    }
    else
    {
        ZbStatus = eZBZLL_MoveToHueSaturation(psZigbeeNode, 1, u16TargetHue >> 8, u8TargetSaturation, u16TransitionTime);
    }
    mLockLock(&psZigbeeNode->mutex);
    psZigbeeNode->u32DeviceHSV = u32HueSatTarget;
    mLockUnlock(&psZigbeeNode->mutex);

    return ZbStatus;
}

static teZbStatus ZigbeeDeviceGetLightColour(tsZigbee_Node *psZigbeeNode, uint32 *u32HueSatTarget)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetLightColour\n");
    if((NULL == psZigbeeNode)||(NULL == u32HueSatTarget))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
    
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL))
    {
        *u32HueSatTarget = psZigbeeNode->u32DeviceRGB;
    }
    else
    {
#if  1   
        uint8  u8TargetHue        = 0;
        uint16 u16TargetHue       = 0;
        uint8  u8CurrentSat       = 0;
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL, 
                        0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTHUE, &u8TargetHue)) != E_ZB_OK)
        {
            mLockUnlock(&psZigbeeNode->mutex);
            return E_ZB_TIMEOUT;
        }
        
        DBG_vPrintf(verbosity, "Current Hue attribute read as: 0x%04X\n", u8TargetHue);
        
        u16TargetHue = ((int)u8TargetHue * 3600) / 254;

        if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL, 
                        0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTSAT, &u8CurrentSat)) != E_ZB_OK)
        {
            mLockUnlock(&psZigbeeNode->mutex);
            return E_ZB_TIMEOUT;
        }
        else
        {
            DBG_vPrintf(verbosity, "Current Saturation attribute read as: 0x%02X\n", u8CurrentSat);
            *u32HueSatTarget = (u16TargetHue << 8) | (u8CurrentSat);
            psZigbeeNode->u32DeviceRGB = *u32HueSatTarget;
        }
#else
        uint16 u16CurrentX, u16CurrentY;
        uint32 u32XYTarget;
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL, 
                        0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTX, &u16CurrentX)) != E_ZB_OK)
        {
            mLockUnlock(&psZigbeeNode->mutex);
            return E_ZB_TIMEOUT;
        }
        
        DBG_vPrintf(verbosity, "Current X attribute read as: %d\n", u16CurrentX);
        
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL, 
                        0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTY, &u16CurrentY)) != E_ZB_OK)
        {
            mLockUnlock(&psZigbeeNode->mutex);
            return E_ZB_TIMEOUT;
        }
        u32XYTarget = (u16CurrentX << 16) | (u16CurrentY);
        *u32HueSatTarget = u32XYTarget;

#endif
    }
    mLockUnlock(&psZigbeeNode->mutex);


    return ZbStatus;
}

static teZbStatus ZigbeeDeviceSetLevel(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Level, uint16 u16TransitionTime)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceSetLevel\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    if(0 != u16GroupAddress)
    {
        ZbStatus = eZBZLL_MoveToLevel(NULL, u16GroupAddress, 1, u8Level, u16TransitionTime);
    }
    else
    {
        ZbStatus = eZBZLL_MoveToLevel(psZigbeeNode, 0, 1, u8Level, u16TransitionTime);
    }

    return ZbStatus;
}

static teZbStatus ZigbeeDeviceGetLevel(tsZigbee_Node *psZigbeeNode, uint8 *u8Level)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetLevel\n");
    if((NULL == psZigbeeNode)||(NULL == u8Level))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTERID_LEVEL_CONTROL))
    {
        DBG_vPrintf(verbosity, "Get Value from memory\n");
        *u8Level = psZigbeeNode->u8LightLevel;
    }
    else
    {
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_LEVEL_CONTROL, 
                            0, 0, 0, E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL, u8Level)) != E_ZB_OK)
        {
            ZbStatus = E_ZB_ERROR;
        }
        else
        {
            psZigbeeNode->u8LightLevel = *u8Level;
        }
    }
    mLockUnlock(&psZigbeeNode->mutex);
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceAddGroup(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddGroup\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }

    mLockLock(&psZigbeeNode->mutex);
    teZbStatus  ZbStatus = eZCB_AddGroupMembership(psZigbeeNode, u16GroupAddress);
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceRemoveGroup(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddGroup\n");
    teZbStatus ZbStatus = E_ZB_ERROR;
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return ZbStatus;
    }

    mLockLock(&psZigbeeNode->mutex);
    ZbStatus = eZCB_RemoveGroupMembership(psZigbeeNode, u16GroupAddress);
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceClearGroup(tsZigbee_Node *psZigbeeNode)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddGroup\n");
    teZbStatus ZbStatus = E_ZB_ERROR;
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return ZbStatus;
    }

    mLockLock(&psZigbeeNode->mutex);
    ZbStatus = eZCB_ClearGroupMembership(psZigbeeNode);
    ZbStatus = eZCB_AddGroupMembership(psZigbeeNode, 0xf00f);
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceAddSence(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddGroup\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_StoreScene(psZigbeeNode, u16GroupAddress, u16SenceId & 0xFF);
    }
    else
    {
        ZbStatus = eZCB_StoreScene(psZigbeeNode, 0xf00f, u16SenceId & 0xFF);
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceRemoveSence(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddGroup\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_RemoveScene(psZigbeeNode, u16GroupAddress, u16SenceId & 0xFF);
    }
    else
    {
        ZbStatus = eZCB_RemoveScene(psZigbeeNode, 0xf00f, u16SenceId & 0xFF);
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceSetSence(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddGroup\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_RecallScene(psZigbeeNode, u16GroupAddress, u16SenceId & 0xFF);
    }
    else
    {
        ZbStatus = eZCB_RecallScene(psZigbeeNode, 0xf00f, u16SenceId & 0xFF);
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

 
static teZbStatus ZigbeeDeviceGetSence(tsZigbee_Node *psZigbeeNode, uint16 *u16SenceId)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddGroup\n");
    if((NULL == psZigbeeNode)||(NULL == u16SenceId))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;
    uint8     u8CurrentScene;

    mLockLock(&psZigbeeNode->mutex);
    if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_SCENES, 
                        0, 0, 0, E_ZB_ATTRIBUTEID_SCENE_CURRENTSCENE, &u8CurrentScene)) != E_ZB_OK)
    {
        ZbStatus = E_ZB_TIMEOUT;
    }
    mLockUnlock(&psZigbeeNode->mutex);
    *u16SenceId = (uint16)u8CurrentScene;
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceGetTempValue(tsZigbee_Node *psZigbeeNode, uint16 *u16TempValue)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetTempValue\n");
    if((NULL == psZigbeeNode)||(NULL == u16TempValue))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
#ifdef END_DEVICE    
    if(T_TRUE)
#else
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTERID_TEMPERATURE))
#endif
    {
        DBG_vPrintf(verbosity, "Get Value from memory\n");
        *u16TempValue = psZigbeeNode->u16TempValue;
    }
    else
    {
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, 
                E_ZB_CLUSTERID_TEMPERATURE, 0, 0, 0, E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED, u16TempValue)) != E_ZB_OK)
        {
            WAR_vPrintf(verbosity, "===E_JIP_ERROR_TIMEOUT===\n");
            ZbStatus = E_ZB_TIMEOUT;
        }
        else
        {
            psZigbeeNode->u16TempValue = *u16TempValue;
        }
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceGetHumiValue(tsZigbee_Node *psZigbeeNode, uint16 *u16HumiValue)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetHumiValue\n");
    if((NULL == psZigbeeNode)||(NULL == u16HumiValue))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
#ifdef END_DEVICE    
    if(T_TRUE)
#else
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTERID_HUMIDITY))
#endif
    {
        DBG_vPrintf(verbosity, "Get Value from memory\n");
        *u16HumiValue = psZigbeeNode->u16HumiValue;
    }
    else
    {
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, 
                E_ZB_CLUSTERID_HUMIDITY, 0, 0, 0, E_CLD_RHMEAS_ATTR_ID_MEASURED_VALUE, u16HumiValue)) != E_ZB_OK)
        {
            syslog(LOG_INFO, "===E_JIP_ERROR_TIMEOUT===\n");
            ZbStatus = E_ZB_TIMEOUT;
        }
        else
        {
            psZigbeeNode->u16HumiValue = *u16HumiValue;
        }
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceGetPowerValue(tsZigbee_Node *psZigbeeNode, uint16 *u16PowerValue)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetPowerValue\n");
    if((NULL == psZigbeeNode)||(NULL == u16PowerValue))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
#ifdef END_DEVICE    
    if(T_TRUE)
#else
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTER_ID_POWER_CONFIGURATION))
#endif
    {
        DBG_vPrintf(verbosity, "Get Value from memory\n");
        *u16PowerValue = psZigbeeNode->u16PowerValue;
    }
    else
    {
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, 
                E_ZB_CLUSTER_ID_POWER_CONFIGURATION, 0, 0, 0, E_CLD_PWRCFG_ATTR_ID_MAINS_VOLTAGE, u16PowerValue)) != E_ZB_OK)
        {
            ERR_vPrintf(T_TRUE, "===E_JIP_ERROR_TIMEOUT===\n");
            ZbStatus = E_ZB_TIMEOUT;
        }
        else
        {
            psZigbeeNode->u16PowerValue = *u16PowerValue;
        }
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceGetIlluValue(tsZigbee_Node *psZigbeeNode, uint16 *u16IlluValue)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetIlluValue\n");
    if((NULL == psZigbeeNode)||(NULL == u16IlluValue))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
#ifdef END_DEVICE    
    if(T_TRUE)
#else
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTERID_ILLUMINANCE))
#endif
    {
        DBG_vPrintf(verbosity, "Get Value from memory\n");
        *u16IlluValue = psZigbeeNode->u16IlluValue;
    }
    else
    {
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, 
                E_ZB_CLUSTERID_ILLUMINANCE, 0, 0, 0, E_CLD_ILLMEAS_ATTR_ID_MEASURED_VALUE, u16IlluValue)) != E_ZB_OK)
        {
            ERR_vPrintf(T_TRUE, "===E_JIP_ERROR_TIMEOUT===\n");
            ZbStatus = E_ZB_TIMEOUT;
        }
        else
        {
            psZigbeeNode->u16IlluValue = *u16IlluValue;
        }
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceGetSimpleValue(tsZigbee_Node *psZigbeeNode, uint16 *u16SimpleValue)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceGetSimpleValue\n");
    if((NULL == psZigbeeNode)||(NULL == u16SimpleValue))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;

    mLockLock(&psZigbeeNode->mutex);
#ifdef END_DEVICE    
    if(T_TRUE)
#else
    if(!iZigbee_ClusterTimedOut(psZigbeeNode, E_ZB_CLUSTERID_BINARY_INPUT_BASIC))
#endif
    {
        DBG_vPrintf(verbosity, "Get Value from memory\n");
        *u16SimpleValue = psZigbeeNode->u16SimpleValue;
    }
    else
    {
        if ((eZCB_ReadAttributeRequest(psZigbeeNode, 
                    E_ZB_CLUSTERID_BINARY_INPUT_BASIC, 0, 0, 0, E_CLD_BINARY_INPUT_BASIC_ATTR_ID_OUT_OF_SERVICE, u16SimpleValue)) != E_ZB_OK)
        {
            ERR_vPrintf(T_TRUE, "===E_JIP_ERROR_TIMEOUT===\n");
            ZbStatus = E_ZB_TIMEOUT;
        }
        else
        {
            psZigbeeNode->u16SimpleValue = *u16SimpleValue;
        }
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceRemoveNetwork(tsZigbee_Node *psZigbeeNode)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceRemoveNetwork\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;
    
    mLockLock(&psZigbeeNode->mutex);
    if (eZCB_ManagementLeaveRequest(psZigbeeNode) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "===E_JIP_ERROR_TIMEOUT===\n");
        ZbStatus = E_ZB_TIMEOUT;
    }
    mLockUnlock(&psZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceAddBind(tsZigbee_Node *psSrcZigbeeNode, tsZigbee_Node *psDesZigbeeNode, uint16 u16ClusterID)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceAddBind\n");
    if((NULL == psSrcZigbeeNode)||(NULL == psDesZigbeeNode))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;
    
    mLockLock(&psSrcZigbeeNode->mutex);
    if (eZCB_BindRequest(psSrcZigbeeNode, psDesZigbeeNode->u64IEEEAddress, u16ClusterID & 0xFF) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "===E_JIP_ERROR_TIMEOUT===\n");
        ZbStatus = E_ZB_TIMEOUT;
    }
    mLockUnlock(&psSrcZigbeeNode->mutex);
    
    return ZbStatus;
}

static teZbStatus ZigbeeDeviceRemoveBind(tsZigbee_Node *psSrcZigbeeNode, tsZigbee_Node *psDesZigbeeNode, uint16 u16ClusterID)
{
    DBG_vPrintf(verbosity, "ZigbeeDeviceRemoveBind\n");
    if((NULL == psSrcZigbeeNode)||(NULL == psDesZigbeeNode))
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return E_ZB_ERROR;
    }
    teZbStatus ZbStatus = E_ZB_OK;
    
    mLockLock(&psSrcZigbeeNode->mutex);
    if (eZCB_UnBindRequest(psSrcZigbeeNode, psDesZigbeeNode->u64IEEEAddress, u16ClusterID & 0xFF) != E_ZB_OK)
    {
        ERR_vPrintf(T_TRUE, "===E_JIP_ERROR_TIMEOUT===\n");
        ZbStatus = E_ZB_TIMEOUT;
    }
    mLockUnlock(&psSrcZigbeeNode->mutex);
    
    return ZbStatus;
}

static void ZigbeeDeviceAttributeUpdate(tsZigbee_Node *psZigbeeNode, uint16 u16ClusterID, 
                                                    uint16 u16AttributeID, teZCL_ZCLAttributeType eType, tuZcbAttributeData uData)
{
    //DBG_vPrintf(verbosity, "ZigbeeDeviceAttributeUpdate\n");
    if(NULL == psZigbeeNode)
    {
        ERR_vPrintf(T_TRUE, "pointer is null\n");
        return;
    }

    
    mLockLock(&psZigbeeNode->mutex);
    switch(u16ClusterID)
    {
        case (E_ZB_CLUSTERID_TEMPERATURE):
        {
            vZigbee_NodeUpdateClusterComms(psZigbeeNode, E_ZB_CLUSTERID_TEMPERATURE);
            switch (u16AttributeID)
            {
                case(E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED):
                {
                    INF_vPrintf(verbosity, "Update local temperature to %d\n", uData.u16Data);
                    psZigbeeNode->u16TempValue = uData.u16Data;
                    if(uData.u16Data >= MAX_TEMP_VALUE)
                    {
                        psZigbeeNode->u8DeviceTempAlarm = 1;
                        ERR_vPrintf(T_TRUE, "===Temperature Danger!===\n");
                    }
                    else
                    {
                        psZigbeeNode->u8DeviceTempAlarm = 0;
                    }
                }
                break;
                    
                default:
                    INF_vPrintf(verbosity, "Update for unknown attribute\n");
                break;
            }
        }
        break;
        
        case (E_ZB_CLUSTER_ID_POWER_CONFIGURATION):
        {
            vZigbee_NodeUpdateClusterComms(psZigbeeNode, E_ZB_CLUSTER_ID_POWER_CONFIGURATION);
            switch (u16AttributeID)
            {
                case(E_CLD_PWRCFG_ATTR_ID_MAINS_VOLTAGE):
                {
                    INF_vPrintf(verbosity, "Update local Power to %d\n", uData.u16Data*100/1024);
                    psZigbeeNode->u16PowerValue = uData.u16Data*100/1024;
                }
                break;
                    
                default:
                    INF_vPrintf(verbosity, "Update for unknown attribute\n");
                break;
            }
        }
        break;
        
        case (E_ZB_CLUSTERID_HUMIDITY):
        {
            vZigbee_NodeUpdateClusterComms(psZigbeeNode, E_ZB_CLUSTERID_HUMIDITY);
            switch (u16AttributeID)
            {
                case(E_CLD_RHMEAS_ATTR_ID_MEASURED_VALUE):
                {
                    INF_vPrintf(verbosity, "Update local Humidity to %d\n", uData.u16Data);
                    psZigbeeNode->u16HumiValue = uData.u16Data;
                    if(uData.u16Data >= MAX_HUMI_VALUE)
                    {
                        psZigbeeNode->u8DeviceHumiAlarm = 1;
                        ERR_vPrintf(T_TRUE, "===Humidity Danger!===\n");
                    }
                    else
                    {
                        psZigbeeNode->u8DeviceHumiAlarm = 0;
                    }
                }
                break;
                    
                default:
                    INF_vPrintf(verbosity, "Update for unknown attribute\n");
                break;
            }
        }
        break;
        
        case (E_ZB_CLUSTERID_ILLUMINANCE):
        {
            vZigbee_NodeUpdateClusterComms(psZigbeeNode, E_ZB_CLUSTERID_ILLUMINANCE);
              switch (u16AttributeID)
              {
                  case(E_CLD_ILLMEAS_ATTR_ID_MEASURED_VALUE):
                  {
                      INF_vPrintf(verbosity, "Update local Illu to %d\n", uData.u16Data);
                      psZigbeeNode->u16IlluValue = uData.u16Data;
                  }
                  break;
                      
                  default:
                      INF_vPrintf(verbosity, "Update for unknown attribute\n");
                  break;
              }
        }
        break;
        
        case (E_ZB_CLUSTERID_BINARY_INPUT_BASIC):
        {
            vZigbee_NodeUpdateClusterComms(psZigbeeNode, E_ZB_CLUSTERID_BINARY_INPUT_BASIC);
            switch (u16AttributeID)
            {
                case(E_CLD_BINARY_INPUT_BASIC_ATTR_ID_OUT_OF_SERVICE):
                {
                    INF_vPrintf(verbosity, "Update local Binary to %d\n", uData.u8Data);
                    psZigbeeNode->u16SimpleValue = uData.u8Data;
                    if(uData.u8Data >= 1)
                    {
                        psZigbeeNode->u8DeviceSimpAlarm = 1;
                        ERR_vPrintf(T_TRUE, "===Binary Danger!===\n");                        
                    }
                    else
                    {
                        psZigbeeNode->u8DeviceSimpAlarm = 0;
                    }
                }
                break;

                default:
                    INF_vPrintf(verbosity, "Update for unknown attribute\n");
                break;
            }
        }
        break;
        
        default:
            DBG_vPrintf(verbosity, "Update unknow Cluster\n");
        break;
    }
    vZigbee_NodeUpdateClusterComms(psZigbeeNode, u16ClusterID);
    mLockUnlock(&psZigbeeNode->mutex);
    //DBG_vPrintf(verbosity, "Device Cluster Up Done\n");
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

