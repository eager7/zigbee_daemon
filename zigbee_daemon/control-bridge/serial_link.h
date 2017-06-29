/****************************************************************************
 *
 * MODULE:             zigbee - daemon
 *
 * COMPONENT:          SerialLink interface
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


#ifndef  SERIALLINK_H_INCLUDED
#define  SERIALLINK_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "serial.h"
#include "mthread.h"
#include "utils.h"
#include "list.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define SL_START_CHAR   0x01
#define SL_ESC_CHAR     0x02
#define SL_END_CHAR     0x03

#define SL_MAX_MESSAGE_LENGTH 256
#define SL_MAX_MESSAGE_QUEUES 3
#define SL_MAX_CALLBACK_QUEUES 3
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_SL_OK,
    E_SL_ERROR,
    E_SL_NOMESSAGE,
    E_SL_ERROR_SERIAL,
    E_SL_ERROR_NOMEM,
} teSL_Status;

/** Serial link message types */
typedef enum
{
    /* Common Commands */
    E_SL_MSG_STATUS                                             =   0x8000,
    E_SL_MSG_LOG                                                =   0x8001,
 
    E_SL_MSG_DATA_INDICATION                                    =   0x8002,
 
    E_SL_MSG_NODE_CLUSTER_LIST                                  =   0x8003,
    E_SL_MSG_NODE_ATTRIBUTE_LIST                                =   0x8004,
    E_SL_MSG_NODE_COMMAND_ID_LIST                               =   0x8005,
    E_SL_MSG_NODE_NON_FACTORY_NEW_RESTART                       =   0x8006,
    E_SL_MSG_NODE_FACTORY_NEW_RESTART                           =   0x8007,
    E_SL_MSG_GET_VERSION                                        =   0x0010,
    E_SL_MSG_VERSION_LIST                                       =   0x8010,
 
    E_SL_MSG_SET_EXT_PAN_ID                                      =   0x0020,
    E_SL_MSG_SET_CHANNEL_MASK                                    =   0x0021,
    E_SL_MSG_SET_SECURITY                                       =   0x0022,
    E_SL_MSG_SET_DEVICE_TYPE                                     =   0x0023,
    E_SL_MSG_START_NETWORK                                      =   0x0024,
    E_SL_MSG_NETWORK_JOINED_FORMED                              =   0x8024,
    E_SL_MSG_START_SCAN                                         =   0x0025,
    E_SL_MSG_NETWORK_REMOVE_DEVICE                              =   0x0026,
    E_SL_MSG_NETWORK_WHITELIST_ENABLE                           =   0x0027,
    E_SL_MSG_ADD_AUTHENTICATE_DEVICE                            =   0x0028,
    E_SL_MSG_AUTHENTICATE_DEVICE_RESPONSE                       =   0x8028,
    E_SL_MSG_CHANNEL_REQUEST                                    =   0x0029,
    E_SL_MSG_CHANNEL_RESPONSE                                    =   0x8029,

    E_SL_MSG_RESET                                              =   0x0011,
    E_SL_MSG_ERASE_PERSISTENT_DATA                              =   0x0012,
    E_SL_MSG_ZLL_FACTORY_NEW                                    =   0x0013,
    E_SL_MSG_GET_PERMIT_JOIN                                    =   0x0014,
    E_SL_MSG_GET_PERMIT_JOIN_RESPONSE                           =   0x8014,
    E_SL_MSG_BIND                                               =   0x0030,
    E_SL_MSG_BIND_RESPONSE                                      =   0x8030,
    E_SL_MSG_UNBIND                                             =   0x0031,
    E_SL_MSG_UNBIND_RESPONSE                                    =   0x8031,
    E_SL_MSG_BIND_GROUP						                    =   0x0032,
    E_SL_MSG_BIND_GROUP_RESPONSE			                    =   0x8032,
    E_SL_MSG_UNBIND_GROUP					                    =	0x0033,
    E_SL_MSG_UNBIND_GROUP_RESPONSE			                    =	0x8033,

    E_SL_MSG_NETWORK_ADDRESS_REQUEST                            =   0x0040,
    E_SL_MSG_NETWORK_ADDRESS_RESPONSE                           =   0x8040,
    E_SL_MSG_IEEE_ADDRESS_REQUEST                               =   0x0041,
    E_SL_MSG_IEEE_ADDRESS_RESPONSE                              =   0x8041,
    E_SL_MSG_NODE_DESCRIPTOR_REQUEST                            =   0x0042,
    E_SL_MSG_NODE_DESCRIPTOR_RESPONSE                           =   0x8042,
    E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST                          =   0x0043,
    E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE                         =   0x8043,
    E_SL_MSG_POWER_DESCRIPTOR_REQUEST                           =   0x0044,
    E_SL_MSG_POWER_DESCRIPTOR_RESPONSE                          =   0x8044,
    E_SL_MSG_ACTIVE_ENDPOINT_REQUEST                            =   0x0045,
    E_SL_MSG_ACTIVE_ENDPOINT_RESPONSE                           =   0x8045,
    E_SL_MSG_MATCH_DESCRIPTOR_REQUEST                           =   0x0046,
    E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE                          =   0x8046,
    E_SL_MSG_MANAGEMENT_LEAVE_REQUEST                           =   0x0047,
    E_SL_MSG_MANAGEMENT_LEAVE_RESPONSE                          =   0x8047,
    E_SL_MSG_LEAVE_INDICATION                                   =   0x8048,
    E_SL_MSG_PERMIT_JOINING_REQUEST                             =   0x0049,
    E_SL_MSG_MANAGEMENT_NETWORK_UPDATE_REQUEST 	                =	0x004A,
    E_SL_MSG_MANAGEMENT_NETWORK_UPDATE_RESPONSE                 =	0x804A,
    E_SL_MSG_SYSTEM_SERVER_DISCOVERY                            =   0x004B,
    E_SL_MSG_SYSTEM_SERVER_DISCOVERY_RESPONSE  	                =   0x804B,
    E_SL_MSG_LEAVE_REQUEST						                =	0x004C,
    E_SL_MSG_DEVICE_ANNOUNCE                                    =   0x004D,
    E_SL_MSG_MANAGEMENT_LQI_REQUEST                             =   0x004E,
    E_SL_MSG_MANAGEMENT_LQI_RESPONSE                            =   0x804E,
    E_SL_MSG_MANY_TO_ONE_ROUTE_REQUEST			                =	0x004F,


    /* Basic Cluster */
    E_SL_MSG_BASIC_RESET_TO_FACTORY_DEFAULTS			        =	0x0050,
    E_SL_MSG_BASIC_RESET_TO_FACTORY_DEFAULTS_RESPONSE	        =	0x8050,
     
    /* Group Cluster */
    E_SL_MSG_ADD_GROUP_REQUEST                                  =   0x0060,
    E_SL_MSG_ADD_GROUP_RESPONSE                                 =   0x8060,
    E_SL_MSG_VIEW_GROUP                                         =   0x0061,
    E_SL_MSG_VIEW_GROUP_RESPONSE                                =   0x8061,
    E_SL_MSG_GET_GROUP_MEMBERSHIP_REQUEST                       =   0x0062,
    E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE                      =   0x8062,
    E_SL_MSG_REMOVE_GROUP_REQUEST                               =   0x0063,
    E_SL_MSG_REMOVE_GROUP_RESPONSE                              =   0x8063,
    E_SL_MSG_REMOVE_ALL_GROUPS                                  =   0x0064,
    E_SL_MSG_ADD_GROUP_IF_IDENTIFY                              =   0x0065,
 
    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_SEND                                      =   0x0070,
    E_SL_MSG_IDENTIFY_QUERY                                     =   0x0071,
 
    /* Level Cluster */
    E_SL_MSG_MOVE_TO_LEVEL                                      =   0x0080,
    E_SL_MSG_MOVE_TO_LEVEL_ONOFF                                =   0x0081,
    E_SL_MSG_MOVE_STEP                                          =   0x0082,
    E_SL_MSG_MOVE_STOP_MOVE                                     =   0x0083,
    E_SL_MSG_MOVE_STOP_ONOFF                                    =   0x0084,

    /* Scenes Cluster */
    E_SL_MSG_VIEW_SCENE                                         =   0x00A0,
    E_SL_MSG_VIEW_SCENE_RESPONSE                                =   0x80A0,
    E_SL_MSG_ADD_SCENE                                          =   0x00A1,
    E_SL_MSG_ADD_SCENE_RESPONSE                                 =   0x80A1,
    E_SL_MSG_REMOVE_SCENE                                       =   0x00A2,
    E_SL_MSG_REMOVE_SCENE_RESPONSE                              =   0x80A2,
    E_SL_MSG_REMOVE_ALL_SCENES                                  =   0x00A3,
    E_SL_MSG_REMOVE_ALL_SCENES_RESPONSE                         =   0x80A3,
    E_SL_MSG_STORE_SCENE                                        =   0x00A4,
    E_SL_MSG_STORE_SCENE_RESPONSE                               =   0x80A4,
    E_SL_MSG_RECALL_SCENE                                       =   0x00A5,
    E_SL_MSG_SCENE_MEMBERSHIP_REQUEST                           =   0x00A6,
    E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE                          =   0x80A6,
 
    /* Colour Cluster */
    E_SL_MSG_MOVE_TO_HUE                                        =   0x00B0,
    E_SL_MSG_MOVE_HUE                                           =   0x00B1,
    E_SL_MSG_STEP_HUE                                           =   0x00B2,
    E_SL_MSG_MOVE_TO_SATURATION                                 =   0x00B3,
    E_SL_MSG_MOVE_SATURATION                                    =   0x00B4,
    E_SL_MSG_STEP_SATURATION                                    =   0x00B5,
    E_SL_MSG_MOVE_TO_HUE_SATURATION                             =   0x00B6,
    E_SL_MSG_MOVE_TO_COLOUR                                     =   0x00B7,
    E_SL_MSG_MOVE_COLOUR                                        =   0x00B8,
    E_SL_MSG_STEP_COLOUR                                        =   0x00B9,
 
    /* ZLL Commands */
    /* Touchlink */
    E_SL_MSG_INITIATE_TOUCHLINK                                 =   0x00D0,
    E_SL_MSG_TOUCHLINK_STATUS                                   =   0x00D1,
    E_SL_MSG_TOUCHLINK_FACTORY_RESET                            =   0x00D2,
    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_TRIGGER_EFFECT                            =   0x00E0,
 
    /* On/Off Cluster */
    E_SL_MSG_ONOFF_NOEFFECTS                                    =   0x0092,
    E_SL_MSG_ONOFF_TIMED                                        =   0x0093,
    E_SL_MSG_ONOFF_EFFECTS                                      =   0x0094,
    E_SL_MSG_ONOFF_UPDATE                                       =   0x8095,

    /* Scenes Cluster */
    E_SL_MSG_ADD_ENHANCED_SCENE                                 =   0x00A7,
    E_SL_MSG_VIEW_ENHANCED_SCENE                                =   0x00A8,
    E_SL_MSG_COPY_SCENE                                         =   0x00A9,
 
    /* Colour Cluster */
    E_SL_MSG_ENHANCED_MOVE_TO_HUE                               =   0x00BA,
    E_SL_MSG_ENHANCED_MOVE_HUE                                  =   0x00BB,
    E_SL_MSG_ENHANCED_STEP_HUE                                  =   0x00BC,
    E_SL_MSG_ENHANCED_MOVE_TO_HUE_SATURATION                    =   0x00BD,
    E_SL_MSG_COLOUR_LOOP_SET                                    =   0x00BE,
    E_SL_MSG_STOP_MOVE_STEP                                     =   0x00BF,
    E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE                         =   0x00C0,
    E_SL_MSG_MOVE_COLOUR_TEMPERATURE                            =   0x00C1,
    E_SL_MSG_STEP_COLOUR_TEMPERATURE                            =   0x00C2,

    /* Closures Cluster */
    E_SL_MSG_LOCK_UNLOCK_DOOR                                   =   0x00F0,
    E_SL_MSG_WINDOW_COVERING_DEVICE_OPERATOR                    =   0x00F1,
    E_SL_MSG_LOCK_UNLOCK_DOOR_UPDATE                            =   0x00F2,
    E_SL_MSG_DOOR_LOCK_SET_DOOR_STATE                           =   0x00F3,
    E_SL_MSG_DOOR_LOCK_SET_DOOR_PASSWORD                        =   0x00F4,

    /* ZHA Commands */
    E_SL_MSG_READ_ATTRIBUTE_REQUEST                             =   0x0100,
    E_SL_MSG_READ_ATTRIBUTE_RESPONSE                            =   0x8100,
    E_SL_MSG_DEFAULT_RESPONSE                                   =   0x8101,
    E_SL_MSG_REPORT_IND_ATTR_RESPONSE                           =   0x8102,
    E_SL_MSG_WRITE_ATTRIBUTE_REQUEST                            =   0x0110,
    E_SL_MSG_WRITE_ATTRIBUTE_RESPONSE                           =   0x8110,
    E_SL_MSG_CONFIG_REPORTING_REQUEST                           =   0x0120,
    E_SL_MSG_CONFIG_REPORTING_RESPONSE                          =   0x8120,
    E_SL_MSG_REPORT_ATTRIBUTES					                =	0x8121,
    E_SL_MSG_READ_REPORT_CONFIG_REQUEST     	                =   0x0122,
    E_SL_MSG_READ_REPORT_CONFIG_RESPONSE                        =   0x8122,
    E_SL_MSG_ATTRIBUTE_DISCOVERY_REQUEST		                =   0x0140,
    E_SL_MSG_ATTRIBUTE_DISCOVERY_RESPONSE		                =   0x8140,
    E_SL_MSG_ATTRIBUTE_EXT_DISCOVERY_REQUEST	                =   0x0141,
    E_SL_MSG_ATTRIBUTE_EXT_DISCOVERY_RESPONSE	                =	0x8141,
    E_SL_MSG_COMMAND_RECEIVED_DISCOVERY_REQUEST	                =   0x0150,
    E_SL_MSG_COMMAND_RECEIVED_DISCOVERY_INDIVIDUAL_RESPONSE		=   0x8150,
    E_SL_MSG_COMMAND_RECEIVED_DISCOVERY_RESPONSE    			=   0x8151,
    E_SL_MSG_COMMAND_GENERATED_DISCOVERY_REQUEST				=   0x0160,
    E_SL_MSG_COMMAND_GENERATED_DISCOVERY_INDIVIDUAL_RESPONSE	=   0x8160,
    E_SL_MSG_COMMAND_GENERATED_DISCOVERY_RESPONSE   			=   0x8161,

    E_SL_MSG_SAVE_PDM_RECORD                                    =   0x0200,
    E_SL_MSG_SAVE_PDM_RECORD_RESPONSE                           =   0x8200,
    E_SL_MSG_LOAD_PDM_RECORD_REQUEST        	                =   0x0201,
    E_SL_MSG_LOAD_PDM_RECORD_RESPONSE       	                =   0x8201,
    E_SL_MSG_DELETE_PDM_RECORD                                  =   0x0202,

    E_SL_MSG_PDM_HOST_AVAILABLE                                 =   0x0300,
    E_SL_MSG_ASC_LOG_MSG                                        =   0x0301,
    E_SL_MSG_ASC_LOG_MSG_RESPONSE                               =   0x8301,
    E_SL_MSG_PDM_HOST_AVAILABLE_RESPONSE                        =   0x8300,
    /* IAS Cluster */
    E_SL_MSG_SEND_IAS_ZONE_ENROLL_RSP			                =	0x0400,
    E_SL_MSG_IAS_ZONE_STATUS_CHANGE_NOTIFY		                =	0x8401,

    /* OTA Cluster */
    E_SL_MSG_LOAD_NEW_IMAGE					                    =	 0x0500,
    E_SL_MSG_BLOCK_REQUEST					                    =	 0x8501,
    E_SL_MSG_BLOCK_SEND						                    =	 0x0502,
    E_SL_MSG_UPGRADE_END_REQUEST			                    =	 0x8503,
    E_SL_MSG_UPGRADE_END_RESPONSE			                    = 	 0x0504,
    E_SL_MSG_IMAGE_NOTIFY					                    = 	 0x0505,
    E_SL_MSG_SEND_WAIT_FOR_DATA_PARAMS                          =  	 0x0506,
    
    E_SL_MSG_APS_DATA_REQ						                =	 0x0530,
    E_SL_MSG_COMPLEX_DESCRIPTOR_REQUEST			                =	 0x0531,
    E_SL_MSG_COMPLEX_DESCRIPTOR_RESPONSE		                =	 0x8531,
    E_SL_MSG_USER_DESCRIPTOR_REQUEST			                =	 0x0532,
    E_SL_MSG_USER_DESCRIPTOR_RESPONSE			                =	 0x8532,
    E_SL_MSG_USER_DESCRIPTOR_SET_REQUEST		                =	 0x0533,
    E_SL_MSG_USER_DESCRIPTOR_SET_CONFIRM		                =	 0x8533,
    
    E_SL_MSG_ROUTE_DISCOVERY_CONFIRM                            =    0x8701,
    E_SL_MSG_APS_DATA_CONFIRM_FAILED                            =    0x8702,

} teSL_MsgType;

