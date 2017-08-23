/****************************************************************************
 *
 * MODULE:             wifi config lib interface
 *
 * COMPONENT:          wifi_parse.h
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2017-08-23 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * MODIFICATION:       $Modification: 2017-08-25
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2016. All rights reserved
 *
 ***************************************************************************/


#ifndef __WIFI_PARSE_H__
#define __WIFI_PARSE_H__

#if defined __cplusplus   
extern "C" {
#endif
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "utils.h"
#include "mthread.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define PORT_AP 7787
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct _wifi_info_t{
    int socket_host;
    int socket_app;
    //tsThread thread;
} wifi_info_t;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
** Prototype    : eThreadStart
** Description  : Start a thread object
** Input        : prThreadFunction, the function of thread
 *                psThreadInfo, the parameter of thread will receive
 *                eDetachState, the run state of thread
** Output       : none
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
int wifi_thread_init();
int wifi_receive_cmd();
int wifi_get_ssid_info();
int wifi_connect();

#if defined __cplusplus
}
#endif

#endif /* __THREADS_H__ */


