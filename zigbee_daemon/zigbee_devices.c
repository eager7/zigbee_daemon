/****************************************************************************
 *
 * MODULE:             Zigbee - daemon
 *
 * COMPONENT:          zigbee devices  interface
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
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "zigbee_devices.h"
#include "zigbee_node.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_DEVICES (verbosity >= 5)
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static teZbStatus eZigbee_SetPermitJoining(uint8 u8time );
static teZbStatus eZigbee_GetChannel(uint8 *pu8Channel );
static teZbStatus eZigbeeDeviceSetOnOff(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
static teZbStatus eZigbeeDeviceGetOnOff(tsZigbeeBase *psZigbeeNode, uint8 *u8Mode);
static teZbStatus eZigbeeDeviceAddGroup(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
static teZbStatus eZigbeeDeviceRemoveGroup(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress);
static teZbStatus eZigbeeDeviceClearGroup(tsZigbeeBase *psZigbeeNode);
static teZbStatus eZigbeeDeviceAddSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
static teZbStatus eZigbeeDeviceRemoveSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
static teZbStatus eZigbeeDeviceCallSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId);
static teZbStatus eZigbeeDeviceGetSence(tsZigbeeBase *psZigbeeNode, uint16 *u16SenceId);
static teZbStatus eZigbeeDeviceRemoveNetwork(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren);
static teZbStatus eZigbeeDeviceSetLevel(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Level, uint16 u16TransitionTime);
static teZbStatus eZigbeeDeviceGetLevel(tsZigbeeBase *psZigbeeNode, uint8 *u8Level);
static teZbStatus eZigbeeDeviceGetLightColour(tsZigbeeBase *psZigbeeNode, tsRGB *psRGB);
static teZbStatus eZigbeeDeviceSetLightColour(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, tsRGB sRGB, uint16 u16TransitionTime);
static teZbStatus eZigbeeDeviceSetClosuresState(tsZigbeeBase *psZigbeeNode, teCLD_WindowCovering_CommandID eCommand);
static teZbStatus eZigbeeDeviceGetSensorValue(tsZigbeeBase *psZigbeeNode, uint16 *u16SensorValue, teZigbee_ClusterID eClusterId);
static teZbStatus eZigbeeDeviceSetDoorLockState(tsZigbeeBase *psZigbeeNode, teCLD_DoorLock_CommandID eCommand);

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
teZbStatus eControlBridgeInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eControlBridgeInitalise\n");
    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s", "CoorDinator");
    psZigbeeNode->Method.preCoordinatorPermitJoin = eZigbee_SetPermitJoining;
    psZigbeeNode->Method.preCoordinatorGetChannel = eZigbee_GetChannel;
    psZigbeeNode->Method.preDeviceSetDoorLock = eZigbeeDeviceSetDoorLockState;
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);
    //sleep(1);psZigbeeNode->Method.preCoordinatorPermitJoin(30);
        
    return E_ZB_OK;
}

teZbStatus eOnOffLightInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eOnOffLightInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "OnOffLight", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceSetOnOff             = eZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.preDeviceGetOnOff             = eZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);
    
    return E_ZB_OK;
}

teZbStatus eDimmerLightInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eDimmerLightInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "DimmerLight", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceSetOnOff             = eZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.preDeviceGetOnOff             = eZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.preDeviceSetLevel             = eZigbeeDeviceSetLevel;
    psZigbeeNode->Method.preDeviceGetLevel             = eZigbeeDeviceGetLevel;
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;

    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eColourLightInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eColourLightInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "ColourLight", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceSetOnOff             = eZigbeeDeviceSetOnOff;
    psZigbeeNode->Method.preDeviceGetOnOff             = eZigbeeDeviceGetOnOff;
    psZigbeeNode->Method.preDeviceSetLevel             = eZigbeeDeviceSetLevel;
    psZigbeeNode->Method.preDeviceGetLevel             = eZigbeeDeviceGetLevel;
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetLightColour       = eZigbeeDeviceSetLightColour;
    psZigbeeNode->Method.preDeviceGetLightColour       = eZigbeeDeviceGetLightColour;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;
    
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eWindowCoveringInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eWindowCoveringInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "WindowCovering", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceSetWindowCovering    = eZigbeeDeviceSetClosuresState;
    
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;
    
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eTemperatureSensorInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eTemperatureSensorInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "Temp&Humi", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceGetSensorValue       = eZigbeeDeviceGetSensorValue;
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;
    
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eLightSensorInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eLightSensorInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "LightSensor", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceGetSensorValue       = eZigbeeDeviceGetSensorValue;
    
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;
    
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eSimpleSensorInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eSimpleSensorInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "SimpleSensor", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceGetSensorValue       = eZigbeeDeviceGetSensorValue;
    
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;
    
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eEndDeviceInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eEndDeviceInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "EndDevice", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceGetSensorValue       = eZigbeeDeviceGetSensorValue;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;
    
    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eDoorLockInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eDoorLockInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "DoorLock", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceSetDoorLock          = eZigbeeDeviceSetDoorLockState;
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;

    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

teZbStatus eDoorLockControllerInitalise(tsZigbeeNodes *psZigbeeNode)
{
    NOT_vPrintln(DBG_DEVICES, "------------eDoorLockControllerInitalise\n");

    snprintf(psZigbeeNode->sNode.auDeviceName, sizeof(psZigbeeNode->sNode.auDeviceName), "%s-%04X", "DoorLockController", psZigbeeNode->sNode.u16ShortAddress);
    psZigbeeNode->Method.preDeviceSetDoorLock          = eZigbeeDeviceSetDoorLockState;
    psZigbeeNode->Method.preDeviceAddGroup             = eZigbeeDeviceAddGroup;
    psZigbeeNode->Method.preDeviceRemoveGroup          = eZigbeeDeviceRemoveGroup;
    psZigbeeNode->Method.preDeviceAddScene             = eZigbeeDeviceAddSence;
    psZigbeeNode->Method.preDeviceRemoveScene          = eZigbeeDeviceRemoveSence;
    psZigbeeNode->Method.preDeviceSetScene             = eZigbeeDeviceCallSence;
    psZigbeeNode->Method.preDeviceGetScene             = eZigbeeDeviceGetSence;
    psZigbeeNode->Method.preDeviceClearGroup           = eZigbeeDeviceClearGroup;
    psZigbeeNode->Method.preDeviceRemoveNetwork        = eZigbeeDeviceRemoveNetwork;

    eZigbeeSqliteAddNewDevice(psZigbeeNode->sNode.u64IEEEAddress, psZigbeeNode->sNode.u16ShortAddress, psZigbeeNode->sNode.u16DeviceID, psZigbeeNode->sNode.auDeviceName, psZigbeeNode->sNode.u8MacCapability);

    return E_ZB_OK;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
tsHSV RGB_HSV(unsigned char R, unsigned char G, unsigned char B)
{
	#define MAX(a,b,c) (a>b?(a>c?a:c):(b>c?b:c))
	#define MIN(a,b,c) (a<b?(a<c?a:c):(b<c?b:c))   
    float V = MAX(R, G, B);
    float S = (V==0)?0:(V-MIN(R,G,B));
	float H = (V == R)?(60*(G-B) / (V-MIN(R,G,B))):((V==G)?(120 + 60*(B-R) / (V - MIN(R,G,B))):(H = 240 + 60*(R-G) / (V - MIN(R,G,B))));
	tsHSV HSV;
	HSV.H = (H < 0)?(H+360):H;
	HSV.S = S/255;
	HSV.V = V/255;
	return HSV;
}
tsRGB HSV_RGB(float H, float S, float V)
{
	tsRGB RGB;
	unsigned char hi = ((unsigned int)(H / 60)) % 6;
	float f = H / 60 - (float)hi;
	float p = V * (1 - S);
	float q = V * (1 - f * S);
	float t = V * (1 - (1 - f) * S);
	#define SET(a,b,c) do{RGB.R=(unsigned char)(a*255);RGB.G=(unsigned char)(b*255);RGB.B=(unsigned char)(c*255);}while(0)
	switch(hi){
		case 0:{SET(V,t,p);}break;
		case 1:{SET(q,V,p);}break;
		case 2:{SET(p,V,t);}break;
		case 3:{SET(p,q,V);}break;
		case 4:{SET(t,p,V);}break;
		case 5:{SET(V,p,q);}break;
	}
	return RGB;
}

static teZbStatus eZigbee_SetPermitJoining(uint8 u8time )
{
    DBG_vPrintln(DBG_DEVICES, "eZigbee_SetPermitJoining\n");
    teZbStatus ZbStatus = E_ZB_OK;
    if(E_ZB_OK != eZCB_SetPermitJoining(u8time)){
        ZbStatus = E_ZB_ERROR;
    }

    return ZbStatus;
}

static teZbStatus eZigbee_GetChannel(uint8 *pu8Channel )
{
    DBG_vPrintln(DBG_DEVICES, "eZigbee_GetChannel\n");
    teZbStatus ZbStatus = E_ZB_OK;
    if(E_ZB_OK != eZCB_ChannelRequest(pu8Channel)){
        ZbStatus = E_ZB_ERROR;
    }
    DBG_vPrintln(DBG_DEVICES, "eZigbee_GetChannel is %d\n", *pu8Channel );

    return ZbStatus;
}

static teZbStatus eZigbeeDeviceAddGroup(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceAddGroup\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    teZbStatus  ZbStatus = eZCB_AddGroupMembership(psZigbeeNode, u16GroupAddress);
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceRemoveGroup(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceAddGroup\n");
    teZbStatus ZbStatus = E_ZB_ERROR;
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);

    ZbStatus = eZCB_RemoveGroupMembership(psZigbeeNode, u16GroupAddress);
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceClearGroup(tsZigbeeBase *psZigbeeNode)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceAddGroup\n");
    teZbStatus ZbStatus = E_ZB_ERROR;
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    ZbStatus = eZCB_ClearGroupMembership(psZigbeeNode);
    //ZbStatus = eZCB_AddGroupMembership(psZigbeeNode, 0xf00f);
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceAddSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceAddGroup\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;

    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_StoreScene(psZigbeeNode, u16GroupAddress, u16SenceId & 0xFF);
    }
    else
    {
        ZbStatus = eZCB_StoreScene(psZigbeeNode, 0xf00f, u16SenceId & 0xFF); // default group
    }
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceRemoveSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceAddGroup\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;

    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_RemoveScene(psZigbeeNode, u16GroupAddress, u16SenceId & 0xFF);
    }
    else
    {
        ZbStatus = eZCB_RemoveScene(psZigbeeNode, 0xf00f, u16SenceId & 0xFFFF);
    }
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceCallSence(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceId)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceAddGroup\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;

    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_RecallScene(psZigbeeNode, u16GroupAddress, u16SenceId & 0xFF);
    }
    else
    {
        ZbStatus = eZCB_RecallScene(psZigbeeNode, 0xf00f, u16SenceId & 0xFF);
    }
    
    return ZbStatus;
}

 
static teZbStatus eZigbeeDeviceGetSence(tsZigbeeBase *psZigbeeNode, uint16 *u16SenceId)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceAddGroup\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    CHECK_POINTER(u16SenceId, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;
    uint8     u8CurrentScene;

    if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_SCENES, 
                        0, 0, 0, E_ZB_ATTRIBUTEID_SCENE_CURRENTSCENE, &u8CurrentScene)) != E_ZB_OK)
    {
        ZbStatus = E_ZB_TIMEOUT;
    }
    *u16SenceId = (uint16)u8CurrentScene;
    
    return ZbStatus;
}


static teZbStatus eZigbeeDeviceSetOnOff(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode)
{
    DBG_vPrintln(DBG_DEVICES, "eZigbeeDeviceSetOnOff\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);

    teZbStatus ZbStatus;  
    if(0 != u16GroupAddress){
        ZbStatus = eZCB_ZLL_OnOff(NULL, u16GroupAddress, u8Mode);
    } else {
        ZbStatus = eZCB_ZLL_OnOff(psZigbeeNode, 0, u8Mode);
    }
    
    return ZbStatus;
}

/** this func is not lock, must be called by one thread in the same time */
static teZbStatus eZigbeeDeviceGetOnOff(tsZigbeeBase *psZigbeeNode, uint8 *u8Mode)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceGetOnOff\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    CHECK_POINTER(u8Mode, E_ZB_ERROR);

    teZbStatus ZbStatus = E_ZB_OK;
    if (eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_ONOFF, 0, 0, 0, E_ZB_ATTRIBUTEID_ONOFF_ONOFF, u8Mode) != E_ZB_OK)
    {
        ERR_vPrintln(T_TRUE, "eZCB_ReadAttributeRequest OnOff Time Out\n");
        ZbStatus = E_ZB_TIMEOUT;
    }
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceSetLevel(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Level, uint16 u16TransitionTime)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceSetLevel\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;

    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_ZLL_MoveToLevel(NULL, u16GroupAddress, 1, u8Level, u16TransitionTime);
    }
    else
    {
        ZbStatus = eZCB_ZLL_MoveToLevel(psZigbeeNode, 0, 1, u8Level, u16TransitionTime);
    }

    return ZbStatus;
}

