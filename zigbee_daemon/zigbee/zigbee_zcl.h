/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP daemon
 *
 * COMPONENT:          Interface to Zigbee control bridge
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

#ifndef  ZIGBEE_ZCL_H_INCLUDED
#define  ZIGBEE_ZCL_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define ZB_DEFAULT_ENDPOINT_HA             1
#define ZB_DEFAULT_ENDPOINT_ZLL            1

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
    /* Zigbee ZCL status codes */
            E_ZB_OK                            = 0x00,
    E_ZB_ERROR                         = 0x01,

    /* ZCB internal status codes */
            E_ZB_ERROR_NO_MEM                  = 0x10,
    E_ZB_COMMS_FAILED                  = 0x11,
    E_ZB_UNKNOWN_NODE                  = 0x12,
    E_ZB_UNKNOWN_ENDPOINT              = 0x13,
    E_ZB_UNKNOWN_CLUSTER               = 0x14,
    E_ZB_REQUEST_NOT_ACTIONED          = 0x15,

    /* Zigbee ZCL status codes */
            E_ZB_NOT_AUTHORISED                = 0x7E,
    E_ZB_RESERVED_FIELD_NZERO          = 0x7F,
    E_ZB_MALFORMED_COMMAND             = 0x80,
    E_ZB_UNSUP_CLUSTER_COMMAND         = 0x81,
    E_ZB_UNSUP_GENERAL_COMMAND         = 0x82,
    E_ZB_UNSUP_MANUF_CLUSTER_COMMAND   = 0x83,
    E_ZB_UNSUP_MANUF_GENERAL_COMMAND   = 0x84,
    E_ZB_INVALID_FIELD                 = 0x85,
    E_ZB_UNSUP_ATTRIBUTE               = 0x86,
    E_ZB_INVALID_VALUE                 = 0x87,
    E_ZB_READ_ONLY                     = 0x88,
    E_ZB_INSUFFICIENT_SPACE            = 0x89,
    E_ZB_DUPLICATE_EXISTS              = 0x8A,
    E_ZB_NOT_FOUND                     = 0x8B,
    E_ZB_UNREPORTABLE_ATTRIBUTE        = 0x8C,
    E_ZB_INVALID_DATA_TYPE             = 0x8D,
    E_ZB_INVALID_SELECTOR              = 0x8E,
    E_ZB_WRITE_ONLY                    = 0x8F,
    E_ZB_INCONSISTENT_STARTUP_STATE    = 0x90,
    E_ZB_DEFINED_OUT_OF_BAND           = 0x91,
    E_ZB_INCONSISTENT                  = 0x92,
    E_ZB_ACTION_DENIED                 = 0x93,
    E_ZB_TIMEOUT                       = 0x94,

    E_ZB_HARDWARE_FAILURE              = 0xC0,
    E_ZB_SOFTWARE_FAILURE              = 0xC1,
    E_ZB_CALIBRATION_ERROR             = 0xC2,
} teZbStatus;

/** Enumerated type of coordinator start modes */
typedef enum
{
    E_START_COORDINATOR     = 0,        /**< Start module as a coordinator */
    E_START_ROUTER          = 1,        /**< Start module as a router */
    E_START_TOUCHLINK       = 2,        /**< Start module as a touchlink router */
} teStartMode;

/** Enumerated type of module modes */
typedef enum
{
    E_MODE_COORDINATOR      = 0,        /**< Start module as a coordinator */
    E_MODE_ROUTER           = 1,        /**< Start module as a router */
    E_MODE_HA_COMPATABILITY = 2,        /**< Start module as router in HA compatability mode */
} teModuleMode;

/** Enumerated type of ZigBee address modes */
typedef enum
{
    E_ZB_ADDRESS_MODE_BOUND                 = 0x00,
    E_ZB_ADDRESS_MODE_GROUP                 = 0x01,
    E_ZB_ADDRESS_MODE_SHORT                 = 0x02,
    E_ZB_ADDRESS_MODE_IEEE                  = 0x03,
    E_ZB_ADDRESS_MODE_BROADCAST             = 0x04,
    E_ZB_ADDRESS_MODE_NO_TRANSMIT           = 0x05,
    E_ZB_ADDRESS_MODE_BOUND_NO_ACK          = 0x06,
    E_ZB_ADDRESS_MODE_SHORT_NO_ACK          = 0x07,
    E_ZB_ADDRESS_MODE_IEEE_NO_ACK           = 0x08,
} teZigbee_AddressMode;

