/****************************************************************************
 *
 * MODULE:             utils.h
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

#ifndef __H_UTILS_H_
#define __H_UTILS_H_

#if defined __cplusplus   
extern "C" {
#endif
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <syslog.h>
#include <json-c/json.h>

/*
#include "list.h"
#include "serial.h"
#include "serial_link.h"
#include "threads.h"
#include "zigbee_type.h"
#include "zigbee_sqlite.h"
#include "zigbee_network.h"
#include "zigbee_devices.h"
#include "zigbee_constant.h"
#include "zigbee_socket_server.h"
*/
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define UI_BLACK(x)     "\e[30;1m"x"\e[0m"
#define UI_RED(x)       "\e[31;1m"x"\e[0m"
#define UI_GREEN(x)     "\e[32;1m"x"\e[0m"
#define UI_YELLOW(x)    "\e[33;1m"x"\e[0m"
#define UI_BLUE(x)      "\e[34;1m"x"\e[0m"
#define UI_PURPLE(x)    "\e[35;1m"x"\e[0m"
#define UI_CYAN(x)      "\e[36;1m"x"\e[0m"
#define UI_WHITE(x)     "\e[37;1m"x"\e[0m"

#define DBG_vPrintf(a,b,ARGS...)  do {  if (a) {printf(UI_BLUE	("[%s_%d]") b, __FILE__, __LINE__, ## ARGS);} \
    /*else {syslog(LOG_DEBUG, "[IOT_%d]" b, __LINE__, ## ARGS);}*/} while(0)
#define INF_vPrintf(a,b,ARGS...)  do {  if (a) {printf(UI_YELLOW("[%s_%d]") b, __FILE__, __LINE__, ## ARGS);} \
    /*else {syslog(LOG_INFO,  "[IOT_%d]" b, __LINE__, ## ARGS);}*/} while(0)
#define NOT_vPrintf(a,b,ARGS...)  do {  if (a) {printf(UI_GREEN	("[%s_%d]") b, __FILE__, __LINE__, ## ARGS);} \
    /*else {syslog(LOG_NOTICE,"[IOT_%d]" b, __LINE__, ## ARGS);}*/} while(0)
#define WAR_vPrintf(a,b,ARGS...)  do {  if (a) {printf(UI_PURPLE("[%s_%d]") b, __FILE__, __LINE__, ## ARGS);} \
    /*else {syslog(LOG_WARNING, "[IOT_%d]" b, __LINE__, ## ARGS);}*/} while(0)
#define ERR_vPrintf(a,b,ARGS...)  do {  if (a) {printf(UI_RED	("[%s:%d]") b, __FILE__, __LINE__, ## ARGS);} \
    /*syslog(LOG_ERR,  "[IOT_%s:%d]" b, __FILE__, __LINE__, ## ARGS);*/} while(0)

#define MIBF    256
#define MDBF    1024
#define MABF    2048
#define PACKED __attribute__((__packed__))


//User Define
#define PCT_DBG_CONTROL 
#define DBG_PCT  1
#define END_DEVICE
//#define ZIGBEE_SQLITE

#define OTA_SERVER
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

typedef enum
{
    T_FALSE = 0,
    T_TRUE  = 1,    
} bool_t;

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
/****************************************************************************/
/***        Locate   Functions                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /*__H_UTILS_H_*/
