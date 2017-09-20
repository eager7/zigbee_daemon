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
#include <string.h>
#include <door_lock.h>
#include <zigbee_node.h>
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
                                DEVICE_ONLINE" INTEGER DEFAULT 0,"
                                DEVICE_INFO" TEXT DEFAULT NULL);";

const char *psUserTable = "CREATE TABLE IF NOT EXISTS "TABLE_USER"("
                                INDEX"INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                USER_ID"INTEGER UNIQUE DEFAULT 0, "
                                USER_NAME"TEXT NOT NULL, "
                                USER_TYPE"INTEGER DEFAULT 0, "
                                USER_PERM"INTEGER DEFAULT 0);";

const char *psPasswordTable = "CREATE TABLE IF NOT EXISTS "TABLE_PASSWORD"("
                                INDEX"INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                PASSWORD_ID"INTEGER UNIQUE DEFAULT 0, "
                                PASSWORD_WORK"INTEGER UNIQUE DEFAULT 0, "
                                PASSWORD_AVAILABLE"INTEGER DEFAULT 0, "
                                PASSWORD_USED"INTEGER DEFAULT 0, "
                                PASSWORD_CREATION_TIME"INTEGER DEFAULT 0, "
                                PASSWORD_START_TIME"INTEGER DEFAULT 0, "
                                PASSWORD_END_TIME"INTEGER DEFAULT 0, "
                                PASSWORD_LEN"INTEGER DEFAULT 0, "
                                PASSWORD_DATA" TEXT NOT NULL);";

const char *psRecordTable = "CREATE TABLE IF NOT EXISTS "TABLE_RECORD"("
                                INDEX"INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                RECORD_TYPE"INTEGER DEFAULT 0, "
                                RECORD_USER"INTEGER DEFAULT 0, "
                                RECORD_TIME"INTEGER DEFAULT 0, "
                                PASSWORD_DATA"TEXT DEFAULT NULL);";
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
        eZigbeeSqliteUpdateDeviceOnline(Temp->u64IEEEAddress, 0);
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
    //DBG_vPrintln(DBG_SQLITE, "Sqlite's Command: %s\n", SqlCommand);
    
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
        Temp->psInfo = (char*)calloc(1, (size_t)sqlite3_column_bytes(stmt,7));
        memcpy(Temp->psInfo, (char*)sqlite3_column_text(stmt, 7), (size_t)sqlite3_column_bytes(stmt,7));
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
        FREE(Temp->psInfo);
        FREE(Temp);
    }

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteUpdateDeviceTable(tsZigbeeBase *psZigbee_Node)
{
    CHECK_POINTER(psZigbee_Node, E_SQ_ERROR);

    char *pcErrReturn;
    char SqlCommand[MDBF] = {0};
    teSQ_Status eSQ_Status = E_SQ_OK;

    if(psZigbee_Node->u64IEEEAddress) {
        snprintf(SqlCommand, sizeof(SqlCommand), "Update "
                         TABLE_DEVICE" SET "
                         DEVICE_ADDR"=%d, "
                         DEVICE_ID"=%d, "
                         DEVICE_NAME"='%s',"
                         DEVICE_CAPABILITY"=%d,"
                         DEVICE_ONLINE"=%d,"
                         DEVICE_INFO"='%s'"
                         " WHERE "DEVICE_MAC"=%llu",
                 psZigbee_Node->u16ShortAddress,
                 psZigbee_Node->u16DeviceID,
                 psZigbee_Node->auDeviceName,
                 psZigbee_Node->u8MacCapability,
                 psZigbee_Node->u8DeviceOnline,
                 psZigbee_Node->psInfo,
                 psZigbee_Node->u64IEEEAddress);
        DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
        if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn)){
            ERR_vPrintln(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
            sqlite3_free(pcErrReturn);
            eSQ_Status = E_SQ_ERROR;
        }
    } else {
        ERR_vPrintln(T_TRUE, "Error Parameter\n");
        eSQ_Status = E_SQ_ERROR;
    }

    return eSQ_Status;
}