/** Enumerated type of Zigbee MAC Capabilities */
typedef enum
{
    E_ZB_MAC_CAPABILITY_ALT_PAN_COORD       = 0x01,
    E_ZB_MAC_CAPABILITY_FFD                 = 0x02,
    E_ZB_MAC_CAPABILITY_POWERED             = 0x04,
    E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE      = 0x08,
    E_ZB_MAC_CAPABILITY_SECURED             = 0x40,
    E_ZB_MAC_CAPABILITY_ALLOCATE_ADDRESS    = 0x80,
} teZigbee_MACCapability;

/** Enumerated type of Zigbee broadcast addresses */
typedef enum
{
    E_ZB_BROADCAST_ADDRESS_ALL              = 0xFFFF,
    E_ZB_BROADCAST_ADDRESS_RXONWHENIDLE     = 0xFFFD,
    E_ZB_BROADCAST_ADDRESS_ROUTERS          = 0xFFFC,
    E_ZB_BROADCAST_ADDRESS_LOWPOWERROUTERS  = 0xFFFB,
} teZigbee_BroadcastAddress;


/** Enumerated type of Zigbee Device IDs */
typedef enum
{
    /* Coordinator */
    E_ZBD_COORDINATOR                       = 0x0840,
    /* Generic */
    E_ZBD_ON_OFF_SWITCH                     = 0x0000,
    E_ZBD_LEVEL_CONTROL_SWITCH              = 0x0001,
    E_ZBD_ON_OFF_OUTPUT                     = 0x0002,
    E_ZBD_LEVEL_CONTROLLABLE_OUTPUT         = 0x0003,
    E_ZBD_SCENE_SELECTOR                    = 0x0004,
    E_ZBD_CONFIGURATION_TOOL                = 0x0005,
    E_ZBD_REMOTE_CONTROL                    = 0x0006,
    E_ZBD_COMBINED_INTERFACE                = 0x0007,
    E_ZBD_RANGE_EXTENDER                    = 0x0008,
    E_ZBD_MAIN_POWER_OUTLET                 = 0x0009,
    E_ZBD_DOOR_LOCK                         = 0x000A,
    E_ZBD_DOOR_LOCK_CONTROLLER              = 0x000B,
    E_ZBD_SIMPLE_SENSOR                     = 0x000C,
    E_ZBD_CONSUMPTION_AWARENESS_DEVICE      = 0x000D,
    E_ZBD_HOME_GATEWAY                      = 0x0050,
    E_ZBD_SMART_PLUG                        = 0x0051,
    E_ZBD_WHITE_GOODS                       = 0x0052,
    E_ZBD_METER_INTERFACE                   = 0x0052,
    /* Lighting */
    E_ZBD_ON_OFF_LIGHT                      = 0x0100,
    E_ZBD_DIMMER_LIGHT                      = 0x0101,
    E_ZBD_COLOUR_DIMMER_LIGHT               = 0x0102,
    E_ZBD_ON_OFF_LIGHT_SWITCH               = 0x0103,
    E_ZBD_DIMMER_SWITCH                     = 0x0104,
    E_ZBD_COLOUR_DIMMER_SWITCH              = 0x0105,
    E_ZBD_LIGHT_SENSOR                      = 0x0106,
    E_ZBD_OCCUPANCY_SENSOR                  = 0x0107,
    /* Closures */
    E_ZBD_SHADE                             = 0x0200,
    E_ZBD_SHADE_CONTROLLER                  = 0x0201,
    E_ZBD_WINDOW_COVERING_DEVICE            = 0x0202,
    E_ZBD_WINDOW_COBERING_CONTROLLER        = 0x0203,
    /* HAVC */
    E_ZBD_HEATING_COOLING_UNIT              = 0x0300,
    E_ZBD_THERMOSTAT                        = 0x0301,
    E_ZBD_TEMPERATURE_SENSOR                = 0x0302,
    E_ZBD_PUMP                              = 0x0303,
    E_ZBD_PUMP_CONTROLLER                   = 0x0304,
    E_ZBD_PRESSURE_SENSOR                   = 0x0305,
    E_ZBD_FLOW_SENSOR                       = 0x0306,
    E_ZBD_MINI_SPLIT_AC                     = 0x0307,

    /* Sensor */
    E_ZBD_END_DEVICE_DEVICE                 = 0x0841,
} teZigbeeDeviceID;


