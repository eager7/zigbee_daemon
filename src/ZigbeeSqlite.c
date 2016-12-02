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


/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/un.h>


#include "Utils.h"
#include "ZigbeeSqlite.h"
#include "ZigbeeNetwork.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_SQLITE 1

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
teSQ_Status eZigbeeSqliteOpen(char *pZigbeeSqlitePath);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern int verbosity;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
//char *pZigbeeSqlitePath = "/etc/config/ZigbeeDaemon.DB";
//char *pZigbeeSqlitePath = "ZigbeeDaemon.DB";
tsZigbeeSqlite sZigbeeSqlite;

const char *pcDevicesTable = "CREATE TABLE IF NOT EXISTS "TABLE_DEVICE"("
                                INDEX" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                DEVICE_MAC" INTEGER UNIQUE DEFAULT 0, "
                                DEVICE_ADDR" INTEGER DEFAULT 0, "
                                DEVICE_ID" INTEGER DEFAULT 0, "
                                DEVICE_NAME" TEXT NOT NULL, "
                                DEVICE_ONLINE" INTEGER DEFAULT 0, "
                                APPENDS_TEXT" TEXT  DEFAULT NULL);";

const char *pcAttributeTable = "CREATE TABLE IF NOT EXISTS "TABLE_ATTRIBUTE"("
                                INDEX" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                CLUSTER_ID" INTEGER DEFAULT 0, "
                                DEVICE_MASK" INTEGER DEFAULT 0, "
                                ATTRIBUTE_VALUE" INTEGER DEFAULT 0, "
                                LAST_TIME" INTEGER DEFAULT 0, "
                                APPENDS_TEXT" TEXT  DEFAULT NULL);";
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teSQ_Status eZigbeeSqliteInit(char *pZigbeeSqlitePath)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteInit\n");
    memset(&sZigbeeSqlite, 0, sizeof(sZigbeeSqlite));
	pthread_mutex_init(&sZigbeeSqlite.mutex, NULL);
    eZigbeeSqliteOpen(pZigbeeSqlitePath);

    tsZigbee_Node sZigbee_Node;
    memset(&sZigbee_Node, 0, sizeof(sZigbee_Node));
    tsZigbee_Node *Temp = NULL;
    eZigbeeSqliteRetrieveDevicesList(&sZigbee_Node);
    dl_list_for_each(Temp,&sZigbee_Node.list,tsZigbee_Node,list)
    {
        Temp->u8DeviceOnline = 0;
        eZigbeeSqliteUpdateDeviceTable(Temp, E_SQ_DEVICE_ONLINE);
    
    }
    eZigbeeSqliteRetrieveDevicesListFree(&sZigbee_Node);
    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteFinished(void)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteFinished\n");
	pthread_mutex_destroy(&sZigbeeSqlite.mutex);    
    eZigbeeSqliteClose();

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteOpen(char *pZigbeeSqlitePath)
{ 
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteOpen\n");
    if(NULL == pZigbeeSqlitePath)
    {
        ERR_vPrintf(T_TRUE, "Failed to Open SQLITE_DB \n");   
        return E_SQ_ERROR;
    }
    
	if (!access(pZigbeeSqlitePath, 0))
    {
        DBG_vPrintf(DBG_SQLITE, "Open %s \n",pZigbeeSqlitePath);
        if( SQLITE_OK != sqlite3_open( pZigbeeSqlitePath, &sZigbeeSqlite.psZgbeeDB) )
        {
            ERR_vPrintf(T_TRUE, "Failed to Open SQLITE_DB \n");
            return E_SQ_OPEN_ERROR;
        }
    }
	else //Datebase does not existede
	{
        DBG_vPrintf(DBG_SQLITE, "Create A New Database %s \n",pZigbeeSqlitePath);
        if (SQLITE_OK != sqlite3_open_v2(pZigbeeSqlitePath, &sZigbeeSqlite.psZgbeeDB, 
                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL))
        { 
            ERR_vPrintf(T_TRUE, "Error initialising PDM database (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
            return E_SQ_ERROR; 
        }     
        
        DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", pcDevicesTable);
        char *pcErrReturn;      /*Create default table*/
        if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, pcDevicesTable, NULL, NULL, &pcErrReturn))
        {
            ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
            sqlite3_free(pcErrReturn);
            unlink(pZigbeeSqlitePath);
            return E_SQ_ERROR;
        }
        DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", pcAttributeTable);
        if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, pcAttributeTable, NULL, NULL, &pcErrReturn))
        {
            ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
            sqlite3_free(pcErrReturn);
            unlink(pZigbeeSqlitePath);
            return E_SQ_ERROR;
        }
	}
    
    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteClose(void)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteClose\n");
	if (NULL != sZigbeeSqlite.psZgbeeDB)
    {
        sqlite3_close(sZigbeeSqlite.psZgbeeDB);
	}

    return E_SQ_OK;
}

