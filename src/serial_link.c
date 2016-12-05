/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          SerialLink interface
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>

#include "serial_link.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_SERIALLINK 0
#define DBG_SERIALLINK_CB 0
#define DBG_SERIALLINK_COMMS 0
#define DBG_SERIALLINK_QUEUE 0

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static uint8 u8SL_CalculateCRC(uint16 u16Type, uint16 u16Length, uint8 *pu8Data);
static int iSL_TxByte(bool_t bSpecialCharacter, uint8 u8Data);
static bool_t bSL_RxByte(uint8 *pu8Data);
static teSL_Status eSL_WriteMessage(uint16 u16Type, uint16 u16Length, uint8 *pu8Data);
static teSL_Status eSL_ReadMessage(uint16 *pu16Type, uint16 *pu16Length, uint16 u16MaxLength, uint8 *pu8Message);
static void *pvSerialReaderThread(void *psThreadInfoVoid);
static void *pvCallbackHandlerThread(void *psThreadInfoVoid);
static void *pvMessageQueueHandlerThread(void *psThreadInfoVoid);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern int verbosity;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

static tsSerialLink sSerialLink;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


teSL_Status eSL_Init(char *cpSerialDevice, uint32 u32BaudRate)
{
    memset(&sSerialLink, 0, sizeof(tsSerialLink));
    if (eSerial_Init(cpSerialDevice, u32BaudRate, &sSerialLink.iSerialFd) != E_SERIAL_OK){
        ERR_vPrintf(T_TRUE,"can't init serial success\n");
        return E_SL_ERROR_SERIAL;
    }
    
    CHECK_RESULT(mLockCreate(&sSerialLink.mutex_write),E_LOCK_OK,E_SL_ERROR);
    
    int i;
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++){
        pthread_mutex_init(&sSerialLink.asReaderMessageQueue[i].mutex, NULL);
        pthread_cond_init(&sSerialLink.asReaderMessageQueue[i].cond_data_available, NULL);
        sSerialLink.asReaderMessageQueue[i].u16Type = 0;
    }
    
    /* Start the callback handler thread */
    sSerialLink.sCallbackHandleThread.pvThreadData = &sSerialLink;
    CHECK_RESULT(mLockCreate(&sSerialLink.sCallbacks.mutex),E_LOCK_OK,E_SL_ERROR);
    CHECK_RESULT(mQueueCreate(&sSerialLink.sCallbackQueue, SL_MAX_CALLBACK_QUEUES), E_QUEUE_OK, E_SL_ERROR);
    CHECK_RESULT(mThreadStart(pvCallbackHandlerThread, &sSerialLink.sCallbackHandleThread, E_THREAD_DETACHED), E_THREAD_OK, E_SL_ERROR);
    
    /* Start the message queue handler thread */
    sSerialLink.sMessageHandleThread.pvThreadData = &sSerialLink;
    CHECK_RESULT(mQueueCreate(&sSerialLink.sMessageQueue, SL_MAX_MESSAGE_QUEUES + 2), E_QUEUE_OK, E_SL_ERROR);
    CHECK_RESULT(mThreadStart(pvMessageQueueHandlerThread, &sSerialLink.sMessageHandleThread, E_THREAD_DETACHED), E_THREAD_OK, E_SL_ERROR);

    /* Start the Serial thread */
    sSerialLink.sSerialReaderThread.pvThreadData = &sSerialLink;
    CHECK_RESULT(mThreadStart(pvSerialReaderThread, &sSerialLink.sSerialReaderThread, E_THREAD_DETACHED), E_THREAD_OK, E_SL_ERROR);
    
    DBG_vPrintf(T_TRUE, "eSL_Init OK\n");
    return E_SL_OK;
}


teSL_Status eSL_Destroy(void)
{
    mThreadStop(&sSerialLink.sCallbackHandleThread);
    mQueueDestroy(&sSerialLink.sCallbackQueue);

    mThreadStop(&sSerialLink.sMessageHandleThread);
    mQueueDestroy(&sSerialLink.sMessageQueue);

    mThreadStop(&sSerialLink.sSerialReaderThread);
 
    while (sSerialLink.sCallbacks.psListHead){   
        eSL_RemoveListener(sSerialLink.sCallbacks.psListHead->u16Type, sSerialLink.sCallbacks.psListHead->prCallback);
    }
    
    return E_SL_OK;
}


