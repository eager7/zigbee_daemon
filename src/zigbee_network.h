/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP daemon
 *
 * COMPONENT:          Interface to Zigbee control bridge
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


#ifndef  ZIGBEENETWORK_H_INCLUDED
#define  ZIGBEENETWORK_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "utils.h"
#include "list.h"
#include "threads.h"
#include "zigbee_constant.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** Enumerated type of statuses - This fits in with the Zigbee ZCL status codes */
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

typedef enum
{
    E_ZB_COORDINATOR                    = 0x0840, 
    E_ZB_ON_OFF_LAMP                    = 0x0100,  /* ZLL mono lamp / HA on/off lamp */
    E_ZB_DIMMER_LAMP                    = 0x0101,  /* HA dimmable lamp */
    E_ZB_COLOUR_LAMP                    = 0x0102,  /* HA dimmable colour lamp */
    E_ZB_DIMMER_COLOUR_LAMP             = 0x0200,  /* ZLL dimmable colour lamp */
    E_ZB_EXTEND_COLOUR_LAMP             = 0x0210,  /* ZLL extended colour lamp */
    E_ZB_TEMPER_COLOUR_LAMP             = 0x0220,  /* ZLL colour temperature lamp */
    E_ZB_WARM_COLD_LAMP                 = 0x0108,  /* PCT Cold & Warm Light*/
    E_ZB_TEMPERATURE_SENSOR             = 0x0302,  /* Temp Humidity sensor */
    E_ZB_LIGHT_SENSOR                   = 0x0106,  /* Light sensor */
    E_ZB_SIMPLE_SENSOR                  = 0x000c,  /* Simple sensor */
    E_ZB_SMART_PLUG                     = 0x0051,  /* Smart Plug */

}teZigbeeDeviceID;


/** Union type for all Zigbee attribute data types */
typedef union
{
    uint8     		u8Data;
    uint16    		u16Data;
    uint32      		u32Data;
    uint64     	u64Data;
} tuZcbAttributeData;


/** Structure representing a cluster on a node */
typedef struct _tsNodeCluster
{
    uint16            u16ClusterID;
    
    uint32            u32NumAttributes;
    uint16            *pau16Attributes;
    
    uint32            u32NumCommands;
    uint8             *pau8Commands;
} tsNodeCluster;


typedef struct
{
    uint8             u8Endpoint;
    uint16            u16ProfileID;
    
    uint32            u32NumClusters;
    tsNodeCluster   *pasClusters;
} tsNodeEndpoint;

/* Forward declarations of structures */
struct _tsZigbee_Node;

typedef teZbStatus (*tpreDeviceInitialise)(struct _tsZigbee_Node *psZigbeeNode);
typedef teZbStatus (*tpreDeviceSetAttribute)(struct _tsZigbee_Node *psZigbeeNode, eZigbee_ClusterID u16ClusterID);
typedef teZbStatus (*tpreDeviceGetAttribute)(struct _tsZigbee_Node *psZigbeeNode, eZigbee_ClusterID u16ClusterID);
typedef teZbStatus (*tpreDeviceAddGroup)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupID);
typedef teZbStatus (*tpreDeviceRemoveGroup)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupID);
typedef teZbStatus (*tpreDeviceClearGroup)(struct _tsZigbee_Node *psZigbeeNode);
typedef teZbStatus (*tpreDeviceAddSence)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SenceID);
typedef teZbStatus (*tpreDeviceRemoveSence)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SencepID);
typedef teZbStatus (*tpreDeviceSetSence)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16SencepID);
typedef teZbStatus (*tpreDeviceGetSence)(struct _tsZigbee_Node *psZigbeeNode, uint16 *u16SencepID);
//management
typedef teZbStatus (*tpreDeviceRemoveNetwork)(struct _tsZigbee_Node *psZigbeeNode);
typedef teZbStatus (*tpreDeviceAddBind)(struct _tsZigbee_Node *psSrcZigbeeNode, struct _tsZigbee_Node *psDesZigbeeNode, uint16 u16ClusterID);
typedef teZbStatus (*tpreDeviceRemoveBind)(struct _tsZigbee_Node *psSrcZigbeeNode, struct _tsZigbee_Node *psDesZigbeeNode, uint16 u16ClusterID);
//coor
typedef teZbStatus (*tpreCoordinatorReset)(struct _tsZigbee_Node *psZigbeeNode);
typedef teZbStatus (*tpreCoordinatorPermitJoin)(uint8 time);
//ZLL
typedef teZbStatus (*tpreDeviceSetOnOff)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
typedef teZbStatus (*tpreDeviceGetOnOff)(struct _tsZigbee_Node *psZigbeeNode, uint8 *u8Mode);
typedef teZbStatus (*tpreDeviceSetLightColour)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint32 u32HueSatTarget, uint16 u16TransitionTime);
typedef teZbStatus (*tpreDeviceGetLightColour)(struct _tsZigbee_Node *psZigbeeNode, uint32 *u32HueSatTarget);
typedef teZbStatus (*tpreDeviceSetLevel)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Level, uint16 u16TransitionTime);
typedef teZbStatus (*tpreDeviceGetLevel)(struct _tsZigbee_Node *psZigbeeNode, uint8 *u8Level);