teSQ_Status eZigbeeSqliteUpdateDeviceOnline(uint64 u64IEEEAddress, uint8 u8DeviceOnline)
{
    char *pcErrReturn;
    char SqlCommand[MDBF] = {0};
    teSQ_Status eSQ_Status = E_SQ_OK;

        snprintf(SqlCommand, sizeof(SqlCommand), "Update "
                         TABLE_DEVICE" SET "
                         DEVICE_ONLINE"=%d"
                         " WHERE "DEVICE_MAC"=%llu",
                 u8DeviceOnline, u64IEEEAddress);
        DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
        if(SQLITE_OK != sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn)){
            ERR_vPrintln(T_TRUE, "sqlite3_exec (%s)\n", pcErrReturn);
            sqlite3_free(pcErrReturn);
            eSQ_Status = E_SQ_ERROR;
        }

    return eSQ_Status;
}

teSQ_Status eZigbeeSqliteAddNewDevice(uint64 u64MacAddress, uint16 u16ShortAddress, uint16 u16DeviceID, char *psDeviceName,
                                      uint8 u8Capability, const char *psInformation)
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
                DEVICE_CAPABILITY","
                DEVICE_INFO
                ") VALUES(%llu,%d,%d,'%s',1,%d,'%s')",
        u64MacAddress, u16ShortAddress, u16DeviceID, psDeviceName,u8Capability, psInformation);
    DBG_vPrintln(DBG_SQLITE, "Sqite's Command: %s\n", SqlCommand);
    
    char *pcErrReturn;
    int ret = sqlite3_exec(sZigbeeSqlite.psZgbeeDB, SqlCommand, NULL, NULL, &pcErrReturn);
    if((SQLITE_OK != ret)&&(SQLITE_CONSTRAINT == ret)) {/** 设备已添加过数据库 */
        eZigbeeSqliteUpdateDeviceOnline(u64MacAddress, 1);
    } else if(SQLITE_OK != ret){
        sqlite3_free(pcErrReturn);
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }

    return E_SQ_OK;
}

/**
 * 用户的数据库操作
 * */
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
             "DELETE FROM "TABLE_USER " WHERE "USER_ID"=%d", u8UserID);
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

/**
 * 开锁记录的数据库操作
 * */
teSQ_Status eZigbeeSqliteAddDoorLockRecord(teDoorLockUserType eType, uint8 u8UserID, uint32 u32Time, const char *psPassword)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "INSERT INTO "TABLE_RECORD"("
                     RECORD_TYPE","
                     RECORD_USER","
                     RECORD_TIME","
                     PASSWORD_DATA") VALUES(%d,%d,%d,%s)",
             eType, u8UserID, u32Time, psPassword);
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

teSQ_Status eZigbeeSqliteDoorLockRetrieveRecordList(uint8 u8UserID, tsDoorLockRecord *psRecordHeader)
{
    CHECK_POINTER(psRecordHeader, E_SQ_ERROR);

    char SqlCommand[MDBF] = {0};
    if(u8UserID == 0xff){
        snprintf(SqlCommand, sizeof(SqlCommand), "SELECT * FROM "TABLE_RECORD"");
    } else {
        snprintf(SqlCommand, sizeof(SqlCommand), "SELECT * FROM "TABLE_RECORD" WHERE "USER_ID"=%d", u8UserID);
    }
    DBG_vPrintln(DBG_SQLITE, "Sqlite's Command: %s\n", SqlCommand);

    sqlite3_stmt * stmt = NULL;
    if(SQLITE_OK != sqlite3_prepare_v2(sZigbeeSqlite.psZgbeeDB, SqlCommand, -1, &stmt, NULL)) {
        ERR_vPrintln(T_TRUE, "sqlite error: (%s)\n", sqlite3_errmsg(sZigbeeSqlite.psZgbeeDB));
        return E_SQ_ERROR;
    }
    dl_list_init(&psRecordHeader->list);
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        tsDoorLockRecord *Temp = (tsDoorLockRecord *)malloc(sizeof(tsDoorLockRecord));
        memset(Temp, 0, sizeof(tsDoorLockRecord));
        Temp->u8UserId = (uint8) sqlite3_column_int(stmt, 1);
        Temp->eType = (teDoorLockUserType) sqlite3_column_int(stmt, 2);
        Temp->u32Time = (uint32) sqlite3_column_int(stmt, 3);
        memcpy(Temp->auPassword,(char*)sqlite3_column_text(stmt, 4), (size_t)sqlite3_column_bytes(stmt,4));
        dl_list_add_tail(&psRecordHeader->list, &Temp->list);
    }
    sqlite3_finalize(stmt);

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDoorLockRetrieveRecordListFree(tsDoorLockRecord *psRecordHeader)
{
    CHECK_POINTER(psRecordHeader, E_SQ_ERROR);

    tsDoorLockRecord *Temp = NULL;
    tsDoorLockRecord *Temp2 = NULL;
    dl_list_for_each_safe(Temp,Temp2,&psRecordHeader->list,tsDoorLockRecord,list) {
        dl_list_del(&Temp->list);
        FREE(Temp);
    }

    return E_SQ_OK;
}
/**
 * 临时密码的数据库操作
 * */