/** Enumerated type of Zigbee Profile IDs */
typedef enum
{
    E_ZB_PROFILEID_HA                       = 0x0104,
    E_ZB_PROFILEID_ZLL                      = 0xC05E,    
} teZigbee_ProfileID;


/** Enumerated type of Zigbee Cluster IDs */
typedef enum
{
    /* Generic */
    E_ZB_CLUSTERID_BASIC                    = 0x0000,
    E_ZB_CLUSTERID_POWER                    = 0x0001,
    E_ZB_CLUSTERID_DEVICE_TEMPERATURE       = 0x0002,
    E_ZB_CLUSTERID_IDENTIFY                 = 0x0003,
    E_ZB_CLUSTERID_GROUPS                   = 0x0004,
    E_ZB_CLUSTERID_SCENES                   = 0x0005,
    E_ZB_CLUSTERID_ONOFF                    = 0x0006,
    E_ZB_CLUSTERID_ONOFF_CONFIGURATION      = 0x0007,
    E_ZB_CLUSTERID_LEVEL_CONTROL            = 0x0008,
    E_ZB_CLUSTERID_ALARMS                   = 0x0009,
    E_ZB_CLUSTERID_TIME                     = 0x000A,
    E_ZB_CLUSTERID_RSSI_LOCATION            = 0x000B,
    E_ZB_CLUSTERID_ANALOG_INPUT_BASIC       = 0x000C,
    E_ZB_CLUSTERID_ANALOG_OUTPUT_BASIC      = 0x000D,
    E_ZB_CLUSTERID_VALUE_BASIC              = 0x000E,
    E_ZB_CLUSTERID_BINARY_INPUT_BASIC       = 0x000F,
    E_ZB_CLUSTERID_BINARY_OUTPUT_BASIC      = 0x0010,
    E_ZB_CLUSTERID_BINARY_VALUE_BASIC       = 0x0011,
    E_ZB_CLUSTERID_MULTISTATE_INPUT_BASIC   = 0x0012,
    E_ZB_CLUSTERID_MULTISTATE_OUTPUT_BASIC  = 0x0013,
    E_ZB_CLUSTERID_MULTISTATE_VALUE_BASIC   = 0x0014,
    E_ZB_CLUSTERID_COMMISSIONING            = 0x0015,
    /* Closures */
    E_ZB_CLUSTERID_SHADE_CONFIGURATION      = 0x0100,
    E_ZB_CLUSTERID_DOOR_LOCK                = 0x0101,
    E_ZB_CLUSTERID_WINDOW_COVERING_DEVICE   = 0x0102,
    
    /* HVAC */
    E_ZB_CLUSTERID_THERMOSTAT               = 0x0201,
    
    /* Lighting */
    E_ZB_CLUSTERID_COLOR_CONTROL            = 0x0300,
    E_ZB_CLUSTERID_BALLAST_CONFIGURATION    = 0x0301,
    
    /* Sensing */
    E_ZB_CLUSTERID_TEMPERATURE              = 0x0402,
    E_ZB_CLUSTERID_HUMIDITY                 = 0x0405,
    E_ZB_CLUSTERID_ILLUMINANCE              = 0x0400,
    E_ZB_CLUSTER_ID_POWER_CONFIGURATION     = 0x0001,
    
    /* ZLL */
    E_ZB_CLUSTERID_ZLL_COMMISIONING         = 0x1000,
} teZigbee_ClusterID;