//Sensor
typedef teZbStatus (*tpreDeviceGetTemperature)(struct _tsZigbee_Node *psZigbeeNode, uint16 *u16TempValue);
typedef teZbStatus (*tpreDeviceGetHumidity)(struct _tsZigbee_Node *psZigbeeNode, uint16 *u16HumiValue);
typedef teZbStatus (*tpreDeviceGetPower)(struct _tsZigbee_Node *psZigbeeNode, uint16 *u16PowerValue);
typedef teZbStatus (*tpreDeviceGetSimple)(struct _tsZigbee_Node *psZigbeeNode, uint16 *u16SimpleValue);
typedef teZbStatus (*tpreDeviceGetIlluminance)(struct _tsZigbee_Node *psZigbeeNode, uint16 *u16IlluValue);
typedef void (*tprAttributeUpdate)(struct _tsZigbee_Node *psZigbeeNode, uint16 u16ClusterID, uint16 u16AttributeID, teZCL_ZCLAttributeType eType, tuZcbAttributeData uData);

typedef struct _method
{
    //Normal
    tpreDeviceSetAttribute      DeviceSetAttribute;
    tpreDeviceGetAttribute      DeviceGetAttribute;
    tpreDeviceAddGroup          DeviceAddGroup;
    tpreDeviceRemoveGroup       DeviceRemoveGroup;
    tpreDeviceClearGroup        DeviceClearGroup;
    tpreDeviceAddSence          DeviceAddSence;
    tpreDeviceRemoveSence       DeviceRemoveSence;
    tpreDeviceSetSence          DeviceSetSence;
    tpreDeviceGetSence          DeviceGetSence;
    tpreDeviceRemoveNetwork     DeviceRemoveNetwork;
    tpreDeviceAddBind           DeviceAddBind;
    tpreDeviceRemoveBind        DeviceRemoveBind;
    //Coordinator
    tpreCoordinatorPermitJoin   CoordinatorPermitJoin;
    //ZLL
    tpreDeviceSetOnOff          DeviceSetOnOff;
    tpreDeviceGetOnOff          DeviceGetOnOff;
    tpreDeviceSetLightColour    DeviceSetLightColour;
    tpreDeviceGetLightColour    DeviceGetLightColour;
    tpreDeviceSetLevel          DeviceSetLevel;
    tpreDeviceGetLevel          DeviceGetLevel;
    //Sensor
    tpreDeviceGetTemperature    DeviceGetTemperature;
    tpreDeviceGetHumidity       DeviceGetHumidity;
    tpreDeviceGetPower          DeviceGetPower;
    tpreDeviceGetSimple         DeviceGetSimple;
    tpreDeviceGetIlluminance    DeviceGetIlluminance;
    tprAttributeUpdate          DeviceAttributeUpdate;
}t_Method;


