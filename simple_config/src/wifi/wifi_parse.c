/****************************************************************************
 *
 * MODULE:             thread lib interface
 *
 * COMPONENT:          mthreads.h
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2016-12-02 15:13:17 +0100 (Fri, 12 Dec 2016 $
 * MODIFICATION:       $Modification: 2017-06-25
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2016. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <json-c/json.h>
#include "wifi_parse.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_WIFI (verbosity >= 3)


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static wifi_info_t wifi_info;
/****************************************************************************/
/***        Local    Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
int wifi_thread_init()
{
    DBG_vPrintln(DBG_WIFI, "start wifi setting");
    signal(SIGPIPE, SIG_IGN);
    memset(&wifi_info, 0, sizeof(wifi_info));
    if(-1 == (wifi_info.socket_host = socket(AF_INET, SOCK_STREAM, 0))){
        ERR_vPrintln(T_TRUE, "socket create failed:%s", strerror(errno));
        return -1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT_AP);

    int on = 1;
    if(setsockopt(wifi_info.socket_host, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        ERR_vPrintln(T_TRUE, "set socket option failed:%s", strerror(errno));
        close(wifi_info.socket_host);
        return -1;
    }

    if(-1 == bind(wifi_info.socket_host, (struct sockaddr *)&server_address, sizeof(server_address))){
        ERR_vPrintln(T_TRUE, "bind failed:%s", strerror(errno));
        close(wifi_info.socket_host);
        return -1;
    }

    if(-1 == listen(wifi_info.socket_host, 5)){
        ERR_vPrintln(T_TRUE, "lister failed:%s", strerror(errno));
        close(wifi_info.socket_host);
        return -1;
    }

    struct sockaddr_in app_address;
    int address_len = sizeof(app_address);
    wifi_info.socket_app = accept(wifi_info.socket_host, (struct sockaddr*)&app_address, (socklen_t*)&address_len);
    if(-1 == wifi_info.socket_app){
        ERR_vPrintln(T_TRUE, "accept failed:%s", strerror(errno));
        close(wifi_info.socket_host);
        return -1;
    }
    close(wifi_info.socket_host);
    wifi_info.socket_host = -1;

    return 0;
}

int wifi_receive_cmd()
{
    uint8 buf_cmd[MIBF] = {0};
    int len = recv(wifi_info.socket_app, buf_cmd, sizeof(buf_cmd), 0);
    struct json_object *json_message = json_tokener_parse((const char*)buf_cmd);
    if(NULL == json_message){
        ERR_vPrintln(T_TRUE, "json format error");
        return -1;
    }
    struct json_object *json_ssid = NULL;
    struct json_object *json_key = NULL;
    if(json_object_object_get_ex(json_message, "ssid", &json_ssid)){
        DBG_vPrintln(DBG_WIFI, "get ssid:%s", json_object_get_string(json_ssid));
    }
    if(json_object_object_get_ex(json_message, "key", &json_key)){
        DBG_vPrintln(DBG_WIFI, "get key:%s", json_object_get_string(json_key));
    }

    json_object_put(json_message);
    return 0;
}

int wifi_get_ssid_info()
{
    
    return 0;
}