static teZbStatus eZigbeeDeviceGetLevel(tsZigbeeBase *psZigbeeNode, uint8 *u8Level)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceGetLevel\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    CHECK_POINTER(u8Level, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;

    if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_LEVEL_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL, u8Level)) != E_ZB_OK)
    {
        ZbStatus = E_ZB_ERROR;
    }
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceGetLevel:%d\n", *u8Level);
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceSetLightColour(tsZigbeeBase *psZigbeeNode, uint16 u16GroupAddress, tsRGB sRGB, uint16 u16TransitionTime)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceSetLightColor\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;
    
    tsHSV HSV = RGB_HSV(sRGB.R,sRGB.G,sRGB.B);
    uint8 u8CurrentHue = HSV.H * 254 / 360;
    uint8 u8CurrentSaturation = HSV.S * 254;
    DBG_vPrintln(DBG_DEVICES, "HSV[%f-%f-%f]\n", HSV.H, HSV.S, HSV.V);
    if(0 != u16GroupAddress)
    {
        ZbStatus = eZCB_ZLL_MoveToHueSaturation(NULL, u16GroupAddress, u8CurrentHue, u8CurrentSaturation, u16TransitionTime);
    }
    else
    {
        ZbStatus = eZCB_ZLL_MoveToHueSaturation(psZigbeeNode, 1, u8CurrentHue, u8CurrentSaturation, u16TransitionTime);
    }

    return ZbStatus;
}

