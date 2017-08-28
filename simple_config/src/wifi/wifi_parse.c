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
#include <unistd.h>

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
static int wifi_ap_to_station()
{
    DBG_vPrintln(DBG_WIFI, "wifi_ap_to_station");

    system("ifconfig ra0 down");
    system("rmmod mt7628");
    system("insmod /lib/modules/3.10.14/mt7628sta.ko");
    system("ifconfig rai0 up");
    system("iwlist rai0 scanning");

    return 0;
}

static int wifi_station_to_ap()
{
    system("ifconfig rai0 down");
    system("rmmod mt7628sta");
    system("insmod /lib/modules/3.10.14/mt7628.ko");
    system("/etc/init.d/wireless.sh ap");
    system("ifconfig ra0 up");
    system("wifi");

    return 0;
}


static int wifi_connect_ssid(const char *ssid, const char *key)
{
    char cmd_ssid_info[MIBF] = {0};
    snprintf(cmd_ssid_info, sizeof(cmd_ssid_info), "iwpriv rai0 get_site_survey | grep %s | awk '{print $4}'", ssid);

    FILE *fp;
    char result[MIBF] = {0};
    fp = popen(cmd_ssid_info, "r");
    fgets(result, sizeof(result), fp);
    DBG_vPrintln(DBG_WIFI, "get ssid encryption info:%s", result);
    pclose(fp);

    system("/etc/init.d/wireless.sh sta");
    if(strstr(result, "WPA2PSK")){
        system("echo \"        option encryption    psk2\" >> /etc/config/wireless");
    } else if(strstr(result, "NONE")){
        system("echo \"        option encryption    none\"  >> /etc/config/wireless");
    } else if(strstr(result, "WPA1PSK")){
        system("echo \"        option encryption    psk\"  >> /etc/config/wireless");
    }

    memset(result, 0, sizeof(result));
    snprintf(result, sizeof(result), "echo \"        option ssid %s\"  >> /etc/config/wireless", ssid);
    system(result);
    memset(result, 0, sizeof(result));
    snprintf(result, sizeof(result), "echo \"        option key %s\"  >> /etc/config/wireless", key);
    system(result);

    system("wifi");//reset wifi
    system("/etc/init.d/wireless.sh dhcp");
    return 0;
}
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
    if(-1 == len){
        ERR_vPrintln(T_TRUE, "receive socket message failed");
        return -1;
    }
    struct json_object *json_message = json_tokener_parse((const char*)buf_cmd);
    if(NULL == json_message){
        ERR_vPrintln(T_TRUE, "json format error");
        return -1;
    }
    struct json_object *json_cmd = NULL;
    if(!json_object_object_get_ex(json_message, "type", &json_cmd)){
        ERR_vPrintln(T_TRUE, "can't parse the message");
        return -1;
    }
    int cmd = json_object_get_int(json_cmd);
    if(0x0011 == cmd){
        DBG_vPrintln(DBG_WIFI, "set wifi from ap to station");
        struct json_object *json_ssid = NULL;
        struct json_object *json_key = NULL;
        if(json_object_object_get_ex(json_message, "ssid", &json_ssid)){
            DBG_vPrintln(DBG_WIFI, "get ssid:%s", json_object_get_string(json_ssid));
        }
        if(json_object_object_get_ex(json_message, "key", &json_key)){
            DBG_vPrintln(DBG_WIFI, "get key:%s", json_object_get_string(json_key));
        }
        wifi_ap_to_station();
        wifi_connect_ssid(json_object_get_string(json_ssid), json_object_get_string(json_key));
    } else if(0x8011 == cmd){
        DBG_vPrintln(DBG_WIFI, "set wifi from station to ap");
        wifi_station_to_ap();
    }

    json_object_put(json_message);
    return 0;
}



