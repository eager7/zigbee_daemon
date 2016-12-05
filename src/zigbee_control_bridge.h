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


#ifndef  MODULECONFIG_H_INCLUDED
#define  MODULECONFIG_H_INCLUDED

#include <stdint.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "utils.h"
#include "threads.h"
#include "serial_link.h"
#include "zigbee_constant.h"
#include "zigbee_network.h"

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define OTA_SERVER

/* Default network configuration */
#define CONFIG_DEFAULT_START_MODE       E_START_COORDINATOR
#define CONFIG_DEFAULT_CHANNEL          20
#define CONFIG_DEFAULT_PANID            0x1234567812345678ll

#define MAC_CAP_PAN_COOR        (1<<0)
#define MAC_CAP_DEVICE_TYPE     (1<<1)
#define MAC_CAP_POWER_SOURCE    (1<<2)
#define MAC_CAP_REVICE_IDLE     (1<<3)
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/** Enumerated type of events */
typedef enum
{
    E_ZCB_EVENT_NETWORK_FORMED,         /**< Control bridge formed a new network.
                                         *   pvData points to the control bridge \ref tsZCB_Node structure */
    E_ZCB_EVENT_NETWORK_JOINED,         /**< Control bridge joined an existing network.
                                         *   pvData points to the control bridge \ref tsZCB_Node structure */
    E_ZCB_EVENT_DEVICE_ANNOUNCE,        /**< A new device joined the network */
    E_ZCB_EVENT_DEVICE_MATCH,           /**< A device responded to a match descriptor request */
    E_ZCB_EVENT_ATTRIBUTE_REPORT,       /**< A device has sent us an attribute report */
} teZcbEvent;

/** Enumerated type of start modes */
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


/** Enumerated type of allowable channels */
typedef enum
{
    E_CHANNEL_AUTOMATIC     = 0,
    E_CHANNEL_MINIMUM       = 11,
    E_CHANNEL_MAXIMUM       = 26
} teChannel;

/** Structure passed from ZCB library to application for an event. */
typedef struct
{
    teZcbEvent          eEvent;
    union {
        struct 
        {
        } sNetworkFormed;
        struct 
        {
        } sNetworkJoined;
        struct
        {
            uint16    u16ShortAddress;
        } sDeviceAnnounce;
        struct 
        {
            uint16    u16ShortAddress;
        } sDeviceMatch;
        struct
        {
            uint16                u16ShortAddress;
            uint8                 u8Endpoint;
            uint16                u16ClusterID;
            uint16                u16AttributeID;
            teZCL_ZCLAttributeType  eType;
            tuZcbAttributeData      uData;
        } sAttributeReport;
    } uData;
} tsZcbEvent;

/** Default response message */
typedef struct
{
    uint8             u8SequenceNo;           /**< Sequence number of outgoing message */
    uint8             u8Endpoint;             /**< Source endpoint */
    uint16            u16ClusterID;           /**< Source cluster ID */
    uint8             u8CommandID;            /**< Source command ID */
    uint8             u8Status;               /**< Command status */
} PACKED tsSL_Msg_DefaultResponse;

#ifdef OTA_SERVER
typedef enum 
{
    E_CLD_OTA_QUERY_JITTER,
    E_CLD_OTA_MANUFACTURER_ID_AND_JITTER,
    E_CLD_OTA_ITYPE_MDID_JITTER,
    E_CLD_OTA_ITYPE_MDID_FVERSION_JITTER
}teOTA_ImageNotifyPayloadType;
#endif
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/** Start control bridge mode */
extern teStartMode      eStartMode;
/** Channel number to operate on */
extern teChannel        eChannel;
/** IEEE802.15.4 extended PAN ID */
extern uint64         u64PanID;
/** Control bridge software version */
extern uint32         u32ZCB_SoftwareVersion;
/** Flag to enable / disable APS acks on packets sent from the control bridge. */
extern int              bZCB_EnableAPSAck;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/** Structure of Zigbee Device ID to JIP device ID mappings.
 *  When a node joins this structure is used to map the Zigbee device to 
 *  a JIP device, and the prInitaliseRoutine is called to populate a
 *  newly created JIP psJIPNode with data.
 */
typedef struct
{
    uint16                  u16ZigbeeDeviceID;      /**< Zigbee Deive ID */
    uint32                  u32JIPDeviceID;         /**< Corresponding JIP device ID */
    tpreDeviceInitialise    prInitaliseRoutine;     /**< Initialisation routine for the JIP device */
    //tprAttributeUpdate      prAttributeUpdateRoutine;   /**< Attribute update routine for the JIP device */
} tsDeviceIDMap;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/** Initialise control bridge connected to serial port */
teZbStatus eZCB_Init();

/** Finished with control bridge - call this to tidy up */ 
teZbStatus eZCB_Finish(void);

/** Attempt to establish comms with the control bridge.
 *  \return E_ZCB_OK when comms established, otherwise E_ZCB_COMMS_FAILED
 */
teZbStatus eZCB_EstablishComms(void);

