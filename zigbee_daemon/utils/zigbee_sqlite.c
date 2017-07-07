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
#include <door_lock.h>
#include "zigbee_sqlite.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_SQLITE (verbosity >= 4)
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

const char *psUserTable = "CREATE TABLE IF NOT EXISTS "TABLE_USER"("
                                INDEX"INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                USER_ID"INTEGER UNIQUE DEFAULT 0, "
                                USER_NAME"TEXT NOT NULL, "
                                USER_TYPE"INTEGER DEFAULT 0, "
                                USER_PERM"INTEGER DEFAULT 0);";

const char *psPasswordTable = "CREATE TABLE IF NOT EXISTS "TABLE_PASSWORD"("
                                INDEX"INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                PASSWD_ID"INTEGER UNIQUE DEFAULT 0, "
                                PASSWD_WORK"INTEGER UNIQUE DEFAULT 0, "
                                PASSWD_AVAILABLE"INTEGER DEFAULT 0, "
                                PASSWD_START_TIME"INTEGER DEFAULT 0, "
                                PASSWD_END_TIME"INTEGER DEFAULT 0, "
                                PASSWD_LEN"INTEGER DEFAULT 0, "
                                PASSWD_DATA" TEXT NOT NULL);";

const char *psRecordTable = "CREATE TABLE IF NOT EXISTS "TABLE_RECORD"("
                                INDEX"INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                RECORD_TYPE"INTEGER DEFAULT 0, "
                                RECORD_USER"INTEGER DEFAULT 0, "
                                RECORD_WORK"INTEGER DEFAULT 0, "
                                RECORD_TIME"INTEGER DEFAULT 0);";
/****************************************************************************/
/***        Located Functions                                            ***/
/****************************************************************************/
static teSQ_Status eZigbeeSqliteOpen(char *pZigbeeSqlitePath)
{
    CHECK_POINTER(pZigbeeSqlitePath, E_SQ_ERROR);

    if (!access(pZigbeeSqlitePath, 0)) {
        DBG_vPrintln(DBG_SQLITE, "Open %s \n",pZigbeeSqlitePath);
        if( SQLITE_OK != sqlite3_open( pZigbeeSqlitePath, &sZigbeeSqlite.psZgbeeDB) ) {
            ERR_vPrintln(T_TRUE, "Failed to Open SQLITE_DB \n");
            return E_SQ_OPEN_ERROR;
        }
    } else {
        /** Database does not existd, create it */
        DBG_vPrintln(DBG_SQLITE, "Create A New Database %s \n",pZigbeeSqlitePath);
        if (SQLITE_OK != sqlite3_open_v2(pZigbeeSqlitePath, &sZigbeeSqlite.psZgbeeDB,
                                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL)) {
            ERR_vPrintln(T_TRUE, "Error initialising PDM database (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
            return E_SQ_ERROR;
        }

        /** Create Tables */
        DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", pcDevicesTable);
        char *pcErrReturn;      /*Create default table*/
        if((SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, pcDevicesTable, NULL, NULL, &pcErrReturn))||
           (SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, psUserTable, NULL, NULL, &pcErrReturn))||
           (SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, psPasswordTable, NULL, NULL, &pcErrReturn))||
           (SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, psRecordTable, NULL, NULL, &pcErrReturn))
                ) {
            ERR_vPrintln(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
            sqlite3_free(pcErrReturn);
            unlink(pZigbeeSqlitePath);
            return E_SQ_ERROR;
        }
    }

    return E_SQ_OK;
}

