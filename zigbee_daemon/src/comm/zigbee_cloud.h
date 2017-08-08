/****************************************************************************
 *
 * MODULE:             zigbee - zigbee daemon
 *
 * COMPONENT:          zigbee cloud interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2016-12-22 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2016. All rights reserved
 *
 ***************************************************************************/



#ifndef __ZIGBEE_CLOUD_H__
#define __ZIGBEE_CLOUD_H__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h> 
#include <stdlib.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include "mthread.h"
#include "zigbee_socket.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define CLOUD_ADDRESS "127.0.0.1"
#define CLOUD_SUB_PORT "5555"
#define CLOUD_PUSH_PORT "5556"

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_CLOUD_OK,
    E_CLOUD_ERROR,
}teCloudStatus;

typedef struct
{
    tsThread    sThreadCloud;
}tsZigbeeCloud;

typedef enum
{
    E_CLOUD_EVENT_DEVICES_REPORT      =   0x0001,
    
} tsCloudEvent;
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
teCloudStatus eZigbeeCloudInit();
teCloudStatus eZigbeeCloudFinished();
teCloudStatus eZigbeeCloudPush(const char *pmsg, uint16 u16Length);
teCloudStatus eCloudPushAllDevicesList(void);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
#if defined __cplusplus
}
#endif

#endif /* __SERIAL_H__ */