////////////////////////////////////////////////////////***ADD***//////////////////////////////////////////////////////////
teSQ_Status eZigbeeSqliteAddNewDevice(uint64 u64MacAddress, uint16 u16ShortAddress, uint16 u16DeviceID, char *psDeviceName, char *psAppends)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteAddDeviceName\n");
    if((NULL == psDeviceName) || (NULL == psAppends))
    {
        ERR_vPrintf(T_TRUE, "Null pointer\n");
        return E_SQ_ERROR;
    }

    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), 
        "INSERT INTO "TABLE_DEVICE"("DEVICE_MAC","DEVICE_ADDR","DEVICE_ID","DEVICE_NAME","DEVICE_ONLINE","APPENDS_TEXT") VALUES(%llu,%d,%d,'%s',1,'%s')",
        u64MacAddress, u16ShortAddress, u16DeviceID, psDeviceName, psAppends);
    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    char *pcErrReturn;
    if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
    {
        ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
        sqlite3_free(pcErrReturn);
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteAddNewCluster(uint16 u16ClusterID, uint64 u64DeviceMask, uint16 u16DeviceAttribute, uint64 u64LastTime, char *psAppends)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteAddDeviceName\n");
    if(NULL == psAppends)
    {
        ERR_vPrintf(T_TRUE, "Null pointer\n");
        return E_SQ_ERROR;
    }
    if(eZigbeeSqliteClusterIsExist(u64DeviceMask, u16ClusterID))
    {
        DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteClusterIsExist\n");
        return E_SQ_ERROR;        
    }

    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), 
        "INSERT INTO "TABLE_ATTRIBUTE"("CLUSTER_ID","DEVICE_MASK","ATTRIBUTE_VALUE","LAST_TIME","APPENDS_TEXT") VALUES(%d,%llu,%d,%llu,'%s')",
        u16ClusterID, u64DeviceMask, u16DeviceAttribute, u64LastTime, psAppends);
    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    char *pcErrReturn;
    if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
    {
        ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
        sqlite3_free(pcErrReturn);
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

////////////////////////////////////////////////////////***DELETE***//////////////////////////////////////////////////////////
teSQ_Status eZigbeeSqliteDeleteDevice(uint64 u64MacAddress)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteDeleteDevice\n");

    if(u64MacAddress)
    {
        char SqlCommand[MDBF] = {0};
        snprintf(SqlCommand, sizeof(SqlCommand), 
            "Delete From "TABLE_DEVICE" WHERE "DEVICE_MAC"=%llu",u64MacAddress);
        DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
        
        char *pcErrReturn;
        if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
        {
            ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
            sqlite3_free(pcErrReturn);
            return E_SQ_ERROR;
        }
        snprintf(SqlCommand, sizeof(SqlCommand), 
            "Delete From "TABLE_ATTRIBUTE" WHERE "DEVICE_MASK"=%llu",u64MacAddress);
        DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
        
        if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
        {
            ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
            sqlite3_free(pcErrReturn);
            return E_SQ_ERROR;
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "Error Parameter\n");
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}


////////////////////////////////////////////////////////***UPDATE***//////////////////////////////////////////////////////////
teSQ_Status eZigbeeSqliteUpdateDeviceTable(tsZigbee_Node *psZigbee_Node, teSQ_UpdateType eDeviceType)
{
    char *pcErrReturn;
    char SqlCommand[MDBF] = {0};
    teSQ_Status eSQ_Status = E_SQ_OK;

    if(psZigbee_Node->u64IEEEAddress)
    {
        switch(eDeviceType)
        {
            case (E_SQ_DEVICE_NAME):
            {
                if(NULL != psZigbee_Node->device_name)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), 
                        "Update "TABLE_DEVICE" SET "DEVICE_NAME"='%s' WHERE "DEVICE_MAC"=%llu",
                        psZigbee_Node->device_name, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
                    {
                        ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
                        sqlite3_free(pcErrReturn);
                        eSQ_Status = E_SQ_ERROR;
                    }
                }
            }
            break;
            case (E_SQ_DEVICE_ADDR):
            {
                if(psZigbee_Node->u16ShortAddress)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), 
                        "Update "TABLE_DEVICE" SET "DEVICE_ADDR"=%d WHERE "DEVICE_MAC"=%llu",
                        psZigbee_Node->u16ShortAddress, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    
                    if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
                    {
                        ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
                        sqlite3_free(pcErrReturn);
                        eSQ_Status = E_SQ_ERROR;
                    }
                }
            }
            break;
            case (E_SQ_DEVICE_ID):
            {
                if(psZigbee_Node->u16DeviceID)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), 
                        "Update "TABLE_DEVICE" SET "DEVICE_ID"=%d WHERE "DEVICE_MAC"=%llu",
                        psZigbee_Node->u16DeviceID, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
                    {
                        ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
                        sqlite3_free(pcErrReturn);
                        eSQ_Status = E_SQ_ERROR;
                    }
                }                
            }
            break;
            case (E_SQ_DEVICE_ONLINE):
            {
                memset(SqlCommand, 0, sizeof(SqlCommand));
                snprintf(SqlCommand, sizeof(SqlCommand), 
                    "Update "TABLE_DEVICE" SET "DEVICE_ONLINE"=%d WHERE "DEVICE_MAC"=%llu",
                    psZigbee_Node->u8DeviceOnline, psZigbee_Node->u64IEEEAddress);
                DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
                {
                    ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
                    sqlite3_free(pcErrReturn);
                    eSQ_Status = E_SQ_ERROR;
                }
            }
            break;
            case (E_SQ_APPENDS_TEXT):
            {
                if(NULL != psZigbee_Node->psDeviceDescription)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), 
                        "Update "TABLE_DEVICE" SET "APPENDS_TEXT"='%s' WHERE "DEVICE_MAC"=%llu",
                        psZigbee_Node->psDeviceDescription, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    
                    char *pcErrReturn;
                    if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
                    {
                        ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
                        sqlite3_free(pcErrReturn);
                        eSQ_Status = E_SQ_ERROR;
                    }
                }
            }
            break;
            default:
                eSQ_Status = E_SQ_ERROR;
                break;
        }
    }/*END SqliteZigbeeNode->u64MacAddress*/
    else
    {
        ERR_vPrintf(T_TRUE, "Error Parameter\n");
        eSQ_Status = E_SQ_ERROR;
    }

    return eSQ_Status;
}

