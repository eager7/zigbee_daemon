/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          SocketServer interface
 *
 * REVISION:           $Revision: 43420 $
 *
 * DATED:              $Date: 2015-10-01 15:13:17 +0100 (Mon, 18 Jun 2012) $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright Tonly B.V. 2015. All rights reserved
 *
 ***************************************************************************/


#ifndef  SOCKET_SERVER_H_INCLUDED
#define  SOCKET_SERVER_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "Threads.h"
#include <netinet/in.h>


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
    //Devices
    E_SS_COMMAND_GET_DEVICES_LIST_ONLINE = 0,
    E_SS_COMMAND_GET_DEVICES_LIST_ALL,
    E_SS_COMMAND_GET_DEVICE_NAME,
    E_SS_COMMAND_GET_DEVICE_ATTRIBUTE,
    E_SS_COMMAND_SET_DEVICE_ATTRIBUTE,

    //CoorDinator
    E_SS_COMMAND_PREMITJOIN,//5
    
    //ZLL
    E_SS_COMMAND_LIGHT_ON,
    E_SS_COMMAND_LIGHT_OFF,
    E_SS_COMMAND_LIGHT_GET_STATUS,
    E_SS_COMMAND_LIGHT_SET_RGB,
    E_SS_COMMAND_LIGHT_GET_RGB,//10
    E_SS_COMMAND_LIGHT_SET_LEVEL,
    E_SS_COMMAND_LIGHT_GET_LEVEL,

    //Sensor
    E_SS_COMMAND_SENSOR_GET_HUMI,
    E_SS_COMMAND_SENSOR_GET_TEMP,
    E_SS_COMMAND_SENSOR_GET_ILLU,//15
    E_SS_COMMAND_SENSOR_GET_SIMPLE,
    E_SS_COMMAND_SENSOR_GET_POWER,
    E_SS_COMMAND_SENSOR_GET_ALARM,
    E_SS_COMMAND_SENSOR_GET_ALL_ALARM,

    E_SS_COMMAND_GETVERSION,//20
    
    //Leave Network
    E_SS_COMMAND_LEAVE_NETWORK,
    
    //OTA & Upgrade
    E_SS_COMMAND_COORDINATOR_UPGRADE,

    
}teSocketCommand;

typedef enum 
{
    SUCCESS,
    FAILED,
}teJsonStatus;

typedef enum 
{
    E_SENSOR_TEMP,
    E_SENSOR_HUMI,
    E_SENSOR_SIMPLE,
    E_SENSOR_ILLU,
    E_SENSOR_POWER,
}teSensorType;

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/



/** Structure of data for the serial link */
typedef struct
{
    int     iSocketFd;

    //pthread_mutex_t         mutex;
    
    tsThread sSocketServer;
    tsThread sSocketQueue;
} tsSocketServer;

typedef struct _clientSocket
{
    int iSocketClient;
    char sClientName[MIBF];
    struct sockaddr_in addrclint;
    uint16 u16Length;
    char sClientData[MABF];
}tsClientSocket;

/** Structure allocated and passed to callback handler thread */
typedef struct
{
    int iSocketClientfd;
    uint16 u16Length;
    uint8  au8Message[MABF];
} tsSSCallbackThreadData;

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