static teZbStatus eZigbeeDeviceGetLightColour(tsZigbeeBase *psZigbeeNode, tsRGB *psRGB)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceGetLightColour\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    CHECK_POINTER(psRGB, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;

    uint8  u8CurrentHue        = 0;
    uint8  u8CurrentSaturation = 0;
    if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTHUE, &u8CurrentHue)) != E_ZB_OK)
    {
        return E_ZB_TIMEOUT;
    }
    
    DBG_vPrintln(DBG_DEVICES, "Current Hue attribute read as: %d\n", u8CurrentHue);
    

    if ((eZCB_ReadAttributeRequest(psZigbeeNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTSAT, &u8CurrentSaturation)) != E_ZB_OK)
    {
        return E_ZB_TIMEOUT;
    }
    DBG_vPrintln(DBG_DEVICES, "Current Saturation attribute read as: 0x%02X\n", u8CurrentSaturation);

    tsHSV sHSV;
    sHSV.H = (float)u8CurrentHue * 360 / 254;
    sHSV.S = (float)u8CurrentSaturation / 254;
    sHSV.V = 1.0;
    *psRGB = HSV_RGB(sHSV.H, sHSV.S, sHSV.V);
    
    return ZbStatus;
}

static teZbStatus eZigbeeDeviceRemoveNetwork(tsZigbeeBase *psZigbeeNode, uint8 u8Rejoin, uint8 u8RemoveChildren)
{
    DBG_vPrintln(DBG_DEVICES, "ZigbeeDeviceRemoveNetwork\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    teZbStatus ZbStatus = E_ZB_OK;
    
    if (eZCB_ManagementLeaveRequest(psZigbeeNode, u8Rejoin, u8RemoveChildren) != E_ZB_OK)
    {
        ERR_vPrintln(T_TRUE, "eZCB_ManagementLeaveRequest Failed\n");
        ZbStatus = E_ZB_TIMEOUT;
    }
    
    return ZbStatus;
}

/**
** Closures
*/
static teZbStatus eZigbeeDeviceSetClosuresState(tsZigbeeBase *psZigbeeNode, teCLD_WindowCovering_CommandID eCommand)
{
    DBG_vPrintln(DBG_DEVICES, "eZigbeeDeviceSetWindowCoveringDevice\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);

    teZbStatus eZbStatus;  
    eZbStatus = eZCB_WindowCoveringDeviceOperator(psZigbeeNode, eCommand);
    
    return eZbStatus;
}

static teZbStatus eZigbeeDeviceSetDoorLockState(tsZigbeeBase *psZigbeeNode, teCLD_DoorLock_CommandID eCommand)
{
    DBG_vPrintln(DBG_DEVICES, "eZigbeeDeviceSetDoorLockState\n");
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);

    teZbStatus eZbStatus;
    eZbStatus = eZCB_DoorLockDeviceOperator(psZigbeeNode, eCommand);

    return eZbStatus;
}

static teZbStatus eZigbeeDeviceGetSensorValue(tsZigbeeBase *psZigbeeNode, uint16 *u16SensorValue, teZigbee_ClusterID eClusterId)
{
    DBG_vPrintln(DBG_DEVICES, "eZigbeeDeviceGetSensorValue\n");
    if(psZigbeeNode->u8MacCapability & E_ZB_MAC_CAPABILITY_FFD){

    } else {
        #define PUT_VALUE(clusterid,value) case(clusterid):{*u16SensorValue=value;}break
        switch(eClusterId){
            PUT_VALUE(E_ZB_CLUSTERID_POWER, psZigbeeNode->sAttributeValue.u16Battery);
            PUT_VALUE(E_ZB_CLUSTERID_BINARY_INPUT_BASIC, psZigbeeNode->sAttributeValue.u8Binary);
            PUT_VALUE(E_ZB_CLUSTERID_TEMPERATURE, psZigbeeNode->sAttributeValue.u16Temp);
            PUT_VALUE(E_ZB_CLUSTERID_HUMIDITY, psZigbeeNode->sAttributeValue.u16Humi);
            PUT_VALUE(E_ZB_CLUSTERID_ILLUMINANCE, psZigbeeNode->sAttributeValue.u16Illum);
            default:break;
        }
    }
    
    return E_ZB_OK;
}