teSL_Status eSL_SendMessage(uint16 u16Type, uint16 u16Length, void *pvMessage, uint8 *pu8SequenceNo)
{
    teSL_Status eStatus;
    
    /* Make sure there is only one thread sending messages to the node at a time. */
    pthread_mutex_lock(&sSerialLink.mutex_write);
    
    eStatus = eSL_WriteMessage(u16Type, u16Length, (uint8 *)pvMessage);
    if (eStatus == E_SL_OK)/* Command sent successfully */
    {
        uint16    u16Length;
        tsSL_Msg_Status sStatus;
        tsSL_Msg_Status *psStatus = &sStatus;
        
        psStatus->u16MessageType = u16Type;
        eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 100, &u16Length, (void**)&psStatus)/* Expect a status response within 100ms */;
        if (eStatus == E_SL_OK)
        {
            DBG_vPrintf(DBG_SERIALLINK, "Status: %d, Sequence %d\n", psStatus->eStatus, psStatus->u8SequenceNo);
            eStatus = psStatus->eStatus;
            if (eStatus == E_SL_OK){
                if (pu8SequenceNo){
                    *pu8SequenceNo = psStatus->u8SequenceNo;
                }
            }
            free(psStatus);//malloc in eSL_MessageQueue
        }
    }
    pthread_mutex_unlock(&sSerialLink.mutex_write);
    return eStatus;
}


teSL_Status eSL_MessageWait(uint16 u16Type, uint32 u32WaitTimeout, uint16 *pu16Length, void **ppvMessage)
{
    int i;
    tsSerialLink *psSerialLink = &sSerialLink;
    
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++){
        pthread_mutex_lock(&psSerialLink->asReaderMessageQueue[i].mutex);

        if (psSerialLink->asReaderMessageQueue[i].u16Type == 0){
            struct timeval sNow;
            struct timespec sTimeout;
                    
            DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Found free slot %d to message 0x%04X\n", i, u16Type);
            psSerialLink->asReaderMessageQueue[i].u16Type = u16Type;
            if (u16Type == E_SL_MSG_STATUS){
                psSerialLink->asReaderMessageQueue[i].pu8Message = *ppvMessage;
            }

            memset(&sNow, 0, sizeof(struct timeval));
            gettimeofday(&sNow, NULL);
            sTimeout.tv_sec = sNow.tv_sec + (u32WaitTimeout/1000);
            sTimeout.tv_nsec = (sNow.tv_usec + ((u32WaitTimeout % 1000) * 1000)) * 1000;
            if (sTimeout.tv_nsec > 1000000000){
                sTimeout.tv_sec++;
                sTimeout.tv_nsec -= 1000000000;
            }
            DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Time now    %lu s, %lu ns\n", sNow.tv_sec, sNow.tv_usec * 1000);
            DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Wait until  %lu s, %lu ns\n", sTimeout.tv_sec, sTimeout.tv_nsec);
            //Waiting Msg Recv, this func will unlock asReaderMessageQueue[i].mutex!!!
            switch (pthread_cond_timedwait(&psSerialLink->asReaderMessageQueue[i].cond_data_available, 
                                                &psSerialLink->asReaderMessageQueue[i].mutex, &sTimeout))
            {
                case (0):
                    DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Got message type 0x%04x, length %d\n", 
                                    psSerialLink->asReaderMessageQueue[i].u16Type, 
                                    psSerialLink->asReaderMessageQueue[i].u16Length);
                    *pu16Length = psSerialLink->asReaderMessageQueue[i].u16Length;
                    *ppvMessage = psSerialLink->asReaderMessageQueue[i].pu8Message;
                    
                    /* Reset queue for next user */
                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                    return E_SL_OK;
                
                case (ETIMEDOUT):
                    ERR_vPrintf(T_TRUE, "Timed out for waiting msg:0x%04x\n", u16Type);
                    /* Reset queue for next user */
                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                    return E_SL_NOMESSAGE;
                    break;
                
                default:
                    /* Reset queue for next user */
                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                    return E_SL_ERROR;
            }
        }else{
            pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
        }
    }                   
    DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Error, no free queue slots\n");
    return E_SL_ERROR;
}