typedef struct _tsZigbee_Node
{
    pthread_mutex_t mutex;
    
    struct
    {
        struct timeval  sLastSuccessfulOnOff;        /**< Time of last successful communications */
        struct timeval  sLastSuccessfulLevel;        /**< Time of last successful communications */
        struct timeval  sLastSuccessfulRGB;        /**< Time of last successful communications */
        struct timeval  sLastSuccessfulTemp;        /**< Time of last successful communications */
        struct timeval  sLastSuccessfulHumi;        /**< Time of last successful communications */
        struct timeval  sLastSuccessfulSimple;        /**< Time of last successful communications */
        struct timeval  sLastSuccessfulPower;        /**< Time of last successful communications */
        struct timeval  sLastSuccessfulIllu;        /**< Time of last successful communications */

        struct timeval  sLastSuccessful;        /**< Time of last successful communications */
        uint16        u16SequentialFailures;  /**< Number of sequential failures */
    } sComms;                                   /**< Structure containing communications statistics */

    char                device_name[MIBF];
    uint16              u16ShortAddress;
    uint64              u64IEEEAddress;
    uint8               u8DeviceOnline;
    char                *psDeviceDescription;
    
    uint16              u16DeviceID;
    uint32              u32NumEndpoints;
    tsNodeEndpoint      *pasEndpoints;
    
    uint32              u32NumGroups;
    uint16              *pau16Groups;
    uint8               u8MacCapability;

    uint8               u8LastNeighbourTableIndex;

    t_Method            Method;
    //Attribute
    uint8               u8LightMode;
    uint8               u8LightLevel;
    uint16              u16SenceId;
    uint16              u16TempValue;
    uint16              u16HumiValue;
    uint16              u16PowerValue;
    uint16              u16IlluValue;
    uint16              u16SimpleValue;
    uint8               u8DeviceTempAlarm;
    uint8               u8DeviceHumiAlarm;
    uint8               u8DeviceSimpAlarm;
    uint32              u32DeviceRGB;
    uint32              u32DeviceHSV;

    struct _tsZigbee_Node  *psNext;
    struct dl_list list;
} tsZigbee_Node;


/** Stucture for the Zigbee network */
typedef struct
{
    pthread_mutex_t         mutex;              /**< Lock for the node list */
    tsZigbee_Node           sNodes;             /**< Linked list of nodes. The head is the control bridge */
} tsZigbee_Network;                     

typedef enum
{
    E_ZIB_ATTRIBUTE_ONOFF   = 0x0006,
    E_ZIB_ATTRIBUTE_LEVEL   = 0x0008,
    E_ZIB_ATTRIBUTE_RGB     = 0x0300,
    
    E_ZIB_ATTRIBUTE_TEMP    = 0x0402,
    E_ZIB_ATTRIBUTE_HUMI    = 0x0405,
    E_ZIB_ATTRIBUTE_SIMPLE  = 0x000F,
    E_ZIB_ATTRIBUTE_ILLU    = 0x0040,
    E_ZIB_ATTRIBUTE_POWER   = 0x0001,
    
}eZigbeeAttributeMode;
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern tsZigbee_Network sZigbee_Network;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void        DBG_PrintNode(tsZigbee_Node *psNode);
void        vZigbee_NodeUpdateComms(tsZigbee_Node *psZigbeeNode, teZbStatus eStatus);

teZbStatus eZigbee_AddNode(uint16 u16ShortAddress, uint64 u64IEEEAddress, uint16 u16DeviceID, uint8 u8MacCapability, tsZigbee_Node **ppsZCBNode);
teZbStatus eZigbee_NodeAddEndpoint(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ProfileID, tsNodeEndpoint **ppsEndpoint);
teZbStatus eZigbee_NodeAddCluster(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID);
teZbStatus eZigbee_NodeAddAttribute(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint16 u16AttributeID);
teZbStatus eZigbee_NodeAddCommand(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint8 u8CommandID); 
teZbStatus eZigbee_NodeClearGroups(tsZigbee_Node *psZigbeeNode);
teZbStatus eZigbee_NodeAddGroup(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress);
teZbStatus eZigbee_RemoveNode(tsZigbee_Node *psZigbeeNode);
teZbStatus eZigbee_GetEndpoints(tsZigbee_Node *psZigbee_Node, eZigbee_ClusterID eClusterID, uint8 *pu8Src, uint8 *pu8Dst);


/** Find the first endpoint on a node that contains a cluster ID.
 *  \param psZCBNode        Pointer to node to search
 *  \param u16ClusterID     Cluster ID of interest
 *  \return A pointer to the endpoint or NULL if cluster ID not found
 */
tsNodeEndpoint *psZigbee_NodeFindEndpoint(tsZigbee_Node *psZigbeeNode, uint16 u16ClusterID);
tsZigbee_Node *psZigbee_FindNodeByShortAddress(uint16 u16ShortAddress); 
tsZigbee_Node *psZigbee_FindNodeByIEEEAddress(uint64 u64IEEEAddress);
tsZigbee_Node *psZigbee_FindNodeControlBridge(void);
int iZigbee_DeviceTimedOut(tsZigbee_Node *psZigbeeNode);
void vZigbee_NodeUpdateClusterComms(tsZigbee_Node *psZigbeeNode, eZigbee_ClusterID Zigbee_ClusterID);
int iZigbee_ClusterTimedOut(tsZigbee_Node *psZigbeeNode, eZigbee_ClusterID Zigbee_ClusterID);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ZIGBEENETWORK_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