/** Enumerated type of attributes in the Basic Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_BASIC_ZCL_VERSION      = 0x0000,
    E_ZB_ATTRIBUTEID_BASIC_APP_VERSION      = 0x0001,
    E_ZB_ATTRIBUTEID_BASIC_STACK_VERSION    = 0x0002,
    E_ZB_ATTRIBUTEID_BASIC_HW_VERSION       = 0x0003,
    E_ZB_ATTRIBUTEID_BASIC_MAN_NAME         = 0x0004,
    E_ZB_ATTRIBUTEID_BASIC_MODEL_ID         = 0x0005,
    E_ZB_ATTRIBUTEID_BASIC_DATE_CODE        = 0x0006,
    E_ZB_ATTRIBUTEID_BASIC_POWER_SOURCE     = 0x0007,
    
    E_ZB_ATTRIBUTEID_BASIC_LOCATION_DESC    = 0x0010,
    E_ZB_ATTRIBUTEID_BASIC_PHYSICAL_ENV     = 0x0011,
    E_ZB_ATTRIBUTEID_BASIC_DEVICE_ENABLED   = 0x0012,
    E_ZB_ATTRIBUTEID_BASIC_ALARM_MASK       = 0x0013,
    E_ZB_ATTRIBUTEID_BASIC_DISBALELOCALCONF = 0x0014,
} teZigbee_AttributeIDBasicCluster;


/** Enumerated type of attributes in the Scenes Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_SCENE_SCENECOUNT       = 0x0000,
    E_ZB_ATTRIBUTEID_SCENE_CURRENTSCENE     = 0x0001,
    E_ZB_ATTRIBUTEID_SCENE_CURRENTGROUP     = 0x0002,
    E_ZB_ATTRIBUTEID_SCENE_SCENEVALID       = 0x0003,
    E_ZB_ATTRIBUTEID_SCENE_NAMESUPPORT      = 0x0004,
    E_ZB_ATTRIBUTEID_SCENE_LASTCONFIGUREDBY = 0x0005,
} teZigbee_AttributeIDScenesCluster;


/** Enumerated type of attributes in the ON/Off Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_ONOFF_ONOFF            = 0x0000,
    E_ZB_ATTRIBUTEID_ONOFF_GLOBALSCENE      = 0x4000,
    E_ZB_ATTRIBUTEID_ONOFF_ONTIME           = 0x4001,
    E_ZB_ATTRIBUTEID_ONOFF_OFFWAITTIME      = 0x4002,
} teZigbee_AttributeIDOnOffCluster;


/** Enumerated type of attributes in the Level Control Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL     = 0x0000,
    E_ZB_ATTRIBUTEID_LEVEL_REMAININGTIME    = 0x0001,
    E_ZB_ATTRIBUTEID_LEVEL_ONOFFTRANSITION  = 0x0010,
    E_ZB_ATTRIBUTEID_LEVEL_ONLEVEL          = 0x0011,
} teZigbee_AttributeIDLevelControlCluster;


/** Enumerated type of attributes in the Level Control Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTHUE      = 0x0000,
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTSAT      = 0x0001,
    E_ZB_ATTRIBUTEID_COLOUR_REMAININGTIME   = 0x0002,
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTX        = 0x0003,
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTY        = 0x0004,
    E_ZB_ATTRIBUTEID_COLOUR_DRIFTCOMPENSATION   = 0x0005,
    E_ZB_ATTRIBUTEID_COLOUR_COMPENSATIONTEXT    = 0x0006,
    E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMPERATURE   = 0x0007,
    E_ZB_ATTRIBUTEID_COLOUR_COLOURMODE      = 0x0008,
    
    
    E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMP_PHYMIN   = 0x400b,
    E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMP_PHYMAX   = 0x400c,
    
} teZigbee_AttributeIDColourControlCluster;

/** Enumerated type of attributes in the Thermostat Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_TSTAT_LOCALTEMPERATURE         = 0x0000,
    E_ZB_ATTRIBUTEID_TSTAT_OUTDOORTEMPERATURE       = 0x0001,
    E_ZB_ATTRIBUTEID_TSTAT_OCCUPANCY                = 0x0002,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMINHEATSETPOINTLIMIT  = 0x0003,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMAXHEATSETPOINTLIMIT  = 0x0004,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMINCOOLSETPOINTLIMIT  = 0x0005,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMAXCOOLSETPOINTLIMIT  = 0x0006,
    E_ZB_ATTRIBUTEID_TSTAT_PICOOLINGDEMAND          = 0x0007,
    E_ZB_ATTRIBUTEID_TSTAT_PIHEATINGDEMAND          = 0x0008,
    
    E_ZB_ATTRIBUTEID_TSTAT_LOCALTEMPERATURECALIB    = 0x0010,
    E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDCOOLSETPOINT     = 0x0011,
    E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDHEATSETPOINT     = 0x0012,
    E_ZB_ATTRIBUTEID_TSTAT_UNOCCUPIEDCOOLSETPOINT   = 0x0013,
    E_ZB_ATTRIBUTEID_TSTAT_UNOCCUPIEDHEATSETPOINT   = 0x0014,
    E_ZB_ATTRIBUTEID_TSTAT_MINHEATSETPOINTLIMIT     = 0x0015,
    E_ZB_ATTRIBUTEID_TSTAT_MAXHEATSETPOINTLIMIT     = 0x0016,
    E_ZB_ATTRIBUTEID_TSTAT_MINCOOLSETPOINTLIMIT     = 0x0017,
    E_ZB_ATTRIBUTEID_TSTAT_MAXCOOLSETPOINTLIMIT     = 0x0018,
    E_ZB_ATTRIBUTEID_TSTAT_MINSETPOINTDEADBAND      = 0x0019,
    E_ZB_ATTRIBUTEID_TSTAT_REMOTESENSING            = 0x001A,
    E_ZB_ATTRIBUTEID_TSTAT_COLTROLSEQUENCEOFOPERATION = 0x001B,
    E_ZB_ATTRIBUTEID_TSTAT_SYSTEMMODE               = 0x001C,
    E_ZB_ATTRIBUTEID_TSTAT_ALARMMASK                = 0x001D,
} teZigbee_AttributeIDThermostatCluster;


/** Enumerated type of attributes in the Temperature sensing Cluster */
typedef enum
{
     E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED         = 0x0000,
     E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED_MIN     = 0x0001,
     E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED_MAX     = 0x0002,
     E_ZB_ATTRIBUTEID_TEMPERATURE_TOLERANCE        = 0x0003,
} teZigbee_AttributeIDTemperatureCluster;

