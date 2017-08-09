/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          Button interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2017-08-02 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2017. All rights reserved
 *
 ***************************************************************************/
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "zigbee_button.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_BUTTON (verbosity >= 4)
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static int iButtonFd;

/****************************************************************************/
/***        Located Functions                                            ***/
/****************************************************************************/
/**
 * Receive signal of SIGUSR2
 * */
static void vButtonSignalHandler (int sig)
{
    DBG_vPrintln(DBG_BUTTON, "Got signal %d\n", sig);
    btn_control_t btn;
    if(sig == SIGUSR2){
        if(-1 != read(iButtonFd, &btn, sizeof(btn))){
            if(btn.state == LONG_KEY){
                switch(btn.value){
                    case BUTTON_SW2:{
                        DBG_vPrintln(DBG_BUTTON, "Got key2\n");

                    }break;
                    case BUTTON_SW3:{
                        DBG_vPrintln(DBG_BUTTON, "Got key3\n");

                    }break;
                    case BUTTON_SW4:{
                        DBG_vPrintln(DBG_BUTTON, "Got key4\n");
                        iLedControl(E_LED2_FLASH, 60);
                    }break;
                        default:break;
                }
            }
        }
    }
    return;
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
int iButtonInitialize()
{
    const char *device_cmd = "/etc/init.d/button_mknod.sh";
    if(access("/dev/button",F_OK) < 0){
        system(device_cmd);
    }
    if(access("/dev/button",F_OK) < 0){
        WAR_vPrintln(T_TRUE, "Can't mknod /dev/button");
        return -1;
    }
    iButtonFd = open("/dev/button",O_RDWR);
    if(iButtonFd < 0){
        ERR_vPrintln(T_TRUE, "can't open /dev/button, return %d, err:%s\n", iButtonFd, strerror(errno));
        return -1;
    }
    sleep(3);
    int val = 0;
    ioctl(iButtonFd, E_GPIO_DRIVER_INIT, &val);
    val = getpid();
    ioctl(iButtonFd, E_GPIO_DRIVER_ENABLE_KEY_INTERUPT, &val);

    signal(SIGUSR2, vButtonSignalHandler);

    return 0;
}

int iButtonFinished()
{
    int val = 0;
    ioctl(iButtonFd, E_GPIO_DRIVER_DISABLE_KEY_INTERUPT, &val);
    close(iButtonFd);
    return 0;
}

int iLedControl(led_cmd_e cmd, uint8 time)
{
    uint64 value = 0;
    if(iButtonFd <= 0){
        WAR_vPrintln(T_TRUE, "Gpio driver not insmod\n");
        return -1;
    }
    switch (cmd){
        case E_LED2_ON:
            value = 1;
            ioctl(iButtonFd, E_GPIO_DRIVER_LED2_CONTROL, &value);
            break;
        case E_LED2_OFF:
            value = 0;
            ioctl(iButtonFd, E_GPIO_DRIVER_LED2_CONTROL, &value);
            break;
        case E_LED2_FLASH:
            ioctl(iButtonFd, E_GPIO_DRIVER_LED2_FLSAH, &time);
            break;
        case E_LED3_ON:
            value = 1;
            ioctl(iButtonFd, E_GPIO_DRIVER_LED3_CONTROL, &value);
            break;
        case E_LED3_OFF:
            value = 0;
            ioctl(iButtonFd, E_GPIO_DRIVER_LED3_CONTROL, &value);
            break;
        case E_LED3_FLASH:
            ioctl(iButtonFd, E_GPIO_DRIVER_LED3_FLSAH, &time);
            break;
        default:
            break;
    }//end of switch
    return 0;
}