/** Make control bridge factory new, and set up configuration. */
teZbStatus eZCB_FactoryNew(void);

teZbStatus eZCB_SetExtendedPANID(uint64 u64PanID);

teZbStatus eZCB_SetChannelMask(uint32 u32ChannelMask);

teZbStatus eZCB_SetInitialSecurity(uint8 u8State, uint8 u8Sequence, uint8 u8Type, uint8 *pu8Key);

/** Set the module mode */
teZbStatus eZCB_SetDeviceType(teModuleMode eModuleMode);

/** Start the network */
teZbStatus eZCB_StartNetwork(void);

/** Get the permit joining status of the control bridge */
teZbStatus eZCB_GetPermitJoining(uint8 *pu8Status);

/** Set whitelisting enable status */
teZbStatus eZCB_SetWhitelistEnabled(uint8 bEnable);

/** Authenticate a device into the network.
 *  \param u64IEEEAddress       MAC address to add
 *  \param au8LinkKey           Pointer to 16 byte device link key
 *  \param au8NetworkKey[out]   Pointer to 16 byte location to put encrypted network key
 *  \param au8MIC[out]          Pointer to location to put 4 byte encryption MIC
 *  \param pu64TrustCenterAddress Pointer to location to put trust center address
 *  \param pu8KeySequenceNumber Pointer to location to store active key sequence number
 *  \return E_ZCB_OK on success
 */
teZbStatus eZCB_AuthenticateDevice(uint64 u64IEEEAddress, uint8 *pau8LinkKey, 
                                    uint8 *pau8NetworkKey, uint8 *pau8MIC,
                                    uint64 *pu64TrustCenterAddress, uint8 *pu8KeySequenceNumber);

/** Initiate Touchlink */
teZbStatus eZCB_ZLL_Touchlink(void);


/** Send a match descriptor request */
teZbStatus eZCB_MatchDescriptorRequest(uint16 u16TargetAddress, uint16 u16ProfileID, 
                                        uint8 u8NumInputClusters, uint16 *pau16InputClusters, 
                                        uint8 u8NumOutputClusters, uint16 *pau16OutputClusters,
                                        uint8 *pu8SequenceNo);


teZbStatus eZCB_IEEEAddressRequest(tsZigbee_Node *psZigbeeNode);


/** Send a node descriptor request */
teZbStatus eZCB_NodeDescriptorRequest(tsZigbee_Node *psZigbeeNode);

/** Send a simple descriptor request and use it to populate a node structure */
teZbStatus eZCB_SimpleDescriptorRequest(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint);


/** Send a request for the neighbour table to a node */
teZbStatus eZCB_NeighbourTableRequest(int *pStart);

/** Request an attribute from a node. The data is requested, parsed, and passed back.
 *  \param psZigbeeNode            Pointer to node from which to read the attribute
 *  \param u16ClusterID         Cluster ID to read the attribute from. The endpoint containing this cluster is determined from the psZigbeeNode structure.
 *  \param u8Direction          0 = Read from client to server. 1 = Read from server to client.
 *  \param u8ManufacturerSpecific 0 = Normal attribute. 1 = Manufacturer specific attribute
 *  \param u16ManufacturerID    if u8ManufacturerSpecific = 1, then the manufacturer ID of the attribute
 *  \param u16AttributeID       ID of the attribute to read
 *  \param pvData[out]          Pointer to location to store the read data. It is assumed that the caller knows how big the requested data will be 
 *                              and has allocated a buffer big enough to contain it. Failure will reault in memory corruption.
 *  \return E_ZCB_OK on success.
 */
teZbStatus eZCB_ReadAttributeRequest(tsZigbee_Node *psZigbeeNode, uint16 u16ClusterID,
                                      uint8 u8Direction, uint8 u8ManufacturerSpecific, uint16 u16ManufacturerID,
                                      uint16 u16AttributeID, void *pvData);
                                      

/** Write an attribute to a node.
 *  \param psZigbeeNode            Pointer to node to write the attribute to
 *  \param u16ClusterID         Cluster ID to write the attribute to. The endpoint containing this cluster is determined from the psZigbeeNode structure.
 *  \param u8Direction          0 = write from client to server. 1 = write from server to client.
 *  \param u8ManufacturerSpecific 0 = Normal attribute. 1 = Manufacturer specific attribute
 *  \param u16ManufacturerID    if u8ManufacturerSpecific = 1, then the manufacturer ID of the attribute
 *  \param u16AttributeID       ID of the attribute to write
 *  \param eType                Type of the attribute being written
 *  \param pvData               Pointer to buffer of data to write. This must be of the right size for the attribute that is being written. 
 *                              Failure will reault in memory corruption.
 *  \return E_ZCB_OK on success.
 */
teZbStatus eZCB_WriteAttributeRequest(tsZigbee_Node *psZigbeeNode, uint16 u16ClusterID,
                                      uint8 u8Direction, uint8 u8ManufacturerSpecific, uint16 u16ManufacturerID,
                                      uint16 u16AttributeID, teZCL_ZCLAttributeType eType, void *pvData);