teSQ_Status eZigbeeSqliteUpdateAttributeTable(tsZigbee_Node *SqliteZigbeeNode, uint16 u16ClusterID, uint16 u16AttributeValue, teSQ_UpdateType eDeviceType)
{
    char *pcErrReturn;
    char SqlCommand[MDBF] = {0};
    teSQ_Status eSQ_Status = E_SQ_OK;

    if(SqliteZigbeeNode->u64IEEEAddress)
    {
        switch(eDeviceType)
        {
            case (E_SQ_ATTRIBUTE_VALUE):
            {
                memset(SqlCommand, 0, sizeof(SqlCommand));
                snprintf(SqlCommand, sizeof(SqlCommand), 
                    "Update "TABLE_ATTRIBUTE" SET "CLUSTER_ID"='%d' WHERE "DEVICE_MAC"=%llu AND "CLUSTER_ID"=%d",
                    u16ClusterID, SqliteZigbeeNode->u64IEEEAddress, u16ClusterID);
                DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
                {
                    ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
                    sqlite3_free(pcErrReturn);
                    eSQ_Status = E_SQ_ERROR;
                }
            }
            break;
            case (E_SQ_APPENDS_TEXT):
            {
                if(NULL != SqliteZigbeeNode->psDeviceDescription)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), 
                        "Update "TABLE_ATTRIBUTE" SET "APPENDS_TEXT"='%s' WHERE "DEVICE_MAC"=%llu AND "CLUSTER_ID"=%d",
                        SqliteZigbeeNode->psDeviceDescription, SqliteZigbeeNode->u64IEEEAddress, u16ClusterID);
                    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    
                    char *pcErrReturn;
                    if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn))
                    {
                        ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
                        sqlite3_free(pcErrReturn);
                        eSQ_Status = E_SQ_ERROR;
                    }
                }
            }
            break;
            
            default:
                eSQ_Status = E_SQ_ERROR;
                break;
        }
    }/*END SqliteZigbeeNode->u64MacAddress*/
    else
    {
        ERR_vPrintf(T_TRUE, "Error Parameter\n");
        eSQ_Status = E_SQ_ERROR;
    }

    return eSQ_Status;
}

