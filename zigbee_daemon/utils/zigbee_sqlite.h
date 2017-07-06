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



#ifndef __ZIGBEE_SQLITE_H__
#define __ZIGBEE_SQLITE_H__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdint.h>
#include "utils.h"
#include "serial_link.h"
#include "zigbee_node.h"
#include "door_lock.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/**
 * 设备数据库关键词
 * */
#define TABLE_DEVICE    " TABLE_DEVICES "

#define INDEX               " IndexNum "
#define DEVICE_ID           " DeviceID "
#define DEVICE_ADDR         " DeviceAddr "
#define DEVICE_MAC          " DeviceMAC "
#define DEVICE_NAME         " DeviceName "
#define DEVICE_ONLINE       " DeviceOnline "
#define DEVICE_CAPABILITY   " DeviceCapability "
/**
 * 门锁数据库关键词
 * */
#define TABLE_USER      "TABLE_USER"

#define USER_ID             " UserID "
#define USER_NAME           " UserName "
#define USER_TYPE           " UserType "
#define USER_PERM           " UserPerm "

#define TABLE_PASSWORD  "TABLE_PASSWD"

#define PASSWD_ID           " PasswdID "
#define PASSWD_WORK         " Worked "
#define PASSWD_AVAILABLE    " Available "
#define PASSWD_START_TIME   " StartTime "
#define PASSWD_END_TIME     " EndTime "
#define PASSWD_LEN          " PasswdLen "
#define PASSWD_DATA         " Password "

#define TABLE_RECORD    "TABLE_RECORD"

#define RECORD_TYPE         " RecordType "
#define RECORD_USER         " UserID "
#define RECORD_TIME         " RecordTime "
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_SQ_OK,
    E_SQ_ERROR,
    E_SQ_OPEN_ERROR,
    E_SQ_ERROR_COMMAND, 
    E_SQ_NO_FOUND,
    E_SQ_NO_FOUND_NODE,
    E_SQ_NO_FOUND_TABLE,
    E_SQ_NO_FOUND_ENDPOINT,
    E_SQ_NO_FOUND_CLUSTER,
    E_SQ_NO_FOUND_ATTRIBUTE,
} teSQ_Status;

typedef enum
{
    E_SQ_UPDATE_ALARM,
    E_SQ_UPDATE_NAME,
}teUpdateMode;

typedef enum 
{
    E_SQ_DEVICE_NAME,
    E_SQ_DEVICE_ADDR,
    E_SQ_DEVICE_ID,
    E_SQ_DEVICE_ONLINE,
} teSQ_UpdateType;

typedef struct _tsZigbeeSqlite
{
    sqlite3 *psZgbeeDB;
    pthread_mutex_t mutex;
}tsZigbeeSqlite;

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
** Prototype    : eZigbeeSqliteInit
** Description  : Open the sqlite3 database
** Input        : pZigbeeSqlitePath, the path of database file
** Output       : None
** Return Value : return E_SQ_OK if successful, else return E_SQ_ERROR

** History      :
** Date         : 2017/6/23
** Author       : PCT
*****************************************************************************/
teSQ_Status eZigbeeSqliteInit(char *pZigbeeSqlitePath);
/*****************************************************************************
** Prototype    : eZigbeeSqliteFinished
** Description  : close the sqlite3 database
** Input        : None
** Output       : None
** Return Value : Return E_SQ_OK

** History      :
** Date         : 2017/6/23
** Author       : PCT
*****************************************************************************/
teSQ_Status eZigbeeSqliteFinished(void);
teSQ_Status eZigbeeSqliteRetrieveDevicesList(tsZigbeeBase *psZigbee_Node);
teSQ_Status eZigbeeSqliteRetrieveDevicesListFree(tsZigbeeBase *psZigbee_Node);
teSQ_Status eZigbeeSqliteUpdateDeviceTable(tsZigbeeBase *psZigbee_Node, teSQ_UpdateType eDeviceType);
teSQ_Status eZigbeeSqliteAddNewDevice(uint64 u64MacAddress, uint16 u16ShortAddress, uint16 u16DeviceID, char *psDeviceName, uint8 u8Capability);
/*****************************************************************************
** Prototype    : eZigbeeSqliteAddDoorLockUser
** Description  : 添加门锁用户
** Input        : u8UserID,u8UserType,u8UserPerm,psUserName此用户的ID，类型，权限
** Output       : None
** Return Value : Return E_SQ_OK

** History      :
** Date         : 2017/6/23
** Author       : PCT
*****************************************************************************/
teSQ_Status eZigbeeSqliteAddDoorLockUser(uint8 u8UserID, uint8 u8UserType, uint8 u8UserPerm, char *psUserName);
/*****************************************************************************
** Prototype    : eZigbeeSqliteAddDoorLockRecord
** Description  : 添加开锁记录，时间为time函数返回的秒数
** Input        : u8Type 开锁的形式，包括密码，指纹，临时密码，钥匙等
** Output       : None
** Return Value : Return E_SQ_OK

** History      :
** Date         : 2017/6/23
** Author       : PCT
*****************************************************************************/
teSQ_Status eZigbeeSqliteAddDoorLockRecord(uint8 u8Type, uint8 u8UserID, uint64 u64Time);
/*****************************************************************************
** Prototype    : eZigbeeSqliteAddDoorLockPassword
** Description  : 添加一个临时密码
** Input        : u8PasswordID，密码的ID
 *                u8Available，密码可用次数
 *                u64StartTime，密码起始时间，值为1970年至今的秒数
 *                u64EndTime，密码结束时间，值为1970年至今的秒数
 *                u8PasswordLen，密码长度
 *                psPassword，密码值
** Output       : None
** Return Value : Return E_SQ_OK

** History      :
** Date         : 2017/6/23
** Author       : PCT
*****************************************************************************/
teSQ_Status eZigbeeSqliteAddDoorLockPassword(uint8 u8PasswordID, uint8 u8Worked, uint8 u8Available, uint64 u64StartTime,
                                             uint64 u64EndTime, uint8 u8PasswordLen, const char *psPassword);
teSQ_Status eZigbeeSqliteDelDoorLockPassword(uint8 u8PasswordID);
teSQ_Status eZigbeeSqliteUpdateDoorLockPassword(uint8 u8PasswordID, uint8 u8Available, uint8 u8Worked);
teSQ_Status eZigbeeSqliteDoorLockRetrievePassword(uint8 u8PasswordID, tsTemporaryPassword *psPassword);
teSQ_Status eZigbeeSqliteDoorLockRetrievePasswordList(tsTemporaryPassword *psPasswordHeader);
teSQ_Status eZigbeeSqliteDoorLockRetrievePasswordListFree(tsTemporaryPassword *psPasswordHeader);

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