typedef struct  /**This type just use for status message */
{
    enum
    {
        E_SL_MSG_STATUS_SUCCESS,
        E_SL_MSG_STATUS_INCORRECT_PARAMETERS,
        E_SL_MSG_STATUS_UNHANDLED_COMMAND,
        E_SL_MSG_STATUS_BUSY,
        E_SL_MSG_STATUS_STACK_ALREADY_STARTED,
        
    } PACKED eStatus;
    uint8             u8SequenceNo;           /**< Sequence number of outgoing message */
    uint16            u16MessageType;         /**< Type of message that this is status to */
    char              acMessage[];            /**< Optional message */
} PACKED tsSL_Msg_Status;

typedef enum
{
    E_STATE_RX_WAIT_START,
    E_STATE_RX_WAIT_TYPEMSB,
    E_STATE_RX_WAIT_TYPELSB,
    E_STATE_RX_WAIT_LENMSB,
    E_STATE_RX_WAIT_LENLSB,
    E_STATE_RX_WAIT_CRC,
    E_STATE_RX_WAIT_DATA,
} teSL_RxState;

typedef struct  /** Structure used to contain a message */
{
    uint16 u16Type;
    uint16 u16Length;
    uint8  au8Message[SL_MAX_MESSAGE_LENGTH];
} tsSL_Message;

