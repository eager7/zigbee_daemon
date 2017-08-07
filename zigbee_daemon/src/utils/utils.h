/****************************************************************************
 *
 * MODULE:             utils.h
 *
 * COMPONENT:          Utils interface
 *
 * REVISION:           $Revision:  1.0$
 *
 * DATED:              $Date: 2016-12-06 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2016. All rights reserved
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

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define mLogInitSetPid(pid)    openlog(pid, LOG_PID|LOG_CONS, LOG_USER)
#define DBG_vPrintln(a,b,ARGS...)  \
    do {if (a) {if(daemonize){syslog(LOG_DEBUG|LOG_USER, "[%d]" b, __LINE__, ## ARGS);} else {printf(("\e[34;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);}}} while(0)
#define INF_vPrintln(a,b,ARGS...)  \
    do {if (a) {if(daemonize){syslog(LOG_DEBUG|LOG_USER, "[%d]" b, __LINE__, ## ARGS);} else {printf(("\e[33;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);}}} while(0)
#define NOT_vPrintln(a,b,ARGS...)  \
    do {if (a) {if(daemonize){syslog(LOG_DEBUG|LOG_USER, "[%d]" b, __LINE__, ## ARGS);} else {printf(("\e[32;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);}}} while(0)
#define WAR_vPrintln(a,b,ARGS...)  \
    do {if (a) {if(daemonize){syslog(LOG_DEBUG|LOG_USER, "[%d]" b, __LINE__, ## ARGS);} else {printf(("\e[35;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);}}} while(0)
#define ERR_vPrintln(a,b,ARGS...)  \
    do {if (a) {if(daemonize){syslog(LOG_DEBUG|LOG_USER, "[%d]" b, __LINE__, ## ARGS);} else {printf(("\e[31;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);}}} while(0)
#define PERR_vPrintln(x) ERR_vPrintln(1,x ":%s", strerror(errno))

#define MIBF    256
#define MDBF    1024
#define MABF    2048
#define PACKED __attribute__((__packed__))

#define CHECK_RESULT(fun,value,ret) do{ if(value!=fun)return ret;}while(0)
#define CHECK_STATUS(fun,value,ret) do{ if(value!=fun){ERR_vPrintln(T_TRUE, "Error:%s\n", strerror(errno));return ret;}}while(0)
#define CHECK_POINTER(value,ret) do{ if(value==NULL){ERR_vPrintln(T_TRUE, "Pointer is NULL\n");return ret;}}while(0)
#define FREE(p) do{ if(p){free(p); p=NULL;} }while(0)
#define JSON_FREE(p) do{ if(p){json_object_put(p); p=NULL;} }while(0)

#define Swap64(ll) (((ll) >> 56) |                          \
                    (((ll) & 0x00ff000000000000) >> 40) |   \
                    (((ll) & 0x0000ff0000000000) >> 24) |   \
                    (((ll) & 0x000000ff00000000) >> 8)  |   \
                    (((ll) & 0x00000000ff000000) << 8)  |   \
                    (((ll) & 0x0000000000ff0000) << 24) |   \
                    (((ll) & 0x000000000000ff00) << 40) |   \
                    (((ll) << 56)))  


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
extern int daemonize;
extern int verbosity;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local    Functions                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /*__H_UTILS_H_*/
