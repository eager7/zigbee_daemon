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
#include "zigbee_devices.h"
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
            INF_vPrintln(DBG_BUTTON, "key value:[%d][%d]", btn.state, btn.value);
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
                        ralink_gpio_led_info led_info;
                        led_info.gpio = LED_ZIGBEE;
                        led_info.on = 1;
                        led_info.off = 1;
                        led_info.blinks = 1;
                        led_info.rests = 0;
                        led_info.times = 60;
                        ioctl(iButtonFd, RALINK_GPIO_LED_SET, &led_info);
                        eZigbee_SetPermitJoining(60);
                    }break;
                        default:break;
                }
            }
        } else{
            WAR_vPrintln(T_TRUE, "can't read key value");
        }
    }
    return;
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
int iButtonInitialize()
{
    int val = 0;
    const char *device_cmd = "/etc/init.d/button_mknod.sh";
    if(access(DEV,F_OK) < 0){
        system(device_cmd);
    }
    if(access(DEV,F_OK) < 0){
        WAR_vPrintln(1, "Can't mknod /dev/gpio");
        return -1;
    }
    iButtonFd = open(DEV,O_RDWR);
    if(iButtonFd < 0){
        ERR_vPrintln(1, "can't open /dev/gpio, return %d, err:%s\n", iButtonFd, strerror(errno));
        return -1;
    }
    ioctl(iButtonFd, RALINK_GPIO_INIT, &val);
    /** LED */
    val = 25;
    ioctl(iButtonFd, RALINK_GPIO_SET_DIR_OUT, &val);
    ralink_gpio_led_info led_info;
    led_info.gpio = LED_ZIGBEE;
    led_info.blinks = 10;
    led_info.times = 10;
    ioctl(iButtonFd, RALINK_GPIO_LED_SET, &led_info);

    /** GPIO */
    val = BUTTON_SW2;
    ioctl(iButtonFd, RALINK_GPIO_SET_DIR_IN, &val);
    val = BUTTON_SW3;
    ioctl(iButtonFd, RALINK_GPIO_SET_DIR_IN, &val);
    val = BUTTON_SW4;
    ioctl(iButtonFd, RALINK_GPIO_SET_DIR_IN, &val);

    //register my information
    ralink_gpio_reg_info info;
    info.pid = getpid();
    info.irq = BUTTON_SW2;
    if (ioctl(iButtonFd, RALINK_GPIO_REG_IRQ, &info) < 0) {
        perror("ioctl");
        close(iButtonFd);
        return -1;
    }
    info.irq = BUTTON_SW3;
    if (ioctl(iButtonFd, RALINK_GPIO_REG_IRQ, &info) < 0) {
        perror("ioctl");
        close(iButtonFd);
        return -1;
    }
    info.irq = BUTTON_SW4;
    if (ioctl(iButtonFd, RALINK_GPIO_REG_IRQ, &info) < 0) {
        perror("ioctl");
        close(iButtonFd);
        return -1;
    }

    //enable gpio interrupt
    printf("enable the button interrupt\n");
    if (ioctl(iButtonFd, RALINK_GPIO_ENABLE_INTP) < 0) {
        perror("ioctl");
        close(iButtonFd);
        return -1;
    }
    signal(SIGUSR2, vButtonSignalHandler);

    return 0;
}

int iButtonFinished()
{
    int val = 0;
    ioctl(iButtonFd, RALINK_GPIO_DISABLE_INTP, &val);
    close(iButtonFd);
    return 0;
}