static teSQ_Status eZigbeeSqliteClose(void)
{
    DBG_vPrintln(DBG_SQLITE, "eZigbeeSqliteClose\n");
    if (NULL != sZigbeeSqlite.psZgbeeDB) {
        sqlite3_close(sZigbeeSqlite.psZgbeeDB);
    }

    return E_SQ_OK;
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teSQ_Status eZigbeeSqliteInit(char *pZigbeeSqlitePath)
{
    memset(&sZigbeeSqlite, 0, sizeof(sZigbeeSqlite));
    eZigbeeSqliteOpen(pZigbeeSqlitePath);

    tsZigbeeBase sZigbee_Node;
    memset(&sZigbee_Node, 0, sizeof(sZigbee_Node));
    tsZigbeeBase *Temp = NULL;
    eZigbeeSqliteRetrieveDevicesList(&sZigbee_Node);
    dl_list_for_each(Temp,&sZigbee_Node.list,tsZigbeeBase,list) {
        Temp->u8DeviceOnline = 0;
        eZigbeeSqliteUpdateDeviceTable(Temp, E_SQ_DEVICE_ONLINE);
    }
    eZigbeeSqliteRetrieveDevicesListFree(&sZigbee_Node);
    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteFinished(void)
{
    DBG_vPrintln(DBG_SQLITE, "eZigbeeSqliteFinished\n");
    eZigbeeSqliteClose();

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteRetrieveDevicesList(tsZigbeeBase *psZigbee_Node)
{
    CHECK_POINTER(psZigbee_Node, E_SQ_ERROR);
    
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), "SELECT * FROM "TABLE_DEVICE"");
    DBG_vPrintln(DBG_SQLITE, "Sqlite's Command: %s\n", SqlCommand);
    
    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL)) {
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }
    dl_list_init(&psZigbee_Node->list); 
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        tsZigbeeBase *Temp = (tsZigbeeBase *)malloc(sizeof(tsZigbeeBase));
        memset(Temp, 0, sizeof(tsZigbeeBase));
        Temp->u64IEEEAddress           = (uint64)sqlite3_column_int64(stmt, 1);
        Temp->u16ShortAddress          = (uint16)sqlite3_column_int(stmt, 2);
        Temp->u16DeviceID              = (uint16)sqlite3_column_int(stmt, 3);
        memcpy(Temp->auDeviceName,(char*)sqlite3_column_text(stmt, 4), (size_t)sqlite3_column_bytes(stmt,4));
        Temp->u8MacCapability          = (uint8)sqlite3_column_int(stmt, 5);
        Temp->u8DeviceOnline           = (uint8)sqlite3_column_int(stmt, 6);
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
    dl_list_for_each_safe(Temp,Temp2,&psZigbee_Node->list,tsZigbeeBase,list) {
        dl_list_del(&Temp->list);
        FREE(Temp);
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteUpdateDeviceTable(tsZigbeeBase *psZigbee_Node, teSQ_UpdateType eDeviceType)
{
    CHECK_POINTER(psZigbee_Node, E_SQ_ERROR);

    char *pcErrReturn;
    char SqlCommand[MDBF] = {0};
    teSQ_Status eSQ_Status = E_SQ_OK;

    #define CHECK_EXEC(f) do{if(SQLITE_OK != f)\
    {ERR_vPrintln(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);\
    sqlite3_free(pcErrReturn);\
    eSQ_Status = E_SQ_ERROR;}}while(0)
    if(psZigbee_Node->u64IEEEAddress)
    {
        switch(eDeviceType)
        {
            case (E_SQ_DEVICE_NAME):
            {
                if(NULL != psZigbee_Node->auDeviceName)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), "Update "TABLE_DEVICE" SET "DEVICE_NAME"='%s' WHERE "DEVICE_MAC"=%llu",
                             psZigbee_Node->auDeviceName, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    CHECK_EXEC(sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn));
                }
            }
            break;
            case (E_SQ_DEVICE_ADDR):
            {
                if(psZigbee_Node->u16ShortAddress)
                {
                    memset(SqlCommand, 0, sizeof(SqlCommand));
                    snprintf(SqlCommand, sizeof(SqlCommand), "Update "TABLE_DEVICE" SET "DEVICE_ADDR"=%d WHERE "DEVICE_MAC"=%llu",
                             psZigbee_Node->u16ShortAddress, psZigbee_Node->u64IEEEAddress);
                    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
                    
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
                    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
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
                DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
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
        ERR_vPrintln(T_TRUE, "Error Parameter\n");
        eSQ_Status = E_SQ_ERROR;
    }

    return eSQ_Status;
}

