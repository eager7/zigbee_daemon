/****************************************************************************
 *
 * MODULE:             Zigbee - daemon
 *
 * COMPONENT:          SocketServer interface
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


#ifndef  ZIGBEE_SOCKET_H_INCLUDED
#define  ZIGBEE_SOCKET_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <netinet/in.h>
#include "mthread.h"
#include "zigbee_node.h"
#include "zigbee_sqlite.h"
#include "zigbee_control_bridge.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define SOCKET_SERVER_PORT 6667
#define NUMBER_SOCKET_CLIENT 5

#define JSON_TYPE "type"
#define JSON_STATUS "status"
#define JSON_SEQUENCE "sequence"
#define JSON_INFO "information"
#define JSON_MAC "mac"
#define JSON_VERSION "version"
#define JSON_TIME "time"
#define JSON_CHANNEL "channel"
#define JSON_DEVICES "devices"
#define JSON_NAME "name"
#define JSON_ID "id"
#define JSON_ONLINE "online"
#define JSON_GROUP "group_id"
#define JSON_MODE "mode"
#define JSON_LEVEL "level"
#define JSON_COLOR "RGB"
#define JSON_REJOIN "rejoin"
#define JSON_REMOVE_CHILDREN "remove_children"
#define JSON_PASSWORD_AVAILABLE "available"
#define JSON_PASSWORD_LEN "length"
#define JSON_PASSWORD "password"
#define JSON_ALARM "alarm"
#define JSON_RECORDS "records"
#define JSON_NUM "number"
#define JSON_PERM "permission"

#define JSON_SENSOR "cluster"
#define JSON_COMMAND "command"
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum
{
    E_SS_OK                         = 0x00,
    E_SS_INCORRECT_PARAMETERS       = 0x01,
    E_SS_ERROR_UNHANDLED_COMMAND    = 0x02,
    E_SS_ERROR                      = 0x03,
    E_SS_BUSY                       = 0x04,

    E_SS_ERROR_SOCKET,
    E_SS_ERROR_BIND,
    E_SS_ERROR_LISTEN,
} teSS_Status;

typedef enum
{
    /* Manage */
    E_SS_COMMAND_STATUS                     = 0x8000,
    E_SS_COMMAND_GET_MAC                    = 0x0010,
    E_SS_COMMAND_GET_VERSION                = 0x0011,
    E_SS_COMMAND_VERSION_LIST               = 0x8011,
    E_SS_COMMAND_OPEN_NETWORK               = 0x0012,
    E_SS_COMMAND_GET_CHANNEL                = 0x0013,
    E_SS_COMMAND_GET_CHANNEL_RESPONSE       = 0x8013,
    E_SS_COMMAND_GET_DEVICES_LIST_ALL       = 0x0014,
    E_SS_COMMAND_GET_DEVICES_RESPONSE       = 0x8014,
    E_SS_COMMAND_LEAVE_NETWORK              = 0x0015,
    E_SS_COMMAND_COORDINATOR_UPGRADE        = 0x0016,

    /* ZLL */
    E_SS_COMMAND_LIGHT_SET_ON_OFF           = 0x0020,
    E_SS_COMMAND_LIGHT_SET_LEVEL            = 0x0021,
    E_SS_COMMAND_LIGHT_SET_RGB              = 0x0022,
    E_SS_COMMAND_LIGHT_GET_STATUS           = 0x0023,
    E_SS_COMMAND_LIGHT_GET_STATUS_RESPONSE  = 0x8023,
    E_SS_COMMAND_LIGHT_GET_LEVEL            = 0x0024,
    E_SS_COMMAND_LIGHT_GET_LEVEL_RESPONSE   = 0x8024,
    E_SS_COMMAND_LIGHT_GET_RGB              = 0x0025,
    E_SS_COMMAND_LIGHT_GET_RGB_RESPONSE     = 0x8025,
    /* Sensor */
    E_SS_COMMAND_SENSOR_GET_SENSOR          = 0x0030,
    E_SS_COMMAND_SENSOR_GET_ALARM           = 0x0031,
    E_SS_COMMAND_SENSOR_GET_ALL_ALARM       = 0x0032,

    /* Closure */
    E_SS_COMMAND_SET_CLOSURES_STATE         = 0x0040,
    /* Door */
    E_SS_COMMAND_DOOR_LOCK_ADD_PASSWORD             = 0x00F0,
    E_SS_COMMAND_DOOR_LOCK_DEL_PASSWORD             = 0x00F1,
    E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD             = 0x00F2,
    E_SS_COMMAND_DOOR_LOCK_GET_PASSWORD_RESPONSE    = 0x80F2,
    E_SS_COMMAND_DOOR_LOCK_GET_RECORD               = 0x00F3,
    E_SS_COMMAND_DOOR_LOCK_GET_RECORD_RESPONSE      = 0x80F3,
    E_SS_COMMAND_DOOR_LOCK_ALARM_REPORT             = 0x00F4,
    E_SS_COMMAND_DOOR_LOCK_OPEN_REPORT              = 0x00F5,
    E_SS_COMMAND_DOOR_LOCK_ADD_USER_REPORT          = 0x00F6,
    E_SS_COMMAND_DOOR_LOCK_DEL_USER_REPORT          = 0x00F7,
    E_SS_COMMAND_DOOR_LOCK_GET_USER                 = 0x00F8,
    E_SS_COMMAND_DOOR_LOCK_GET_USER_RESPONSE        = 0x80F8,
    E_SS_COMMAND_SET_DOOR_LOCK_STATE                = 0x00F9,

    /* Discovery */
    E_SS_COMMAND_DISCOVERY                  = 0xFFFF,

}teSocketCommand;

typedef enum 
{
    SUCCESS,
    FAILED,
}teJsonStatus;

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct
{
    int      iSocketFd;
    uint8    u8NumClientSocket;
    tsQueue  sQueue;
    tsThread sThreadSocket;
    tsThread sThreadQueue;
} tsSocketServer;

typedef struct _clientSocket
{
    uint16 u16Length;
    int iSocketClient;
    char auClientData[MABF];
    struct sockaddr_in addrclint;
}tsClientSocket;

/** Structure allocated and passed to callback handler thread */
typedef struct
{
    int iSocketClientfd;
    uint16 u16Length;
    uint8  au8Message[MABF];
} tsSSCallbackThreadData;

typedef teSS_Status (*tpreMessageHandlePacket)(int iSocketfd, struct json_object *psJsonMessage);
typedef struct _tsSocketHandleMap
{
    teSocketCommand eSocketCommand;
    tpreMessageHandlePacket preMessageHandlePacket;
}tsSocketHandleMap;
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
teSS_Status eSocketServer_Init(void);
teSS_Status eSocketServer_Destroy(void);
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