typedef struct 
{
    uint16 u16Type;
    uint16 u16Length;
    uint8 *pu8Message;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_data_available;
} tsReaderMessageQueue;

/** Structure of data for the serial link */
typedef struct
{
    int iSerialFd;
    pthread_mutex_t mutex_write;
    tsThread sSerialReaderThread;
    tsReaderMessageQueue asReaderMessageQueue[SL_MAX_MESSAGE_QUEUES];
} tsSerialLink;

/** Callback function for a given message type 
 *  \param pvUser           User supplied pointer to be passed to the callback function
 *  \param u16Length        Length of the received message
 *  \param pvMessage        Pointer to the message data.
 *  \return Nothing
 */
typedef void (*tprSL_MessageCallback)(void *pvUser, uint16 u16Length, void *pvMessage);

/** Linked list structure for a callback function entry */
typedef struct _tsSL_CallbackEntry
{
    uint16                      u16Type;        /**< Message type for this callback */
    tprSL_MessageCallback       prCallback;     /**< User supplied callback function for this message type */
    void                        *pvUser;        /**< User supplied data for the callback function */
    struct dl_list              list;
    pthread_mutex_t             mutex;
} tsSL_CallbackEntry;

/** Structure allocated and passed to callback handler thread */
typedef struct
{
    tsSL_Message            sMessage;       /** The received message */ 
    tprSL_MessageCallback   prCallback;     /**< User supplied callback function for this message type */
    void *                  pvUser;         /**< User supplied data for the callback function */
} tsCallbackThreadData;