//PCT_CODE_SENSOR
/** Enumerated type of attributes in the Power sensing Cluster */
typedef enum
{
    /* Mains Information attribute set attribute ID's (3.3.2.2.1) */
    E_CLD_PWRCFG_ATTR_ID_MAINS_VOLTAGE                = 0x0000,
    E_CLD_PWRCFG_ATTR_ID_MAINS_FREQUENCY,

    /* Mains settings attribute set attribute ID's (3.3.2.2.2) */
    E_CLD_PWRCFG_ATTR_ID_MAINS_ALARM_MASK             = 0x0010,
    E_CLD_PWRCFG_ATTR_ID_MAINS_VOLTAGE_MIN_THRESHOLD,
    E_CLD_PWRCFG_ATTR_ID_MAINS_VOLTAGE_MAX_THRESHOLD,
    E_CLD_PWRCFG_ATTR_ID_MAINS_VOLTAGE_DWELL_TRIP_POINT,

    /* Battery information attribute set attribute ID's (3.3.2.2.3) */
    E_CLD_PWRCFG_ATTR_ID_BATTERY_VOLTAGE              = 0x0020,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_PERCENTAGE_REMAINING,

    /* Battery settings attribute set attribute ID's (3.3.2.2.4) */
    E_CLD_PWRCFG_ATTR_ID_BATTERY_MANUFACTURER         = 0x0030,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_SIZE,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_AHR_RATING,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_QUANTITY,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_RATED_VOLTAGE,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_ALARM_MASK,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_VOLTAGE_MIN_THRESHOLD,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_VOLTAGE_THRESHOLD1,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_VOLTAGE_THRESHOLD2,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_VOLTAGE_THRESHOLD3,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_PERCENTAGE_MIN_THRESHOLD,    
    E_CLD_PWRCFG_ATTR_ID_BATTERY_PERCENTAGE_THRESHOLD1,  
    E_CLD_PWRCFG_ATTR_ID_BATTERY_PERCENTAGE_THRESHOLD2, 
    E_CLD_PWRCFG_ATTR_ID_BATTERY_PERCENTAGE_THRESHOLD3, 
    E_CLD_PWRCFG_ATTR_ID_BATTERY_ALARM_STATE, 
    
    /* Battery information 2 attribute set attribute ID's (3.3.2.2.3) */
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_VOLTAGE              = 0x0040,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_PERCENTAGE_REMAINING,

    /* Battery settings 2 attribute set attribute ID's (3.3.2.2.4) */
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_MANUFACTURER         = 0x0050,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_SIZE,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_AHR_RATING,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_QUANTITY,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_RATED_VOLTAGE,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_ALARM_MASK,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_VOLTAGE_MIN_THRESHOLD,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_VOLTAGE_THRESHOLD1,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_VOLTAGE_THRESHOLD2,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_VOLTAGE_THRESHOLD3,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_PERCENTAGE_MIN_THRESHOLD,    
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_PERCENTAGE_THRESHOLD1,  
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_PERCENTAGE_THRESHOLD2, 
    E_CLD_PWRCFG_ATTR_ID_BATTERY_2_PERCENTAGE_THRESHOLD3, 

    /* Battery information 3 attribute set attribute ID's (3.3.2.2.3) */
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_VOLTAGE              = 0x0060,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_PERCENTAGE_REMAINING,

    /* Battery settings 3 attribute set attribute ID's (3.3.2.2.4) */
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_MANUFACTURER         = 0x0070,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_SIZE,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_AHR_RATING,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_QUANTITY,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_RATED_VOLTAGE,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_ALARM_MASK,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_VOLTAGE_MIN_THRESHOLD,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_VOLTAGE_THRESHOLD1,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_VOLTAGE_THRESHOLD2,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_VOLTAGE_THRESHOLD3,
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_PERCENTAGE_MIN_THRESHOLD,    
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_PERCENTAGE_THRESHOLD1,  
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_PERCENTAGE_THRESHOLD2, 
    E_CLD_PWRCFG_ATTR_ID_BATTERY_3_PERCENTAGE_THRESHOLD3     

} teZigbee_AttributeIDPowerConfigureCluster;
typedef enum
{
    /* Relative Humidity Measurement Information attribute set attribute ID's (4.7.2.2.1) */
    E_CLD_RHMEAS_ATTR_ID_MEASURED_VALUE          = 0x0000,  /* Mandatory */
    E_CLD_RHMEAS_ATTR_ID_MIN_MEASURED_VALUE,                /* Mandatory */
    E_CLD_RHMEAS_ATTR_ID_MAX_MEASURED_VALUE,                /* Mandatory */
    E_CLD_RHMEAS_ATTR_ID_TOLERANCE,
} teZigbee_AttributeIDHumidityCluster;

