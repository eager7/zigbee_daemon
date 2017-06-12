/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          Serial interface
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
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "zigbee_sqlite.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_SQLITE (verbosity >= 4)
#define CHECK_SQLITE(f, ret) do{ if(SQLITE_OK != f){ ERR_vPrintf(T_TRUE, "Failed\n"); return ret; } }while(0)
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
tsZigbeeSqlite sZigbeeSqlite;

const char *pcDevicesTable = "CREATE TABLE IF NOT EXISTS "TABLE_DEVICE"("
                                INDEX" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                DEVICE_MAC" INTEGER UNIQUE DEFAULT 0, "
                                DEVICE_ADDR" INTEGER DEFAULT 0, "
                                DEVICE_ID" INTEGER DEFAULT 0, "
                                DEVICE_NAME" TEXT NOT NULL, "
                                DEVICE_CAPABILITY" INTEGER DEFAULT 0, "
                                DEVICE_ONLINE" INTEGER DEFAULT 0);";

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teSQ_Status eZigbeeSqliteOpen(char *pZigbeeSqlitePath)
{ 
    CHECK_POINTER(pZigbeeSqlitePath, E_SQ_ERROR);
    
	if (!access(pZigbeeSqlitePath, 0))
    {
        DBG_vPrintf(DBG_SQLITE, "Open %s \n",pZigbeeSqlitePath);
        if( SQLITE_OK != sqlite3_open( pZigbeeSqlitePath, &sZigbeeSqlite.psZgbeeDB) )
        {
            ERR_vPrintf(T_TRUE, "Failed to Open SQLITE_DB \n");
            return E_SQ_OPEN_ERROR;
        }
    }
	else //Datebase does not existd, create it
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

teSQ_Status eZigbeeSqliteInit(char *pZigbeeSqlitePath)
{
    memset(&sZigbeeSqlite, 0, sizeof(sZigbeeSqlite));
    eZigbeeSqliteOpen(pZigbeeSqlitePath);

    tsZigbeeBase sZigbee_Node;
    memset(&sZigbee_Node, 0, sizeof(sZigbee_Node));
    tsZigbeeBase *Temp = NULL;
    eZigbeeSqliteRetrieveDevicesList(&sZigbee_Node);
    dl_list_for_each(Temp,&sZigbee_Node.list,tsZigbeeBase,list)
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
    eZigbeeSqliteClose();

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteRetrieveDevicesList(tsZigbeeBase *psZigbee_Node)
{
    CHECK_POINTER(psZigbee_Node, E_SQ_ERROR);
    
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), "SELECT * FROM "TABLE_DEVICE"");
    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL))
    {
        ERR_vPrintf(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }
    dl_list_init(&psZigbee_Node->list); 
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        tsZigbeeBase *Temp = (tsZigbeeBase *)malloc(sizeof(tsZigbeeBase));
        memset(Temp, 0, sizeof(tsZigbeeBase));
        Temp->u64IEEEAddress           = sqlite3_column_int64(stmt, 1);
        Temp->u16ShortAddress          = sqlite3_column_int(stmt, 2);
        Temp->u16DeviceID              = sqlite3_column_int(stmt, 3);
        memcpy(Temp->auDeviceName,(char*)sqlite3_column_text(stmt, 4), sqlite3_column_bytes(stmt,4));
        Temp->u8MacCapability          = sqlite3_column_int(stmt, 5);
        Temp->u8DeviceOnline           = sqlite3_column_int(stmt, 6);
        dl_list_add_tail(&psZigbee_Node->list, &Temp->list); 
    }
    sqlite3_finalize(stmt);

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteRetrieveDevicesListFree(tsZigbeeBase *psZigbee_Node)
{
    CHECK_POINTER(psZigbee_Node, E_SQ_ERROR);

    tsZigbeeBase *Temp = NULL;
    tsZigbeeBase *Temp2 = NULL;
    dl_list_for_each_safe(Temp,Temp2,&psZigbee_Node->list,tsZigbeeBase,list)
    {
        dl_list_del(&Temp->list);
        FREE(Temp);
    }
    Temp2 = NULL;

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteUpdateDeviceTable(tsZigbeeBase *psZigbee_Node, teSQ_UpdateType eDeviceType)
{
    CHECK_POINTER(psZigbee_Node, E_SQ_ERROR);

    char *pcErrReturn;
    char SqlCommand[MDBF] = {0};
    teSQ_Status eSQ_Status = E_SQ_OK;

#define CHECK_EXEC(f) do{if(SQLITE_OK != f){ERR_vPrintf(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);sqlite3_free(pcErrReturn);eSQ_Status = E_SQ_ERROR;}}while(0)
    if(psZigbee_Node->u64IEEEAddress)
    {
        switch(eDeviceType)
        {
            case (E_SQ_DEVICE_NAME):
            {
                if(NULL != psZigbee_Node->auDeviceName)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), "Update "TABLE_DEVICE" SET "DEVICE_NAME"='%s' WHERE "DEVICE_MAC"=%llu",psZigbee_Node->auDeviceName, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    CHECK_EXEC(sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn));
                }
            }
            break;
            case (E_SQ_DEVICE_ADDR):
            {
                if(psZigbee_Node->u16ShortAddress)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), "Update "TABLE_DEVICE" SET "DEVICE_ADDR"=%d WHERE "DEVICE_MAC"=%llu",psZigbee_Node->u16ShortAddress, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    
                    CHECK_EXEC(sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn));
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
                    CHECK_EXEC(sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn));
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
                CHECK_EXEC(sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn));
                
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

teSQ_Status eZigbeeSqliteAddNewDevice(uint64 u64MacAddress, uint16 u16ShortAddress, uint16 u16DeviceID, char *psDeviceName, uint8 u8Capability)
{
    CHECK_POINTER(psDeviceName, E_SQ_ERROR);

    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), 
        "INSERT INTO "TABLE_DEVICE"("DEVICE_MAC","DEVICE_ADDR","DEVICE_ID","DEVICE_NAME","DEVICE_ONLINE","DEVICE_CAPABILITY") VALUES(%llu,%d,%d,'%s',1,%d)",
        u64MacAddress, u16ShortAddress, u16DeviceID, psDeviceName,u8Capability);
    DBG_vPrintf(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if((SQLITE_OK != ret)&&(SQLITE_CONSTRAINT == ret))
    {
        tsZigbeeBase sZigbeeNode;
        memset(&sZigbeeNode, 0, sizeof(sZigbeeNode));
        sZigbeeNode.u64IEEEAddress = u64MacAddress;
        sZigbeeNode.u8DeviceOnline = 1;
        eZigbeeSqliteUpdateDeviceTable(&sZigbeeNode, E_SQ_DEVICE_ONLINE);
    }
    else
    {
        sqlite3_free(pcErrReturn);
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

