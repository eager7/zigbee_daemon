/****************************************************************************
 *
 * MODULE:             zigbee - zigbee daemon
 *
 * COMPONENT:          zigbee sqlite interface
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2016-12-02 15:13:17 +0100 (Fri, 12 Dec 2016 $
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

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define TABLE_DEVICE    " TABLE_DEVICES "

#define INDEX           " IndexNum "
#define DEVICE_ID       " DeviceID "
#define DEVICE_ADDR     " DeviceAddr "
#define DEVICE_MAC      " DeviceMAC "
#define DEVICE_NAME     " DeviceName "
#define DEVICE_ONLINE   " DeviceOnline "
#define DEVICE_CAPABILITY   " DeviceCapability "

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
teSQ_Status eZigbeeSqliteOpen(char *pZigbeeSqlitePath);
teSQ_Status eZigbeeSqliteClose(void);
teSQ_Status eZigbeeSqliteInit(char *pZigbeeSqlitePath);
teSQ_Status eZigbeeSqliteFinished(void);
teSQ_Status eZigbeeSqliteRetrieveDevicesList(tsZigbeeBase *psZigbee_Node);
teSQ_Status eZigbeeSqliteRetrieveDevicesListFree(tsZigbeeBase *psZigbee_Node);
teSQ_Status eZigbeeSqliteUpdateDeviceTable(tsZigbeeBase *psZigbee_Node, teSQ_UpdateType eDeviceType);
teSQ_Status eZigbeeSqliteAddNewDevice(uint64 u64MacAddress, uint16 u16ShortAddress, uint16 u16DeviceID, char *psDeviceName, uint8 u8Capability);

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