typedef struct _tsSL_CallBack
{
    tsQueue  sQueue;                     /*The Message queue of serial read thread and callback handle thread*/
    tsThread sThread;                    /*Thread for serial message callback handle*/
    tsSL_CallbackEntry sCallbackHead;    /*Serial callback list*/
}tsSL_CallBack;
/** Message Handle for Serial Waiting*/
typedef struct _tsSL_MsgBrocast
{
    tsQueue  sQueue;        /*The message queue of serial read thread and message brocast thread*/
    tsThread sThread;       /*Thread for serial message brocast*/
}tsSL_MsgBrocast;
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

teSL_Status eSL_Init(char *cpSerialDevice, uint32 u32BaudRate);

teSL_Status eSL_Destroy(void);

/** Send a command message to the serial device.
 *  This also listens for the returned Status message.
 *  If one is received, the status for the message is returned, otherwise
 *  E_SL_ERROR is returned.
 *  \param u16Type          Type of message to send
 *  \param pu16Length       Message length
 *  \param pvMessage        Message data buffer
 *  \param pu8SequenceNo    Pointer to location to receive the outgoing sequence number. May be NULL if no sequence expected.
 */
teSL_Status eSL_SendMessage(uint16 u16Type, uint16 u16Length, void *pvMessage, uint8 *pu8SequenceNo);