teSL_Status eSL_AddListener(uint16 u16Type, tprSL_MessageCallback prCallback, void *pvUser)
{
    tsSL_CallbackEntry *psCurrentEntry;
    tsSL_CallbackEntry *psNewEntry;
    
    DBG_vPrintf(DBG_SERIALLINK_CB, "Register handler %p for message type 0x%04x\n", prCallback, u16Type);
    
    psNewEntry = malloc(sizeof(tsSL_CallbackEntry));
    if (!psNewEntry)
    {
        return E_SL_ERROR_NOMEM;
    }
    
    psNewEntry->u16Type     = u16Type;
    psNewEntry->prCallback  = prCallback;
    psNewEntry->pvUser      = pvUser;
    psNewEntry->psNext      = NULL;
    
    pthread_mutex_lock(&sSerialLink.sCallbacks.mutex);
    if (sSerialLink.sCallbacks.psListHead == NULL)
    {
        /* Insert at start of list */
        sSerialLink.sCallbacks.psListHead = psNewEntry;
    }
    else
    {
        /* Insert at end of list */
        psCurrentEntry = sSerialLink.sCallbacks.psListHead;
        while (psCurrentEntry->psNext)
        {
            psCurrentEntry = psCurrentEntry->psNext;
        }
        
        psCurrentEntry->psNext = psNewEntry;
    }
    pthread_mutex_unlock(&sSerialLink.sCallbacks.mutex);
    return E_SL_OK;
}


teSL_Status eSL_RemoveListener(uint16 u16Type, tprSL_MessageCallback prCallback)
{
    tsSL_CallbackEntry *psCurrentEntry;
    tsSL_CallbackEntry *psOldEntry = NULL;
    
    DBG_vPrintf(DBG_SERIALLINK_CB, "Remove handler %p for message type 0x%04x\n", prCallback, u16Type);
    
    pthread_mutex_lock(&sSerialLink.sCallbacks.mutex);
    
    if (sSerialLink.sCallbacks.psListHead->prCallback == prCallback)
    {
        /* Start of the list */
        psOldEntry = sSerialLink.sCallbacks.psListHead;
        sSerialLink.sCallbacks.psListHead = psOldEntry->psNext;
    }
    else
    {
        psCurrentEntry = sSerialLink.sCallbacks.psListHead;
        while (psCurrentEntry->psNext)
        {
            if (psCurrentEntry->psNext->prCallback == prCallback)
            {
                psOldEntry = psCurrentEntry->psNext;
                psCurrentEntry->psNext = psCurrentEntry->psNext->psNext;
                break;
            }
        }
    }
    pthread_mutex_unlock(&sSerialLink.sCallbacks.mutex);
    
    if (!psOldEntry)
    {
        DBG_vPrintf(DBG_SERIALLINK_CB, "Entry not found\n");
        return E_SL_ERROR;
    }
    
    /* Free removed entry from list */
    free(psOldEntry);
    return E_SL_OK;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


static teSL_Status eSL_ReadMessage(uint16 *pu16Type, uint16 *pu16Length, uint16 u16MaxLength, uint8 *pu8Message)
{

    static teSL_RxState eRxState = E_STATE_RX_WAIT_START;
    static uint8 u8CRC;
    uint8 u8Data;
    static uint16 u16Bytes;
    static bool_t bInEsc = T_FALSE;

    while(bSL_RxByte(&u8Data))
    {
        DBG_vPrintf(DBG_SERIALLINK_COMMS, "0x%02x\n", u8Data);
        switch(u8Data)
        {

        case SL_START_CHAR:
            u16Bytes = 0;
            bInEsc = T_FALSE;
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "RX Start\n");
            eRxState = E_STATE_RX_WAIT_TYPEMSB;
            break;

        case SL_ESC_CHAR:
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "Got ESC\n");
            bInEsc = T_TRUE;
            break;

        case SL_END_CHAR:
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "Got END\n");
            
            if(*pu16Length > u16MaxLength)
            {
                /* Sanity check length before attempting to CRC the message */
                DBG_vPrintf(DBG_SERIALLINK_COMMS, "Length > MaxLength\n");
                eRxState = E_STATE_RX_WAIT_START;
                break;
            }
            
            if(u8CRC == u8SL_CalculateCRC(*pu16Type, *pu16Length, pu8Message))
            {
#if DBG_SERIALLINK
                int i;
                DBG_vPrintf(DBG_SERIALLINK, "RX Message type 0x%04x length %d: { ", *pu16Type, *pu16Length);
                for (i = 0; i < *pu16Length; i++)
                {
                    printf("0x%02x ", pu8Message[i]);
                }
                printf("}\n");
#endif /* DBG_SERIALLINK */
                
                eRxState = E_STATE_RX_WAIT_START;
                return E_SL_OK;
            }
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "CRC BAD\n");
            break;

        default:
            if(bInEsc)
            {
                u8Data ^= 0x10;
                bInEsc = T_FALSE;
            }

            switch(eRxState)
            {

                case E_STATE_RX_WAIT_START:
                    break;
                    

                case E_STATE_RX_WAIT_TYPEMSB:
                    *pu16Type = (uint16)u8Data << 8;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_TYPELSB:
                    *pu16Type += (uint16)u8Data;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_LENMSB:
                    *pu16Length = (uint16)u8Data << 8;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_LENLSB:
                    *pu16Length += (uint16)u8Data;
                    DBG_vPrintf(DBG_SERIALLINK_COMMS, "Length %d\n", *pu16Length);
                    if(*pu16Length > u16MaxLength)
                    {
                        DBG_vPrintf(DBG_SERIALLINK_COMMS, "Length > MaxLength\n");
                        eRxState = E_STATE_RX_WAIT_START;
                    }
                    else
                    {
                        eRxState++;
                    }
                    break;

                case E_STATE_RX_WAIT_CRC:
                    DBG_vPrintf(DBG_SERIALLINK_COMMS, "CRC %02x\n", u8Data);
                    u8CRC = u8Data;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_DATA:
                    if(u16Bytes < *pu16Length)
                    {
                        DBG_vPrintf(DBG_SERIALLINK_COMMS, "Data\n");
                        pu8Message[u16Bytes++] = u8Data;
                    }
                    break;

                default:
                    DBG_vPrintf(DBG_SERIALLINK_COMMS, "Unknown state\n");
                    eRxState = E_STATE_RX_WAIT_START;
            }
            break;

        }

    }

    return E_SL_NOMESSAGE;
}