////////////////////////////////////////////////////////***RETRIEVE***//////////////////////////////////////////////////////////
bool_t eZigbeeSqliteDeviceIsExist(uint64 u64IEEEAddress)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), 
        "SELECT * FROM "TABLE_DEVICE" WHERE "DEVICE_MAC"=%llu",u64IEEEAddress);
    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL))
    {
        ERR_vPrintf(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return T_FALSE;
    }
    int iNum = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        iNum ++;
    }
    sqlite3_finalize(stmt);
    if(0 == iNum)
    {
        ERR_vPrintf(T_TRUE, "Not Found This Device\n");
        return T_FALSE;
    }

    return T_TRUE;
}

bool_t eZigbeeSqliteClusterIsExist(uint64 u64DeviceMask, uint16 u16ClusterID)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), 
        "SELECT * FROM "TABLE_ATTRIBUTE" WHERE "DEVICE_MASK"=%llu AND "CLUSTER_ID"=%d",u64DeviceMask, u16ClusterID);
    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL))
    {
        ERR_vPrintf(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return T_FALSE;
    }
    int iNum = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        iNum ++;
    }
    sqlite3_finalize(stmt);
    if(0 == iNum)
    {
        ERR_vPrintf(T_TRUE, "Not Found This Device\n");
        return T_FALSE;
    }

    return T_TRUE;
}