/** Wait for a message of the given type to be received from the serial device
 *  \param u16Type          Type of message to wait for
 *  \param u32WaitTimeout   Maximum time to wait for messages (ms)
 *  \param pu16Length       Pointer to location to receive message length
 *  \param ppvMessage       Pointer to location to receive a pointer to the message buffer
 *                          Once a message buffer has been returned, the calling function 
 *                          has the responsibility of freeing the buffer,
 *  \return E_SL_OK on success
 */
teSL_Status eSL_MessageWait(uint16 u16Type, uint32 u32WaitTimeout, uint16 *pu16Length, void **ppvMessage);

/** Add a callback function for a particular message type
 *  The callback function will be called in the context of a new thread that exists 
 *  only to service the incoming message and will subsequently be destroyed.
 *  Multiple callbacks for a given message type may be registered.
 *  \param u16Type          Type of message to register a handler for
 *  \param prCallback       Callback function to be called when a message of this type arrives.
 *  \return E_SL_OK on success.
 */
teSL_Status eSL_AddListener(uint16 u16Type, tprSL_MessageCallback prCallback, void *pvUser);

/** Remove a callback function for a particular message type
 *  \param u16Type          Type of message to remove a handler for
 *  \param prCallback       Callback function to be removed for this message type
 *  \return E_SL_OK on success.
 */
teSL_Status eSL_RemoveListener(uint16 u16Type, tprSL_MessageCallback prCallback);
teSL_Status eSL_RemoveAllListener(void);


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* SERIALLINK_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