/****************************************************************************
 *
 * NAME: vSL_WriteRawMessage
 *
 * DESCRIPTION:
 *
 * PARAMETERS: Name        RW  Usage
 *
 * RETURNS:
 * void
 ****************************************************************************/
static teSL_Status eSL_WriteMessage(uint16 u16Type, uint16 u16Length, uint8 *pu8Data)
{
    int n;
    uint8 u8CRC;

    u8CRC = u8SL_CalculateCRC(u16Type, u16Length, pu8Data);

    DBG_vPrintf(DBG_SERIALLINK_COMMS, "(%d, %d, %02x)\n", u16Type, u16Length, u8CRC);

    if (verbosity >= 10)
    {
        char acBuffer[4096];
        int iPosition = 0, i;
        
        iPosition = sprintf(&acBuffer[iPosition], "Host->Node 0x%04X (Length % 4d)", u16Type, u16Length);
        for (i = 0; i < u16Length; i++)
        {
            iPosition += sprintf(&acBuffer[iPosition], " 0x%02X", pu8Data[i]);
        }
        INF_vPrintf(T_TRUE, "%s\n", acBuffer);
    }
    /* Send start character */
    if (iSL_TxByte(T_TRUE, SL_START_CHAR) < 0) return E_SL_ERROR;

    /* Send message type */
    if (iSL_TxByte(T_FALSE, (u16Type >> 8) & 0xff) < 0) return E_SL_ERROR;
    if (iSL_TxByte(T_FALSE, (u16Type >> 0) & 0xff) < 0) return E_SL_ERROR;

    /* Send message length */
    if (iSL_TxByte(T_FALSE, (u16Length >> 8) & 0xff) < 0) return E_SL_ERROR;
    if (iSL_TxByte(T_FALSE, (u16Length >> 0) & 0xff) < 0) return E_SL_ERROR;

    /* Send message checksum */
    if (iSL_TxByte(T_FALSE, u8CRC) < 0) return E_SL_ERROR;

    /* Send message payload */  
    for(n = 0; n < u16Length; n++)
    {       
        if (iSL_TxByte(T_FALSE, pu8Data[n]) < 0) return E_SL_ERROR;
    }

    /* Send end character */
    if (iSL_TxByte(T_TRUE, SL_END_CHAR) < 0) return E_SL_ERROR;

    return E_SL_OK;
}

