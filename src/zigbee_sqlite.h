/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          SocketServer interface
 *
 * REVISION:           $Revision: 43420 $
 *
 * DATED:              $Date: 2015-10-01 15:13:17 +0100 (Mon, 18 Jun 2012) $
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright Tonly B.V. 2015. All rights reserved
 *
 ***************************************************************************/


#ifndef  ZIGBEE_SQLITE_H_INCLUDED
#define  ZIGBEE_SQLITE_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <netinet/in.h>
#include <sqlite3.h>
#include "utils.h"
#include "list.h"
#include "threads.h"
#include "zigbee_network.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define TABLE_DEVICE    " TABLE_DEVICES "
#define TABLE_ATTRIBUTE " TABLE_ATTRIBUTE "

#define INDEX           " IndexNum "
#define DEVICE_ID       " DeviceID "
#define DEVICE_ADDR     " DeviceAddr "
#define DEVICE_MAC      " DeviceMAC "
#define DEVICE_NAME     " DeviceName "
#define DEVICE_ONLINE   " DeviceOnline "
#define APPENDS_TEXT    " AppendsText "
#define CLUSTER_ID      " ClusterID "
#define DEVICE_MASK     " DeviceMask "
#define ATTRIBUTE_VALUE " AttributeValue "
#define LAST_TIME       " LastTime "

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
    E_SQ_APPENDS_TEXT,

    E_SQ_ATTRIBUTE_VALUE,
    E_SQ_LAST_TIME,
} teSQ_UpdateType;

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
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
extern char *pZigbeeSqlitePath;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

teSQ_Status eZigbeeSqliteInit(char *pZigbeeSqlitePath);
teSQ_Status eZigbeeSqliteFinished(void);
teSQ_Status eZigbeeSqliteOpen(char *pZigbeeSqlitePath);
teSQ_Status eZigbeeSqliteClose(void);
teSQ_Status eZigbeeSqliteCreateNode(uint64 u64MacAddress);

teSQ_Status eZigbeeSqliteAddNewDevice(uint64 u64MacAddress, uint16 u16ShortAddress, uint16 u16DeviceID, char *psDeviceName, char *psAppends);
teSQ_Status eZigbeeSqliteAddNewCluster(uint16 u16ClusterID, uint64 u64DeviceMask, uint16 u16DeviceAttribute, uint64 u64LastTime, char *psAppends);

teSQ_Status eZigbeeSqliteDeleteDevice(uint64 u64MacAddress);

teSQ_Status eZigbeeSqliteUpdateDeviceTable(tsZigbee_Node *psZigbee_Node, teSQ_UpdateType eDeviceType);
teSQ_Status eZigbeeSqliteUpdateAttributeTable(tsZigbee_Node *psZigbee_Node, uint16 u16ClusterID, uint16 u16AttributeValue, teSQ_UpdateType eDeviceType);

bool_t eZigbeeSqliteDeviceIsExist(uint64 u64IEEEAddress);
bool_t eZigbeeSqliteClusterIsExist(uint64 u64DeviceMask, uint16 u16ClusterID);
teSQ_Status eZigbeeSqliteRetrieveDevice(tsZigbee_Node *psZigbee_Node);
teSQ_Status eZigbeeSqliteRetrieveAttribute(uint16 u16ClusterID, uint64 u64DeviceMask, uint16 *u16DeviceAttribute, uint64 *u64LastTime, char *psAppends);
teSQ_Status eZigbeeSqliteRetrieveDevicesList(tsZigbee_Node *psZigbee_Node);
teSQ_Status eZigbeeSqliteRetrieveDevicesListFree(tsZigbee_Node *psZigbee_Node);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ZIGBEE_SQLITE_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