typedef enum
{
    /* Binary Input (Basic) Device Information attribute set attribute ID's (3.2.2.2.1) */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_ACTIVE_TEXT                    =                0x0004, /* Optional */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_DESCRIPTION                    =                0x001C, /*Optional */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_INACTIVE_TEXT                  =                0x002E, /*Optional */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_OUT_OF_SERVICE                 =                0x0051, /*Mandatory */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_POLARITY                       =                0x0054, /*Optional */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_PRESENT_VALUE                  =                0x0055, /* Mandatory */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_RELIABILITY                    =                0x0067, /*Optional */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_STATUS_FLAGS                   =                0x006F, /* Mandatory */
    E_CLD_BINARY_INPUT_BASIC_ATTR_ID_APPLICATION_TYPE               =                0x0100, /*Optional */
} teZigbee_AttributeIDBinaryCluster;

typedef enum
{
    /* Illuminance Level Sensing Information attribute set attribute ID's (4.2.2.2.1) */
    E_CLD_ILLMEAS_ATTR_ID_MEASURED_VALUE          = 0x0000,  /* Mandatory */
    E_CLD_ILLMEAS_ATTR_ID_MIN_MEASURED_VALUE,                /* Mandatory */
    E_CLD_ILLMEAS_ATTR_ID_MAX_MEASURED_VALUE,                /* Mandatory */
    E_CLD_ILLMEAS_ATTR_ID_TOLERANCE,
    E_CLD_ILLMEAS_ATTR_ID_LIGHT_SENSOR_TYPE
} teZigbee_AttributeIDIlluMinanceCluster;

//endif