static uint8 u8SL_CalculateCRC(uint16 u16Type, uint16 u16Length, uint8 *pu8Data)
{
    int n;
    uint8 u8CRC = 0;

    u8CRC ^= (u16Type >> 8) & 0xff;
    u8CRC ^= (u16Type >> 0) & 0xff;
    
    u8CRC ^= (u16Length >> 8) & 0xff;
    u8CRC ^= (u16Length >> 0) & 0xff;

    for(n = 0; n < u16Length; n++)
    {
        u8CRC ^= pu8Data[n];
    }
    return(u8CRC);
}

/****************************************************************************
 *
 * NAME: vSL_TxByte
 *
 * DESCRIPTION:
 *
 * PARAMETERS:  Name                RW  Usage
 *
 * RETURNS:
 * void
 ****************************************************************************/
static int iSL_TxByte(bool_t bSpecialCharacter, uint8 u8Data)
{
    if(!bSpecialCharacter && (u8Data < 0x10))
    {
        u8Data ^= 0x10;

        if (eSerial_Write(SL_ESC_CHAR) != E_SERIAL_OK) return -1;
        //DBG_vPrintf(DBG_SERIALLINK_COMMS, " 0x%02x", SL_ESC_CHAR);
    }
    //DBG_vPrintf(DBG_SERIALLINK_COMMS, " 0x%02x", u8Data);

    return eSerial_Write(u8Data);
}


/****************************************************************************
 *
 * NAME: bSL_RxByte
 *
 * DESCRIPTION:
 *
 * PARAMETERS:  Name                RW  Usage
 *
 * RETURNS:
 * void
 ****************************************************************************/
static bool_t bSL_RxByte(uint8 *pu8Data)
{
    if (eSerial_Read(pu8Data) == E_SERIAL_OK)
    {
        return T_TRUE;
    }
    else
    {
        return T_FALSE;
    }
}


static teSL_Status eSL_MessageQueue(tsSerialLink *psSerialLink, uint16 u16Type, uint16 u16Length, uint8 *pu8Message)
{
    int i;
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    {
        pthread_mutex_lock(&psSerialLink->asReaderMessageQueue[i].mutex);

        if (psSerialLink->asReaderMessageQueue[i].u16Type == u16Type)
        {        
            DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Found listener for message type 0x%04x in slot %d\n", u16Type, i);
                
            if (u16Type == E_SL_MSG_STATUS)
            {
                tsSL_Msg_Status *psRxStatus = (tsSL_Msg_Status*)pu8Message;
                tsSL_Msg_Status *psWaitStatus = (tsSL_Msg_Status*)psSerialLink->asReaderMessageQueue[i].pu8Message;
                
                /* Also check the type of the message that this is status to. */
                if (psWaitStatus)
                {
                    DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Status listener for message type 0x%04X, rx 0x%04X\n", psWaitStatus->u16MessageType, ntohs(psRxStatus->u16MessageType));
                    
                    if (psWaitStatus->u16MessageType != ntohs(psRxStatus->u16MessageType))
                    {
                        DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Not the status listener for this message\n");
                        pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                        continue;
                    }
                }
            }
            
            uint8  *pu8MessageCopy = malloc(u16Length);
            if (!pu8MessageCopy)
            {
                ERR_vPrintf(T_TRUE, "Memory allocation failure");
                pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);//PCT
                return E_SL_ERROR_NOMEM;
            }
            memcpy(pu8MessageCopy, pu8Message, u16Length);
            psSerialLink->asReaderMessageQueue[i].u16Length = u16Length;
            psSerialLink->asReaderMessageQueue[i].pu8Message = pu8MessageCopy;

            /* Signal data available */
            pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
            pthread_cond_broadcast(&psSerialLink->asReaderMessageQueue[i].cond_data_available);
            return E_SL_OK;
        }
        else
        {
            pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
        }
    }
    DBG_vPrintf(DBG_SERIALLINK_QUEUE, "No listeners for message type 0x%04X\n", u16Type);
    return E_SL_NOMESSAGE;
}


