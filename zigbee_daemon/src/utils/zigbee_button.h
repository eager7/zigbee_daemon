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
#include <stdint.h>
#include "utils.h"
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define BUTTON_SW2 0x02
#define BUTTON_SW3 0x03
#define BUTTON_SW4 0x04

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
} button_driver_e;

typedef enum {
    E_LED2_ON,
    E_LED2_OFF,
    E_LED2_FLASH,
    E_LED3_ON,
    E_LED3_OFF,
    E_LED3_FLASH,
} led_cmd_e;
typedef struct _btn_control{
    unsigned char state;//0x01,short press; 0x02,long press
    unsigned char value;
} btn_control_t;
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
int iLedControl(led_cmd_e cmd, uint8 time);

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
