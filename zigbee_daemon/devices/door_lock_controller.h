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
#define DOOR_LOCK_PASSWORD_LEN 10
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum {
    E_DOOR_LOCK_USER_TYPE_FIGNER_PRINT = 0x00,
    E_DOOR_LOCK_USER_TYPE_PIN_CODE = 0x01,
    E_DOOR_LOCK_USER_TYPE_RFID = 0x02,
} teDoorLockUserType;

typedef enum {
    E_DOOR_LOCK_USER_PERM_UNRESTRICTED = 0x00,
    E_DOOR_LOCK_USER_YEAR_DAY_SCHEDULE = 0x01,
    E_DOOR_LOCK_USER_WEEK_DAY_SCHEDULE = 0x02,
    E_DOOR_LOCK_USER_MASTER = 0x03,
    E_DOOR_LOCK_USER_NON_ACCESS_USER = 0x04,
} teDoorLockUserPerm;

typedef struct {
    uint8 u8UserID;
    teDoorLockUserType eUserType;
    teDoorLockUserPerm eUserPerm;
} tsDoorLockUser;


typedef struct{
    uint8 u8UserID;
    uint8 u8Available;
    uint8 auTime[20];
    uint8 u8PasswordLen;
    uint8 auPassword[DOOR_LOCK_PASSWORD_LEN];
}tsCLD_DoorLockPayload;
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
** Prototype    : eXXXXInitialize
** Description  : Initialize a device, set the name, address and callback func
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