/** Enumerated type of attribute data types from ZCL */
typedef enum PACK
{
    /* Null */
    E_ZCL_NULL            = 0x00,
 
    /* General Data */
    E_ZCL_GINT8           = 0x08,                // General 8 bit - not specified if signed or not
    E_ZCL_GINT16,
    E_ZCL_GINT24,
    E_ZCL_GINT32,
    E_ZCL_GINT40,
    E_ZCL_GINT48,
    E_ZCL_GINT56,
    E_ZCL_GINT64,
 
    /* Logical */
    E_ZCL_BOOL            = 0x10,
 
    /* Bitmap */
    E_ZCL_BMAP8            = 0x18,                // 8 bit bitmap
    E_ZCL_BMAP16,
    E_ZCL_BMAP24,
    E_ZCL_BMAP32,
    E_ZCL_BMAP40,
    E_ZCL_BMAP48,
    E_ZCL_BMAP56,
    E_ZCL_BMAP64,
 
    /* Unsigned Integer */
    E_ZCL_UINT8           = 0x20,                // Unsigned 8 bit
    E_ZCL_UINT16,
    E_ZCL_UINT24,
    E_ZCL_UINT32,
    E_ZCL_UINT40,
    E_ZCL_UINT48,
    E_ZCL_UINT56,
    E_ZCL_UINT64,
 
    /* Signed Integer */
    E_ZCL_INT8            = 0x28,                // Signed 8 bit
    E_ZCL_INT16,
    E_ZCL_INT24,
    E_ZCL_INT32,
    E_ZCL_INT40,
    E_ZCL_INT48,
    E_ZCL_INT56,
    E_ZCL_INT64,
 
    /* Enumeration */
    E_ZCL_ENUM8            = 0x30,                // 8 Bit enumeration
    E_ZCL_ENUM16,
 
    /* Floating Point */
    E_ZCL_FLOAT_SEMI    = 0x38,                // Semi precision
    E_ZCL_FLOAT_SINGLE,                        // Single precision
    E_ZCL_FLOAT_DOUBLE,                        // Double precision
 
    /* String */
    E_ZCL_OSTRING        = 0x41,                // Octet string
    E_ZCL_CSTRING,                            // Character string
    E_ZCL_LOSTRING,                            // Long octet string
    E_ZCL_LCSTRING,                            // Long character string
 
    /* Ordered Sequence */
    E_ZCL_ARRAY          = 0x48,
    E_ZCL_STRUCT         = 0x4c,
 
    E_ZCL_SET            = 0x50,
    E_ZCL_BAG            = 0x51,
 
    /* Time */
    E_ZCL_TOD            = 0xe0,                // Time of day
    E_ZCL_DATE,                                // Date
    E_ZCL_UTCT,                                // UTC Time
 
    /* Identifier */
    E_ZCL_CLUSTER_ID    = 0xe8,                // Cluster ID
    E_ZCL_ATTRIBUTE_ID,                        // Attribute ID
    E_ZCL_BACNET_OID,                        // BACnet OID
 
    /* Miscellaneous */
    E_ZCL_IEEE_ADDR        = 0xf0,                // 64 Bit IEEE Address
    E_ZCL_KEY_128,                            // 128 Bit security key, currently not supported as it would add to code space in u16ZCL_WriteTypeNBO and add extra 8 bytes to report config record for each reportable attribute
 
    /* NOTE:
     * 0xfe is a reserved value, however we are using it to denote a message signature.
     * This may have to change some time if ZigBee ever allocate this value to a data type
     */
    E_ZCL_SIGNATURE     = 0xfe,             // ECDSA Signature (42 bytes)
 
    /* Unknown */
    E_ZCL_UNKNOWN        = 0xff
 
} teZCL_ZCLAttributeType;

/* Window Covering Command - Payload */
typedef enum 
{
    E_CLD_WINDOW_COVERING_DEVICE_CMD_UP_OPEN                  = 0x00,         /* Mandatory */
    E_CLD_WINDOW_COVERING_DEVICE_CMD_DOWN_CLOSE               = 0x01,         /* Mandatory */
    E_CLD_WINDOW_COVERING_DEVICE_CMD_STOP                     = 0x02,         /* Mandatory */
    E_CLD_WINDOW_COVERING_DEVICE_CMD_GO_TO_LIFT_VALUE         = 0x04,         /* Option */
    E_CLD_WINDOW_COVERING_DEVICE_CMD_GO_TO_LIFT_PERCENTAGE    = 0x05,         /* Option */
    E_CLD_WINDOW_COVERING_DEVICE_CMD_GO_TO_TILE_VALUE         = 0x07,         /* Option */
    E_CLD_WINDOW_COVERING_DEVICE_CMD_GO_TO_TILE_PERCENTAGE    = 0x08,         /* Option */
} teCLD_WindowCovering_CommandID;