static void *pvSerialReaderThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSerialLink *psSerialLink = (tsSerialLink *)psThreadInfo->pvThreadData;
    tsSL_Message  sMessage;
    int iHandled;

    DBG_vPrintf(DBG_SERIALLINK, "pvReaderThread Starting\n");

    psThreadInfo->eState = E_THREAD_RUNNING;

    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        /* Initialise buffer */
        memset(&sMessage, 0, sizeof(tsSL_Message));
        /* Initialise length to large value so CRC is skipped if end received */
        sMessage.u16Length = 0xFFFF;

        if (eSL_ReadMessage(&sMessage.u16Type, &sMessage.u16Length, SL_MAX_MESSAGE_LENGTH, sMessage.au8Message) == E_SL_OK)
        {
            iHandled = 0;

            if (verbosity >= 10){
                char acBuffer[4096];
                int iPosition = 0, i;

                iPosition = sprintf(&acBuffer[iPosition], "Node->Host 0x%04X (Length % 4d)", sMessage.u16Type, sMessage.u16Length);
                for (i = 0; i < sMessage.u16Length; i++){
                    iPosition += sprintf(&acBuffer[iPosition], " 0x%02X", sMessage.au8Message[i]);
                }
                NOT_vPrintf(T_TRUE, "%s\n", acBuffer);
            }
            
            // Search for callback handlers foe this message type
            tsSL_CallbackEntry *psCurrentEntry;
            pthread_mutex_lock(&psSerialLink->sCallbacks.mutex);
            for (psCurrentEntry = psSerialLink->sCallbacks.psListHead; psCurrentEntry; psCurrentEntry = psCurrentEntry->psNext)
            {
                if (psCurrentEntry->u16Type == sMessage.u16Type)
                {
                    tsCallbackThreadData *psCallbackData;
                    DBG_vPrintf(DBG_SERIALLINK_CB, "Found callback routine %p for message 0x%04x\n", psCurrentEntry->prCallback, sMessage.u16Type);

                    // Put the message into the queue for the callback handler thread
                    psCallbackData = malloc(sizeof(tsCallbackThreadData));
                    if (!psCallbackData){
                        ERR_vPrintf(T_TRUE, "Memory allocation error\n");
                    }else{
                        memcpy(&psCallbackData->sMessage, &sMessage, sizeof(tsSL_Message));
                        psCallbackData->prCallback = psCurrentEntry->prCallback;
                        psCallbackData->pvUser = psCurrentEntry->pvUser;

                        if (mQueueEnqueue(&psSerialLink->sCallbackQueue, psCallbackData) == E_QUEUE_OK){
                            iHandled = 1;
                        }else{
                            ERR_vPrintf(T_TRUE, "Failed to queue message for callback\n");
                            free(psCallbackData);
                        }
                    }
                }
            }
            pthread_mutex_unlock(&sSerialLink.sCallbacks.mutex);
            if(iHandled){
                continue;
            }

            if (sMessage.u16Type == E_SL_MSG_LOG){//Log doesn't handle in thread
                /* Log messages handled here first, and passsed to new thread in case user has added another handler */
                uint8 u8LogLevel = sMessage.au8Message[0];
                char *pcMessage = (char *)&sMessage.au8Message[1];
                sMessage.au8Message[sMessage.u16Length] = '\0';
                NOT_vPrintf(u8LogLevel, "Module: %s\n", pcMessage);
                iHandled = 1; /* Message handled by logger */
            }else{
                //Send Queue to Handler
                tsSL_Message *pMessageQueue;
                pMessageQueue = (tsSL_Message *)malloc(sizeof(tsSL_Message));
                if(pMessageQueue){
                    memcpy(pMessageQueue, &sMessage, sizeof(tsSL_Message));
                    if (mQueueEnqueue(&psSerialLink->sMessageQueue, pMessageQueue) == E_QUEUE_OK){
                        WAR_vPrintf(DBG_SERIALLINK_QUEUE, "Set Queue Message:0x%04x to MessageHandleThread\n", pMessageQueue->u16Type);
                        iHandled = 1;
                    }else{
                        ERR_vPrintf(T_TRUE, "Failed to queue message for callback\n");
                        free(pMessageQueue);
                    }
                }else{
                    ERR_vPrintf(T_TRUE, "Memory allocation error\n");
                }
            }
        
            if (!iHandled){
                ERR_vPrintf(T_TRUE, "Message 0x%04X was not handled\n", sMessage.u16Type);
            }
        }
    }

    int i;
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++){
        psSerialLink->asReaderMessageQueue[i].u16Length  = 0;
        psSerialLink->asReaderMessageQueue[i].pu8Message = NULL;
        pthread_cond_broadcast(&psSerialLink->asReaderMessageQueue[i].cond_data_available);
    }

    DBG_vPrintf(DBG_SERIALLINK, "Exit\n");

    /* Return from thread clearing resources */
    mThreadFinish(psThreadInfo);
    return NULL;
}


