/****************************************************************************
 *
 * MODULE:             zigbee - zigbee daemon
 *
 * COMPONENT:          zigbee discovery interface
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



#ifndef __ZIGBEE_DISCOVERY_H__
#define __ZIGBEE_DISCOVERY_H__

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
#include "utils.h"
#include "mthread.h"
#include "zigbee_socket.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define BROADCAST_ADDRESS "255.255.255.255"
#define BROADCAST_PORT 6789

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_DISCOVERY_OK,
    E_DISCOVERY_ERROR,
    E_DISCOVERY_ERROR_MALLOC,
    E_DISCOVERY_ERROR_PARAM,
    E_DISCOVERY_ERROR_FORMAT,
    E_DISCOVERY_ERROR_PTHREAD_CREATE,
    E_DISCOVERY_ERROR_CREATESOCK,
    E_DISCOVERY_ERROR_SETSOCK,
    E_DISCOVERY_ERROR_BIND,
}teDiscStatus;

typedef struct
{
    int                             iSocketFd;
    struct sockaddr_in              server_addr;  
    tsThread                        sThreadDiscovery;
}tsZigbeeDiscovery;

typedef enum
{
    E_BROADCAST_EVENT_ATTRIBUTE_REPORT      =   0x0001,
    
} tsBroadcastEvent;
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
teDiscStatus eZigbeeDiscoveryInit();
teDiscStatus eZigbeeDiscoveryFinished();
teDiscStatus eZigbeeDiscoveryBroadcastEvent(uint8 *pu8Msg, tsBroadcastEvent sEvent);

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
