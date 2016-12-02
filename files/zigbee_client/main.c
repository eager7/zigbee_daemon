/****************************************************************************
 *
 * MODULE:             main.c
 *
 * COMPONENT:          Utils interface
 *
 * REVISION:           $Revision:  0$
 *
 * DATED:              $Date: 2015-10-21 15:13:17 +0100 (Thu, 21 Oct 2015 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com B.V. 2015. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include "utils.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "socket_client.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_MAIN 1
#define ZIGBEE_LEAVE
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void SignalHandler();

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const char *psVersion = "Version 0.1";
const char *msg_premit = "{\"command\":5,\"sequence\":0,\"time\":100}";
const char *msg_on =  "{\"command\":6,\"sequence\":6,\"device_address\":\"6066005655905860\",\"group_id\":0}";
const char *msg_off = "{\"command\":7,\"sequence\":7,\"device_address\":\"6066005655905860\",\"group_id\":0}";
const char *msg_get = "{\"command\":8,\"sequence\":8,\"device_address\":\"6066005655905860\"}";
const char *msg_leave = "{\"command\":21,\"sequence\":8,\"device_address\":\"6066005663930783\"}";
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
int main (int argc, char *argv[])
{
    DBG_vPrintf(DBG_MAIN, "This is a socket comminution client program connect with %s\n", argv[1]);

    if(NULL == argv[1]){
        ERR_vPrintf(T_TRUE, "Please input the ipaddr of server\n");
        exit(1);
    }

    signal(SIGTERM, SignalHandler);
    signal(SIGINT,  SignalHandler);

    teSocketStatus eSocketStatus;
    eSocketStatus = SocketClientInit(6667, argv[1]);
    if(E_SOCK_OK != eSocketStatus)
    {
        ERR_vPrintf(T_TRUE, "SocketClientInit Error %d\n", eSocketStatus);
        return -1;
    }
	sleep(1);
    

    char command[5] = {0};
    while(1){
        BLUE_vPrintf(T_TRUE, "Please input your command:\n");
        BLUE_vPrintf(T_TRUE, "0:exit\n");
        BLUE_vPrintf(T_TRUE, "1:premit join\n");
        BLUE_vPrintf(T_TRUE, "2:leave network\n");
        BLUE_vPrintf(T_TRUE, "3:on\n");
        BLUE_vPrintf(T_TRUE, "4:off\n");
        
        if(NULL == fgets(command, 5, stdin)){
            continue;
        }
        int comm = atoi(command);
        switch(comm){
            case 0:
                DBG_vPrintf(DBG_MAIN, "your command is exit this program\n");
                exit(0);
                break;
            case 1:
                DBG_vPrintf(DBG_MAIN, "your command is add new device\n");
                SocketClientSendMsg(msg_premit);
                break;
            case 2:
                DBG_vPrintf(DBG_MAIN, "your command is make a device leave\n");
                SocketClientSendMsg(msg_leave);
                break;
            case 3:
                DBG_vPrintf(DBG_MAIN, "your command is make a device on\n");
                break;
            case 4:
                DBG_vPrintf(DBG_MAIN, "your command is make a device off\n");
                break;
            default:
                break;
        }
    }
    return 0;
}

static void SignalHandler()
{
    PURPLE_vPrintf(DBG_MAIN, "Receive a Terminal signal, Exit This Program\n");
    SocketClientFinished();
    exit(0);
}