static void *pvCallbackHandlerThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSerialLink *psSerialLink = (tsSerialLink *)psThreadInfo->pvThreadData;

    DBG_vPrintf(DBG_SERIALLINK, "Starting\n");
    
    psThreadInfo->eState = E_THREAD_RUNNING;
    
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        tsCallbackThreadData *psCallbackData;
        
        if (mQueueDequeue(&psSerialLink->sCallbackQueue, (void**)&psCallbackData) == E_QUEUE_OK)
        {
            DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Calling callback %p for message 0x%04X\n", psCallbackData->prCallback, psCallbackData->sMessage.u16Type);
            psCallbackData->prCallback(psCallbackData->pvUser, psCallbackData->sMessage.u16Length, psCallbackData->sMessage.au8Message);
            free(psCallbackData);
        }
    }
    DBG_vPrintf(DBG_SERIALLINK, "Exit\n");
    
    /* Return from thread clearing resources */
    mThreadFinish(psThreadInfo);
    return NULL;
}

static void *pvMessageQueueHandlerThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSerialLink *psSerialLink = (tsSerialLink *)psThreadInfo->pvThreadData;

    DBG_vPrintf(DBG_SERIALLINK, "pvMessageQueueHandlerThread Starting\n");
    
    psThreadInfo->eState = E_THREAD_RUNNING;
    
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        tsSL_Message *psMessageData;
        
        DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Waiting Message from SerialLinkThread\n");
        if(mQueueDequeue(&psSerialLink->sMessageQueue, (void**)&psMessageData) == E_QUEUE_OK)
        {
            WAR_vPrintf(DBG_SERIALLINK_QUEUE, "Get Queue Message:0x%04x from SerialLinkThread:", psMessageData->u16Type);
            if(psMessageData->u16Type == E_SL_MSG_STATUS){
                tsSL_Msg_Status *sStatus;
                sStatus = (tsSL_Msg_Status *)psMessageData->au8Message;
                WAR_vPrintf(DBG_SERIALLINK_QUEUE, "0x%04x", sStatus->u16MessageType);
            }
            printf("\n");
            for(int i = 0; i < 5; i++){
                // See if any threads are waiting for this message
                teSL_Status eStatus = eSL_MessageQueue(psSerialLink, psMessageData->u16Type, psMessageData->u16Length, psMessageData->au8Message);
                if (eStatus == E_SL_OK)
                {
                    DBG_vPrintf(DBG_SERIALLINK_QUEUE, "Message Brocast Successful\n");
                    break;
                }
                else //No Waiting or Error
                {
                    WAR_vPrintf(DBG_SERIALLINK_QUEUE, "No listeners for message type 0x%04X,:", psMessageData->u16Type);
                    if(psMessageData->u16Type == E_SL_MSG_STATUS){
                        tsSL_Msg_Status *sStatus;
                        sStatus = (tsSL_Msg_Status *)psMessageData->au8Message;
                        WAR_vPrintf(DBG_SERIALLINK_QUEUE, "0x%04x", sStatus->u16MessageType);
                    }
                    printf("\n");
                    usleep(20000);
                    continue;
                }
            }
            free(psMessageData);
        }
    }
    
    DBG_vPrintf(DBG_SERIALLINK, "Exit\n");
    
    /* Return from thread clearing resources */
    mThreadFinish(psThreadInfo);
    return NULL;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