teSQ_Status eZigbeeSqliteAddNewDevice(uint64 u64MacAddress, uint16 u16ShortAddress, uint16 u16DeviceID, char *psDeviceName, uint8 u8Capability)
{
    CHECK_POINTER(psDeviceName, E_SQ_ERROR);

    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), 
        "INSERT INTO "TABLE_DEVICE"("
                DEVICE_MAC","
                DEVICE_ADDR","
                DEVICE_ID","
                DEVICE_NAME","
                DEVICE_ONLINE","
                DEVICE_CAPABILITY") VALUES(%llu,%d,%d,'%s',1,%d)",
        u64MacAddress, u16ShortAddress, u16DeviceID, psDeviceName,u8Capability);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if((SQLITE_OK != ret)&&(SQLITE_CONSTRAINT == ret)) {
        tsZigbeeBase sZigbeeNode;
        memset(&sZigbeeNode, 0, sizeof(sZigbeeNode));
        sZigbeeNode.u64IEEEAddress = u64MacAddress;
        sZigbeeNode.u8DeviceOnline = 1;
        eZigbeeSqliteUpdateDeviceTable(&sZigbeeNode, E_SQ_DEVICE_ONLINE);
    } else {
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteAddDoorLockUser(uint8 u8UserID, uint8 u8UserType, uint8 u8UserPerm, char *psUserName)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "INSERT INTO "TABLE_USER"("
                     USER_ID","
                     USER_NAME","
                     USER_TYPE","
                     USER_PERM") VALUES(%d,'%s',%d,%d)",
             u8UserID, psUserName, u8UserType, u8UserPerm);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDelDoorLockUser(uint8 u8UserID)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "DELETE FROM "TABLE_USER"WHERE"USER_ID"=%d", u8UserID);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDelDoorLockRetrieveUserList(tsDoorLockUser *psUserHeader)
{
    CHECK_POINTER(psUserHeader, E_SQ_ERROR);

    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), "SELECT * FROM "TABLE_USER"");
    DBG_vPrintln(DBG_SQLITE, "Sqlite's Command: %s\n", SqlCommand);

    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL)) {
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }
    dl_list_init(&psUserHeader->list);
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        tsDoorLockUser *Temp = (tsDoorLockUser *)malloc(sizeof(tsDoorLockUser));
        memset(Temp, 0, sizeof(tsDoorLockUser));
        Temp->u8UserID    = (uint8)sqlite3_column_int(stmt, 1);
        memcpy(Temp->auName,(char*)sqlite3_column_text(stmt, 2), (size_t)sqlite3_column_bytes(stmt,2));
        Temp->eUserType  = (teDoorLockUserType)sqlite3_column_int(stmt, 3);
        Temp->eUserPerm    = (teDoorLockUserPerm)sqlite3_column_int(stmt, 4);
        dl_list_add_tail(&psUserHeader->list, &Temp->list);
    }
    sqlite3_finalize(stmt);

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDoorLockRetrieveUserListFree(tsDoorLockUser *psPasswordHeader)
{
    CHECK_POINTER(psPasswordHeader, E_SQ_ERROR);

    tsDoorLockUser *Temp = NULL;
    tsDoorLockUser *Temp2 = NULL;
    dl_list_for_each_safe(Temp,Temp2,&psPasswordHeader->list,tsDoorLockUser,list) {
        dl_list_del(&Temp->list);
        FREE(Temp);
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteAddDoorLockRecord(uint8 u8Type, uint8 u8UserID, uint64 u64Time)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "INSERT INTO "TABLE_RECORD"("
                     RECORD_TYPE","
                     RECORD_USER","
                     RECORD_TIME") VALUES(%d,%d,%llu)",
             u8Type, u8UserID, u64Time);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteAddDoorLockPassword(uint8 u8PasswordID, uint8 u8Worked, uint8 u8Available, uint32 u32StartTime,
                                             uint32 u32EndTime, uint8 u8PasswordLen, const char *psPassword)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "INSERT INTO "TABLE_PASSWORD"("
                     PASSWD_ID","
                     PASSWD_WORK","
                     PASSWD_AVAILABLE","
                     PASSWD_START_TIME","
                     PASSWD_END_TIME","
                     PASSWD_LEN","
                     PASSWD_DATA") VALUES(%d,%d,%d,%d, %d, %d, '%s')",
             u8PasswordID, u8Worked, u8Available, u32StartTime, u32EndTime, u8PasswordLen, psPassword);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDelDoorLockPassword(uint8 u8PasswordID)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "DELETE FROM "TABLE_PASSWORD" WHERE "PASSWD_ID"=%d", u8PasswordID);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteUpdateDoorLockPassword(uint8 u8PasswordID, uint8 u8Available, uint8 u8Worked)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "UPDATE "TABLE_PASSWORD" SET "PASSWD_AVAILABLE"=%d WHERE "PASSWD_ID"=%d", u8Available, u8PasswordID);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    memset(SqlCommand, 0, sizeof(SqlCommand));
    snprintf(SqlCommand, sizeof(SqlCommand),
             "UPDATE "TABLE_PASSWORD" SET "PASSWD_WORK"=%d WHERE "PASSWD_ID"=%d", u8Worked, u8PasswordID);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDoorLockRetrievePassword(uint8 u8PasswordID, tsTemporaryPassword *psPassword)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "SELECT * FROM "TABLE_PASSWORD" WHERE "PASSWD_ID"=%d", u8PasswordID);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);

    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL)) {
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        psPassword->u8PasswordId    = (uint8)sqlite3_column_int(stmt, 1);
        psPassword->u8Worked        = (uint8)sqlite3_column_int(stmt, 2);
        psPassword->u8AvailableNum  = (uint8)sqlite3_column_int(stmt, 3);
        psPassword->u32TimeStart    = (uint32)sqlite3_column_int(stmt, 4);
        psPassword->u32TimeEnd      = (uint32)sqlite3_column_int(stmt, 5);
        psPassword->u8PasswordLen   = (uint8)sqlite3_column_int(stmt, 6);
        memcpy(psPassword->auPassword,(char*)sqlite3_column_text(stmt, 7), (size_t)sqlite3_column_bytes(stmt,7));
    }
    sqlite3_finalize(stmt);

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDoorLockRetrievePasswordList(tsTemporaryPassword *psPasswordHeader)
{
    CHECK_POINTER(psPasswordHeader, E_SQ_ERROR);

    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), "SELECT * FROM "TABLE_PASSWORD"");
    DBG_vPrintln(DBG_SQLITE, "Sqlite's Command: %s\n", SqlCommand);

    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL)) {
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }
    dl_list_init(&psPasswordHeader->list);
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        tsTemporaryPassword *Temp = (tsTemporaryPassword *)malloc(sizeof(tsTemporaryPassword));
        memset(Temp, 0, sizeof(tsTemporaryPassword));
        Temp->u8PasswordId    = (uint8)sqlite3_column_int(stmt, 1);
        Temp->u8Worked        = (uint8)sqlite3_column_int(stmt, 2);
        Temp->u8AvailableNum  = (uint8)sqlite3_column_int(stmt, 3);
        Temp->u32TimeStart    = (uint32)sqlite3_column_int(stmt, 4);
        Temp->u32TimeEnd      = (uint32)sqlite3_column_int(stmt, 5);
        Temp->u8PasswordLen   = (uint8)sqlite3_column_int(stmt, 6);
        memcpy(Temp->auPassword,(char*)sqlite3_column_text(stmt, 7), (size_t)sqlite3_column_bytes(stmt,7));
        dl_list_add_tail(&psPasswordHeader->list, &Temp->list);
    }
    sqlite3_finalize(stmt);

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDoorLockRetrievePasswordListFree(tsTemporaryPassword *psPasswordHeader)
{
    CHECK_POINTER(psPasswordHeader, E_SQ_ERROR);

    tsTemporaryPassword *Temp = NULL;
    tsTemporaryPassword *Temp2 = NULL;
    dl_list_for_each_safe(Temp,Temp2,&psPasswordHeader->list,tsTemporaryPassword,list) {
        dl_list_del(&Temp->list);
        FREE(Temp);
    }

    return E_SQ_OK;
}