/* Door Lock Command - Payload */
#define DOOR_LOCK_PASSWORD_LEN 10
/* Lock State */
typedef enum
{
    E_CLD_DOOR_LOCK_LOCK_STATE_NOT_FULLY_LOCKED  = 0x00,
    E_CLD_DOOR_LOCK_LOCK_STATE_LOCK,
    E_CLD_DOOR_LOCK_LOCK_STATE_UNLOCK
} teCLD_DoorLock_LockState;
typedef enum
{
    /* Door Lock attribute set attribute ID's (A1) */
    E_CLD_DOOR_LOCK_ATTR_ID_LOCK_STATE                = 0x0000,             /* 0.Mandatory */
    E_CLD_DOOR_LOCK_ATTR_ID_LOCK_TYPE,                                      /* 1.Mandatory */
    E_CLD_DOOR_LOCK_ATTR_ID_ACTUATOR_ENABLED,                               /* 2.Mandatory */
    E_CLD_DOOR_LOCK_ATTR_ID_DOOR_STATE,                                     /* 3.Optional */
    E_CLD_DOOR_LOCK_ATTR_ID_NUMBER_OF_DOOR_OPEN_EVENTS,                     /* 4.Optional */
    E_CLD_DOOR_LOCK_ATTR_ID_NUMBER_OF_DOOR_CLOSED_EVENTS,                   /* 5.Optional */
    E_CLD_DOOR_LOCK_ATTR_ID_NUMBER_OF_MINUTES_DOOR_OPENED,                  /* 6.Optional */
    E_CLD_DOOR_LOCK_ATTR_ID_ZIGBEE_SECURITY_LEVEL     = 0x0034,
    E_CLD_DOOR_LOCK_ATTR_ID_NUMBER_OF_RFID_USERS_SUPPORTED = 0x0013,
    E_CLD_DOOR_LOCK_ATTR_ID_MAX_PIN_CODE_LENGTH = 0x0017,
    E_CLD_DOOR_LOCK_ATTR_ID_MIN_PIN_CODE_LENGTH = 0x0018,
} teCLD_DoorLock_Cluster_AttrID;

typedef enum
{
    E_CLD_DOOR_LOCK_DEVICE_CMD_LOCK                  = 0x00,         /* Mandatory */
    E_CLD_DOOR_LOCK_DEVICE_CMD_UNLOCK                = 0x01,         /* Mandatory */
    /** PCT Add */
    E_CLD_DOOR_LOCK_CMD_TOGGLE                      = 0x02,
    E_CLD_DOOR_LOCK_CMD_UNLOCK_WITH_TIMEOUT         = 0x03,
    E_CLD_DOOR_LOCK_CMD_GET_LOG_RECORD              = 0x04,
    E_CLD_DOOR_LOCK_CMD_SET_PIN_CODE                = 0x05,
    E_CLD_DOOR_LOCK_CMD_GET_PIN_CODE                = 0x06,
    E_CLD_DOOR_LOCK_CMD_CLEAR_PIN_CODE              = 0x07,
    E_CLD_DOOR_LOCK_CMD_CLEAR_ALL_PIN_CODES         = 0x08,
    E_CLD_DOOR_LOCK_CMD_SET_USER_STATUS             = 0x09,
    E_CLD_DOOR_LOCK_CMD_GET_USER_STATUS             = 0x0a,
    E_CLD_DOOR_LOCK_CMD_SET_USER_TYPE               = 0x14,
    E_CLD_DOOR_LOCK_CMD_GET_USER_TYPE               = 0x15,
    E_CLD_DOOR_LOCK_CMD_SET_RFID_CODE               = 0x16,
    E_CLD_DOOR_LOCK_CMD_GET_RFID_CODE               = 0x17,
    E_CLD_DOOR_LOCK_CMD_CLEAR_RFID_CODE             = 0x18,
    E_CLD_DOOR_LOCK_CMD_CLEAR_ALL_RFID_CODES        = 0x19,
} teCLD_DoorLock_CommandID;

typedef struct{
    unsigned char u8PasswordID;
    unsigned char u8AvailableNum;
    const char *psTime;
    unsigned char u8PasswordLen;
    const char *psPassword;
}tsCLD_DoorLock_Payload;
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

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
 
#if defined __cplusplus 
}  
#endif

#endif  /* ZIGBEECONSTANT_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

