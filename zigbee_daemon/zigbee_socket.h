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

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum
{
    E_SS_OK,
    E_SS_ERROR,
    E_SS_ERROR_COMMAND,
    E_SS_ERROR_JSON_FORMAT,
    E_SS_ERROR_NO_COMMAND,
    E_SS_ERROR_MESSAGE,
    
    E_SS_ERROR_NOMEM,
    E_SS_ERROR_SOCKET,
    E_SS_ERROR_BIND,
    E_SS_ERROR_LISTEN,
    E_SS_NOMESSAGE,
    
} teSS_Status;

typedef enum
{
    /* Manage */
    E_SS_COMMAND_GETVERSION                 = 0x0000,
    E_SS_COMMAND_PREMITJOIN                 = 0x0001,
    E_SS_COMMAND_LEAVE_NETWORK              = 0x0002,
    E_SS_COMMAND_GET_CHANNEL                = 0x0003,
    /* Devices */
    E_SS_COMMAND_GET_DEVICES_LIST_ONLINE    = 0x0010,
    E_SS_COMMAND_GET_DEVICES_LIST_ALL       = 0x0011,
    E_SS_COMMAND_GET_DEVICE_NAME            = 0x0012,
    E_SS_COMMAND_GET_DEVICE_ATTRIBUTE       = 0x0013,
    E_SS_COMMAND_SET_DEVICE_ATTRIBUTE       = 0x0014,
    /* ZLL */
    E_SS_COMMAND_LIGHT_SET_ON_OFF           = 0x0020,
    E_SS_COMMAND_LIGHT_SET_LEVEL            = 0x0021,
    E_SS_COMMAND_LIGHT_SET_RGB              = 0x0022,
    E_SS_COMMAND_LIGHT_GET_STATUS           = 0x0023,
    E_SS_COMMAND_LIGHT_GET_LEVEL            = 0x0024,
    E_SS_COMMAND_LIGHT_GET_RGB              = 0x0025,
    /* Sensor */
    E_SS_COMMAND_SENSOR_GET_SENSOR          = 0x0030,
    E_SS_COMMAND_SENSOR_GET_ALARM           = 0x0031,
    E_SS_COMMAND_SENSOR_GET_ALL_ALARM       = 0x0032,

    /* Closure */
    E_SS_COMMAND_SET_CLOSURES_STATE         = 0x0040,    
    //OTA & Upgrade
    E_SS_COMMAND_COORDINATOR_UPGRADE        = 0x0050,

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

