/****************************************************************************
 *
 * MODULE:             button_driver.h
 *
 * COMPONENT:          Key interface
 *
 * REVISION:           $Revision:  1.0$
 *
 * DATED:              $Date: 2017-07-28 15:13:17 +0100 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2017. All rights reserved
 *
 ***************************************************************************/

#ifndef __H_BUTTON_DRIVER_H_
#define __H_BUTTON_DRIVER_H_

#if defined __cplusplus   
extern "C" {
#endif
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <asm/uaccess.h>
//#include "../linux-3.10.14/drivers/char/ralink_gpio.h"
#include <asm/rt2880/rt_mmap.h>
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define GPIO_MAJOR 0	//major version
#define GPIO_MINOR 0

#define GPIO_NUM 1
#define GPIO_NAME "button_driver"

//////////////////////////REGISTER ADDRESS////////////////////////////
#define RALINK_SYSCTL_ADDR		RALINK_SYSCTL_BASE	// system control
#define RALINK_REG_GPIOMODE		(RALINK_SYSCTL_ADDR + 0x60)
#define RALINK_AGPIO_CFG		(RALINK_SYSCTL_ADDR + 0x3C)
#define RALINK_REG_GPIOMODE2	(RALINK_SYSCTL_ADDR + 0x64)

#define RALINK_IRQ_ADDR			RALINK_INTCL_BASE
#define RALINK_REG_INTDIS		(RALINK_IRQ_ADDR   + 0x78)
#define RALINK_REG_INTENA		(RALINK_IRQ_ADDR   + 0x80)

#define RALINK_PRGIO_ADDR		RALINK_PIO_BASE // Programmable I/O
#define RALINK_REG_PIODIR		(RALINK_PRGIO_ADDR + 0x00)
#define RALINK_REG_PIOEDGE		(RALINK_PRGIO_ADDR + 0xA0)
#define RALINK_REG_PIODATA		(RALINK_PRGIO_ADDR + 0x20)
#define RALINK_REG_PIORENA		(RALINK_PRGIO_ADDR + 0x50)
#define RALINK_REG_PIOFENA		(RALINK_PRGIO_ADDR + 0x60)
#define RALINK_REG_PIOINT		(RALINK_PRGIO_ADDR + 0x90)

#define BUTTON_SW2 0x02
#define BUTTON_SW3 0x03
#define BUTTON_SW4 0x04
#define KEY2 28
#define KEY3 24
#define KEY4 22
#define KEYS ((1<<KEY2)|(1<<KEY3)|(1<<KEY4))

#define SHORT_KEY 0x01
#define LONG_KEY 0x02
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum {
    E_GPIO_DRIVER_LED2_CONTROL = 0x01,
    E_GPIO_DRIVER_LED3_CONTROL,
    E_GPIO_DRIVER_LED2_FLSAH,
    E_GPIO_DRIVER_LED3_FLSAH,
    E_GPIO_DRIVER_STOP_FLSAH,
    E_GPIO_DRIVER_ENABLE_KEY_INTERUPT,
    E_GPIO_DRIVER_DISABLE_KEY_INTERUPT,

    E_GPIO_DRIVER_INIT,
}button_driver_e;

typedef struct _led_control{
    struct timer_list timer_led;
    bool bLedState;
    unsigned int timer_times;
}led_control_t;

typedef struct _btn_control{
    unsigned char state;//0x01,short press; 0x02,long press
    unsigned char value;
} btn_control_t;

typedef struct _button_driver_t{
    int driver_major;
    pid_t pid;
	struct cdev device; //驱动设备在内核中以inode的形式存在，字符类设备使用cdev结构体
    led_control_t led2;
    led_control_t led3;
    btn_control_t btn;
}button_driver_t;
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
/***        Local    Functions                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /*__H_BUTTON_DRIVER_H_*/