/** Wait for default response to a command with sequence number u8SequenceNo.
 *  \param u8SequenceNo         Sequence number of command to wait for resonse to
 *  \return Status from default response.
 */
teZbStatus eZCB_GetDefaultResponse(uint8 u8SequenceNo);

/** Add a node to a group */
teZbStatus eZCB_AddGroupMembership(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress);

/** Remove a node from a group */
teZbStatus eZCB_RemoveGroupMembership(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress);

/** Populate the nodes group membership information */
teZbStatus eZCB_GetGroupMembership(tsZigbee_Node *psZigbeeNode);

/** Remove a node from all groups */
teZbStatus eZCB_ClearGroupMembership(tsZigbee_Node *psZigbeeNode);

/** Remove a scene in a node */
teZbStatus eZCB_RemoveScene(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);

/** Store a scene in a node */
teZbStatus eZCB_StoreScene(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);

/** Recall a scene */
teZbStatus eZCB_RecallScene(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8SceneID);

/** Get a nodes scene membership */
teZbStatus eZCB_GetSceneMembership(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 *pu8NumScenes, uint8 **pau8Scenes);

/** Get a locked node structure for the control bridge
 *  \return pointer to control bridge
 */
tsZigbee_Node *psZCB_NodeOldestComms(void);
teZbStatus eZCB_SetPermitJoining(uint8 u8Interval);

/** Bind */
teZbStatus eZCB_BindRequest(tsZigbee_Node *psZigbeeNode, uint64 u64DestinationAddress, uint16 u16BindCluster);
teZbStatus eZCB_UnBindRequest(tsZigbee_Node *psZigbeeNode, uint64 u64DestinationAddress, uint16 u16BindCluster);
teZbStatus eZCB_ManagementLeaveRequest(tsZigbee_Node *psZigbeeNode);
//ZLL
teZbStatus eZBZLL_OnOff(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Mode);
teZbStatus eZBZLL_OnOffCheck(tsZigbee_Node *psZigbeeNode, uint8 u8Mode);
teZbStatus eZBZLL_MoveToLevel(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8OnOff, uint8 u8Level, uint16 u16TransitionTime);
teZbStatus eZBZLL_MoveToLevelCheck(tsZigbee_Node *psZigbeeNode, uint8 u8OnOff, uint8 u8Level, uint16 u16TransitionTime);
teZbStatus eZBZLL_MoveToHue(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Hue, uint16 u16TransitionTime);
teZbStatus eZBZLL_MoveToSaturation(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Saturation, uint16 u16TransitionTime);
teZbStatus eZBZLL_MoveToHueSaturation(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint8 u8Hue, uint8 u8Saturation, uint16 u16TransitionTime);
teZbStatus eZBZLL_MoveToColour(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress, uint16 u16X, uint16 u16Y, uint16 u16TransitionTime);
teZbStatus eZBZLL_MoveToColourTemperature(tsZigbee_Node *psZigbeeNode, 
    uint16 u16GroupAddress, uint16 u16ColourTemperature, uint16 u16TransitionTime);
teZbStatus eZBZLL_MoveColourTemperature(tsZigbee_Node *psZigbeeNode,
    uint16 u16GroupAddress, uint8 u8Mode, uint16 u16Rate, uint16 u16ColourTemperatureMin, uint16 u16ColourTemperatureMax);
teZbStatus eZCB_SendMatchDescriptorRequest(void);

#ifdef OTA_SERVER
teZbStatus sendOtaLoadNewImage(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint32 u32FileIdentifier, 
                                uint16 u16HeaderVersion, uint16 u16HeaderLength, uint16 u16HeaderControlField, 
                                uint16 u16ManufacturerCode, uint16 u16ImageType, uint32 u32FileVersion, 
                                uint16 u16StackVersion, char *au8HeaderString, uint32 u32TotalImage, 
                                uint8 u8SecurityCredVersion, uint64 u64UpgradeFileDest, uint16 u16MinimumHwVersion, 
                                uint16 u16MaxHwVersion);
teZbStatus sendOtaBlock(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint8 u8SrcEndPoint, 
                        uint8 u8DstEndPoint, uint8 u8SeqNbr, uint8 u8Status, uint32 u32FileOffset, 
                        uint32 u32FileVersion, uint16 u16ImageType, uint16 u16ManuCode, 
                        uint8 u8DataSize, char *au8Data);
teZbStatus sendOtaEndResponse(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint8 u8SrcEndPoint, 
                                uint8 u8DstEndPoint, uint8 u8SeqNbr, uint32 u32UpgradeTime, 
                                uint32 u32CurrentTime, uint32 u32FileVersion, uint16 u16ImageType,
                                uint16 u16ManuCode);
teZbStatus sendOtaImageNotify(uint8 u8DstAddrMode, uint16 u16ShortAddr, uint8 u8SrcEndPoint, 
                                uint8 u8DstEndPoint, uint8 u8NotifyType, uint32 u32FileVersion, 
                                uint16 u16ImageType, uint16 u16ManuCode, uint8 u8Jitter);
#endif
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* MODULECONFIG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

