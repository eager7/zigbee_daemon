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
// TODO : the definitions below are really confusing
// ZCb End points
#define CZD_ENDPOINT_ZHA             1   // Zigbee-HA
#define CZD_ENDPOINT_ONOFF           1   // ON/OFF cluster
#define CZD_ENDPOINT_GROUP           1   // GROUP cluster
#define CZD_ENDPOINT_SCENE           1   // SCENE cluster
#define CZD_ENDPOINT_SIMPLE          1   // SimpleDescriptor cluster
#define CZD_ENDPOINT_LAMP            1   // ON/OFF, Color control
#define CZD_ENDPOINT_TUNNEL          1   // For tunnel mesages
#define CZD_ENDPOINT_ATTR            1   // For attrs

#define CZD_NW_STATUS_SUCCESS                   0x00 // A request has been executed successfully.
#define CZD_NW_STATUS_INVALID_PARAMETER         0xc1 // An invalid or out-of-range parameter has been passed to a primitive from the next higher layer.
#define CZD_NW_STATUS_INVALID_REQUEST           0xc2 // The next higher layer has issued a request that is invalid or cannot be executed given the current state of the NWK lay-er.
#define CZD_NW_STATUS_NOT_PERMITTED             0xc3 // An NLME-JOIN.request has been disallowed.
#define CZD_NW_STATUS_STARTUP_FAILURE           0xc4 // An NLME-NETWORK-FORMATION.request has failed to start a network.
#define CZD_NW_STATUS_ALREADY_PRESENT           0xc5 // A device with the address supplied to the NLME-DIRECT-JOIN.request is already present in the neighbor table of the device on which the NLME-DIRECT-JOIN.request was issued.
#define CZD_NW_STATUS_SYNC_FAILURE              0xc6 // Used to indicate that an NLME-SYNC.request has failed at the MAC layer.
#define CZD_NW_STATUS_NEIGHBOR_TABLE_FULL       0xc7 // An NLME-JOIN-DIRECTLY.request has failed because there is no more room in the neighbor table.
#define CZD_NW_STATUS_UNKNOWN_DEVICE            0xc8 // An NLME-LEAVE.request has failed because the device addressed in the parameter list is not in the neighbor table of the issuing device.
#define CZD_NW_STATUS_UNSUPPORTED_ATTRIBUTE     0xc9 // An NLME-GET.request or NLME-SET.request has been issued with an unknown attribute identifier.
#define CZD_NW_STATUS_NO_NETWORKS               0xca // An NLME-JOIN.request has been issued in an environment where no networks are detectable.
#define CZD_NW_STATUS_MAX_FRM_COUNTER           0xcc // Security processing has been attempted on an outgoing frame, and has failed because the frame counter has reached its maximum value.
#define CZD_NW_STATUS_NO_KEY                    0xcd // Security processing has been attempted on an outgoing frame, and has failed because no key was available with which to process it.
#define CZD_NW_STATUS_BAD_CCM_OUTPUT            0xce // Security processing has been attempted on an outgoing frame, and has failed because the security engine produced erroneous output.
#define CZD_NW_STATUS_ROUTE_DISCOVERY_FAILED    0xd0 // An attempt to discover a route has failed due to a reason other than a lack of routing capacity.
#define CZD_NW_STATUS_ROUTE_ERROR               0xd1 // An NLDE-DATA.request has failed due to a routing failure on the sending device or an NLME-ROUTE-DISCOVERY.request has failed due to the cause cited in the accompanying NetworkStatusCode.
#define CZD_NW_STATUS_BT_TABLE_FULL             0xd2 // An attempt to send a broadcast frame or member mode mul-ticast has failed due to the fact that there is no room in the BTT.
#define CZD_NW_STATUS_FRAME_NOT_BUFFERED        0xd3 // An NLDE-DATA.request has failed due to insufficient buffering available.  A non-member mode multicast frame was discarded pending route discovery.
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

