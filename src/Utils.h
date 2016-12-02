/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          Utils interface
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

#ifndef __H_UTILS_H_
#define __H_UTILS_H_

#if defined __cplusplus   
extern "C" {
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <syslog.h>
#include <json-c/json.h>
#include "zigbee_type.h"

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

#define PCT_DBG_CONTROL 
#define DBG_PCT  1
#define END_DEVICE
//#define ZIGBEE_SQLITE

#define PACKED __attribute__((__packed__))
#define MIBF    256
#define MDBF    1024
#define MABF    2048

//User Define
#define OTA_SERVER

#if defined __cplusplus
}
#endif

#endif /*__H_UTILS_H_*/