teSQ_Status eZigbeeSqliteRetrieveDevice(tsZigbee_Node *psZigbee_Node)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteRetrieveDevice\n");

    if(NULL != psZigbee_Node)
    {
        char SqlCommand[MDBF] = {0};
        snprintf(SqlCommand, sizeof(SqlCommand), 
            "SELECT * FROM "TABLE_DEVICE" WHERE "DEVICE_MAC"=%llu",psZigbee_Node->u64IEEEAddress);
        DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
        
        sqlite3_stmt * stmt = NULL;
        if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL))
        {
            ERR_vPrintf(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
            return E_SQ_ERROR;
        }
        int iNum = 0;
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            iNum ++;
            psZigbee_Node->u16ShortAddress         = sqlite3_column_int(stmt, 2);
            psZigbee_Node->u16DeviceID             = sqlite3_column_int(stmt, 3);
            memcpy(psZigbee_Node->device_name,(char*)sqlite3_column_text(stmt, 4), sqlite3_column_bytes(stmt,4));
            psZigbee_Node->u8DeviceOnline          = sqlite3_column_int(stmt, 5);
            if(psZigbee_Node->psDeviceDescription)
                memcpy(psZigbee_Node->psDeviceDescription,(char*)sqlite3_column_text(stmt, 6), sqlite3_column_bytes(stmt,6));
                
        }
        sqlite3_finalize(stmt);
        if(0 == iNum)
        {
            ERR_vPrintf(T_TRUE, "Not Found This Device\n");
            return E_SQ_NO_FOUND;
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "Error Parameter\n");
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteRetrieveAttribute(uint16 u16ClusterID, uint64 u64DeviceMask, uint16 *u16DeviceAttribute, uint64 *u64LastTime, char *psAppends)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteRetrieveDevice\n");

    if((0 != u64DeviceMask) && (0 != u16ClusterID) && (NULL != u16DeviceAttribute) && (NULL != u64LastTime))
    {
        char SqlCommand[MDBF] = {0};
        snprintf(SqlCommand, sizeof(SqlCommand), 
            "SELECT * FROM "TABLE_ATTRIBUTE" WHERE "DEVICE_MASK"=%llu AND "CLUSTER_ID"=%d",u64DeviceMask, u16ClusterID);
        DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
        
        sqlite3_stmt * stmt = NULL;
        if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL))
        {
            ERR_vPrintf(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
            return E_SQ_ERROR;
        }
        int iNum = 0;
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            iNum ++;
            *u16DeviceAttribute           = sqlite3_column_int(stmt, 3);
            *u64LastTime                  = sqlite3_column_int(stmt, 4);
            if(psAppends)
                memcpy(psAppends,(char*)sqlite3_column_text(stmt, 5), sqlite3_column_bytes(stmt,5));
                
        }
        sqlite3_finalize(stmt);
        if(0 == iNum)
        {
            ERR_vPrintf(T_TRUE, "Not Found This Device\n");
            return E_SQ_NO_FOUND;
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "Error Parameter\n");
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}


teSQ_Status eZigbeeSqliteRetrieveDevicesList(tsZigbee_Node *psZigbee_Node)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteRetrieveDevicesList\n");

    if(NULL != psZigbee_Node)
    {
        char SqlCommand[MDBF] = {0};
        snprintf(SqlCommand, sizeof(SqlCommand), 
            "SELECT * FROM "TABLE_DEVICE"");
        DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
        
        sqlite3_stmt * stmt = NULL;
        if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL))
        {
            ERR_vPrintf(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
            return E_SQ_ERROR;
        }
        int iNum = 0;
        dl_list_init(&psZigbee_Node->list); 
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            iNum ++;
            tsZigbee_Node *Temp = (tsZigbee_Node *)malloc(sizeof(tsZigbee_Node));
            psZigbee_Node->u64IEEEAddress           = sqlite3_column_int(stmt, 1);
            psZigbee_Node->u16ShortAddress         = sqlite3_column_int(stmt, 2);
            psZigbee_Node->u16DeviceID             = sqlite3_column_int(stmt, 3);
            memcpy(psZigbee_Node->device_name,(char*)sqlite3_column_text(stmt, 4), sqlite3_column_bytes(stmt,4));
            psZigbee_Node->u8DeviceOnline          = sqlite3_column_int(stmt, 5);
            if(psZigbee_Node->psDeviceDescription)
                memcpy(psZigbee_Node->psDeviceDescription,(char*)sqlite3_column_text(stmt, 6), sqlite3_column_bytes(stmt,6));
            dl_list_add_tail(&psZigbee_Node->list, &Temp->list);                
        }
        sqlite3_finalize(stmt);
        if(0 == iNum)
        {
            ERR_vPrintf(T_TRUE, "Not Found This Device\n");
            return E_SQ_NO_FOUND;
        }
    }
    else
    {
        ERR_vPrintf(T_TRUE, "Error Parameter\n");
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteRetrieveDevicesListFree(tsZigbee_Node *psZigbee_Node)
{
    DBG_vPrintf(DBG_SQLITE, "eZigbeeSqliteRetrieveDevicesList\n");

    tsZigbee_Node *Temp = NULL;
    tsZigbee_Node *Temp2 = NULL;
    dl_list_for_each_safe(Temp,Temp2,&psZigbee_Node->list,tsZigbee_Node,list)
    {
        dl_list_del(&Temp->list);
        free(Temp);
        Temp = NULL;
    }
    Temp2 = NULL;

    return E_SQ_OK;
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