teSQ_Status eZigbeeSqliteAddDoorLockPassword(uint8 u8PasswordID, uint8 u8Worked, uint8 u8Available, uint32 u32StartTime,
                                             uint32 u32EndTime, uint8 u8PasswordLen, const char *psPassword)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "INSERT INTO "TABLE_PASSWORD"("
                     PASSWORD_ID","
                     PASSWORD_WORK","
                     PASSWORD_AVAILABLE","
                     PASSWORD_USED","
                     PASSWORD_CREATION_TIME","
                     PASSWORD_START_TIME","
                     PASSWORD_END_TIME","
                     PASSWORD_LEN","
                     PASSWORD_DATA") VALUES(%d,%d,%d,%d,%d,%d, %d, %d, '%s')",
             u8PasswordID, u8Worked, u8Available, 0, (uint32)time((time_t*)NULL), u32StartTime, u32EndTime, u8PasswordLen, psPassword);
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
             "DELETE FROM "TABLE_PASSWORD" WHERE "PASSWORD_ID"=%d", u8PasswordID);
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

teSQ_Status eZigbeeSqliteUpdateDoorLockPassword(uint8 u8PasswordID, uint8 u8Available, uint8 u8Worked, uint8 u8UsedNum)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "UPDATE "TABLE_PASSWORD" SET "
                     PASSWORD_AVAILABLE"=%d, "
                     PASSWORD_WORK"=%d, "
                     PASSWORD_USED"=%d "" WHERE "
                     PASSWORD_ID"=%d",
             u8Available, u8Worked, u8UsedNum, u8PasswordID);
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

teSQ_Status eZigbeeSqliteDoorLockRetrievePassword(uint8 u8PasswordID, tsTemporaryPassword *psPassword)
{
    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand),
             "SELECT * FROM "TABLE_PASSWORD" WHERE "PASSWORD_ID"=%d", u8PasswordID);
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
        psPassword->u8UseNum  = (uint8)sqlite3_column_int(stmt, 4);
        psPassword->u32TimeCreation  = (uint32)sqlite3_column_int(stmt, 5);
        psPassword->u32TimeStart    = (uint32)sqlite3_column_int(stmt, 6);
        psPassword->u32TimeEnd      = (uint32)sqlite3_column_int(stmt, 7);
        psPassword->u8PasswordLen   = (uint8)sqlite3_column_int(stmt, 8);
        memcpy(psPassword->auPassword,(char*)sqlite3_column_text(stmt, 9), (size_t)sqlite3_column_bytes(stmt,9));
    }
    sqlite3_finalize(stmt);

    return E_SQ_OK;
}

teSQ_Status eZigbeeSqliteDoorLockRetrievePasswordList(tsTemporaryPassword *psPasswordHeader)
{
    CHECK_POINTER(psPasswordHeader, E_SQ_ERROR);

    char SqlCommand[MDBF] = {0};
    snprintf(SqlCommand, sizeof(SqlCommand), "SELECT * FROM "TABLE_PASSWORD"");
    //DBG_vPrintln(DBG_SQLITE, "Sqlite's Command: %s\n", SqlCommand);

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
        Temp->u8UseNum  = (uint8)sqlite3_column_int(stmt, 4);
        Temp->u32TimeCreation  = (uint32)sqlite3_column_int(stmt, 5);
        Temp->u32TimeStart    = (uint32)sqlite3_column_int(stmt, 6);
        Temp->u32TimeEnd      = (uint32)sqlite3_column_int(stmt, 7);
        Temp->u8PasswordLen   = (uint8)sqlite3_column_int(stmt, 8);
        memcpy(Temp->auPassword,(char*)sqlite3_column_text(stmt, 9), (size_t)sqlite3_column_bytes(stmt,9));
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