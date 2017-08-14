/****************************************************************************
 *
 * MODULE:             zigbee - zigbee daemon
 *
 * COMPONENT:          zigbee sqlite interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2017-06-23 15:13:17  $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2016. All rights reserved
 *
 ***************************************************************************/



#ifndef __ZIGBEE_BUTTON_H__
#define __ZIGBEE_BUTTON_H__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "utils.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DEV "/dev/gpio"
#define LED_ZIGBEE 25
#define LED_APP    27
#define BUTTON_SW2 28
#define BUTTON_SW3 24
#define BUTTON_SW4 22

#define SHORT_KEY 0x01
#define LONG_KEY 0x02

/*
 * ioctl commands
 */
#define	RALINK_GPIO_SET_DIR		0x01
#define RALINK_GPIO_SET_DIR_IN		0x11
#define RALINK_GPIO_SET_DIR_OUT		0x12
#define	RALINK_GPIO_READ		0x02
#define	RALINK_GPIO_WRITE		0x03
#define	RALINK_GPIO_SET			0x21
#define	RALINK_GPIO_CLEAR		0x31
#define	RALINK_GPIO_READ_INT		0x02 //same as read
#define	RALINK_GPIO_WRITE_INT		0x03 //same as write
#define	RALINK_GPIO_SET_INT		0x21 //same as set
#define	RALINK_GPIO_CLEAR_INT		0x31 //same as clear
#define RALINK_GPIO_ENABLE_INTP		0x08
#define RALINK_GPIO_DISABLE_INTP	0x09
#define RALINK_GPIO_REG_IRQ		0x0A
#define RALINK_GPIO_LED_SET		0x41
#define RALINK_GPIO_INIT		0x42 //PCT
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct _btn_control{
    unsigned char state;    //0x01,short press; 0x02,long press
    unsigned char value;
} btn_control_t;
/** structure used at regsitration */
typedef struct {
    unsigned int irq;		//request irq pin number
    pid_t pid;			    //process id to notify
} ralink_gpio_reg_info;

typedef struct {
    int gpio;			    //gpio number (0 ~ 23)
    unsigned int on;		//interval of led on
    unsigned int off;		//interval of led off
    unsigned int blinks;    //number of blinking cycles
    unsigned int rests;		//number of break cycles
    unsigned int times;		//blinking times
} ralink_gpio_led_info;
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
/*****************************************************************************
** Prototype    : iButtonInitialize
** Description  : Open the sqlite3 database
** Input        : pZigbeeSqlitePath, the path of database file
** Output       : None
** Return Value : return E_SQ_OK if successful, else return E_SQ_ERROR

** History      :
** Date         : 2017/6/23
** Author       : PCT
*****************************************************************************/
int iButtonInitialize();
int iButtonFinished();

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
#if defined __cplusplus
}
#endif

#endif /* __SERIAL_H__ */
