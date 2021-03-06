/****************************************************************************
 *
 * MODULE:             door lock controller - zigbee daemon
 *
 * COMPONENT:          zigbee devices interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2017-06-29 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2017. All rights reserved
 *
 ***************************************************************************/



#ifndef __DOOR_LOCK_CONTROLLER_H__
#define __DOOR_LOCK_CONTROLLER_H__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdint.h>
#include "utils.h"
#include "serial_link.h"
#include "zigbee_sqlite.h"
#include "zigbee_control_bridge.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


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
** Prototype    : eDoorLockControllerInitialize
** Description  : 初始化门锁控制器设备，为设备添加通用回调函数和门锁控制函数
** Input        : psZigbeeNode, the structure of node
** Output       : none
** Return Value : Return E_ZB_OK

** History      :
** Date         : 2017/2/28
** Author       : PCT
*****************************************************************************/
teZbStatus eDoorLockControllerInitialize(tsZigbeeNodes *psZigbeeNode);
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
