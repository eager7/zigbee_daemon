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
#define DBG_SERIAL_LINK (verbosity >= 6)
#define DBG_SERIAL_LINK_CB (verbosity >= 12)
#define DBG_SERIAL_LINK_COMM (verbosity >= 12)
#define DBG_SERIAL_LINK_QUEUE 0

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
static tsSerialLink     sSerialLink;            /*Thread for serial read*/
static tsSL_CallBack    sSL_CallBack;
static tsSL_MsgBrocast  sSL_MsgBroadcast;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teSL_Status eSL_Init(char *cpSerialDevice, uint32 u32BaudRate)
{
    CHECK_RESULT(eSerial_Init(cpSerialDevice, u32BaudRate, &sSerialLink.iSerialFd), E_SERIAL_OK, E_SL_ERROR_SERIAL);

    int i = 0;
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++){
        pthread_mutex_init(&sSerialLink.asReaderMessageQueue[i].mutex, NULL);
        pthread_cond_init(&sSerialLink.asReaderMessageQueue[i].cond_data_available, NULL);
        sSerialLink.asReaderMessageQueue[i].u16Type = 0;
    }
    
    /* Start the callback handler thread */
    sSL_CallBack.sThread.pvThreadData = &sSL_CallBack;
    dl_list_init(&sSL_CallBack.sCallbackHead.list);  //Init List Head Node
    CHECK_RESULT(eLockCreate(&sSL_CallBack.sCallbackHead.mutex), E_THREAD_OK, E_SL_ERROR);   //mutex for callback be called
    CHECK_RESULT(eQueueCreate(&sSL_CallBack.sQueue, SL_MAX_CALLBACK_QUEUES), E_THREAD_OK, E_SL_ERROR);
    CHECK_RESULT(eThreadStart(pvCallbackHandlerThread, &sSL_CallBack.sThread, E_THREAD_DETACHED), E_THREAD_OK, E_SL_ERROR);
    
    /* Start the message queue handler thread */
    sSL_MsgBroadcast.sThread.pvThreadData = &sSL_MsgBroadcast;
    CHECK_RESULT(eQueueCreate(&sSL_MsgBroadcast.sQueue, SL_MAX_MESSAGE_QUEUES), E_THREAD_OK, E_SL_ERROR);
    CHECK_RESULT(eThreadStart(pvMessageQueueHandlerThread, &sSL_MsgBroadcast.sThread, E_THREAD_DETACHED), E_THREAD_OK, E_SL_ERROR);

    /* Start the Serial thread */
    sSerialLink.sSerialReaderThread.pvThreadData = &sSerialLink;
    CHECK_RESULT(eLockCreate(&sSerialLink.mutex_write), E_THREAD_OK, E_SL_ERROR);   //mutex for uart write
    CHECK_RESULT(eThreadStart(pvSerialReaderThread, &sSerialLink.sSerialReaderThread, E_THREAD_DETACHED), E_THREAD_OK, E_SL_ERROR);
    
    DBG_vPrintln(T_TRUE, "eSL_Init OK\n");
    return E_SL_OK;
}

teSL_Status eSL_Destroy(void)
{
    eThreadStop(&sSL_CallBack.sThread);
    eQueueDestroy(&sSL_CallBack.sQueue);

    eThreadStop(&sSL_MsgBroadcast.sThread);
    eQueueDestroy(&sSL_MsgBroadcast.sQueue);

    eThreadStop(&sSerialLink.sSerialReaderThread);
 
    return E_SL_OK;
}

teSL_Status eSL_SendMessage(uint16 u16Type, uint16 u16Length, void *pvMessage, uint8 *pu8SequenceNo)
{
    teSL_Status eStatus;
    
    /* Make sure there is only one thread sending messages to the node at a time. */
    eLockLock(&sSerialLink.mutex_write);
    
    eStatus = eSL_WriteMessage(u16Type, u16Length, (uint8 *)pvMessage);
    if (eStatus == E_SL_OK)/* Command sent successfully */
    {
        uint16    u16Len;
        tsSL_Msg_Status sStatus;
        tsSL_Msg_Status *psStatus = &sStatus;
        
        psStatus->u16MessageType = u16Type;
        eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 100, &u16Len, (void**)&psStatus);/* Expect a status response within 100ms */
        if (eStatus == E_SL_OK)
        {
            DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Status: %d, Sequence %d\n", psStatus->eStatus, psStatus->u8SequenceNo);
            eStatus = (teSL_Status)psStatus->eStatus;
            if (eStatus == E_SL_OK){
                if (pu8SequenceNo){
                    *pu8SequenceNo = psStatus->u8SequenceNo;
                }
            }
            FREE(psStatus);//malloc in eSL_MessageQueue
        }
    }
    eLockunLock(&sSerialLink.mutex_write);
    return eStatus;
}

teSL_Status eSL_MessageWait(uint16 u16Type, uint32 u32WaitTimeout, uint16 *pu16Length, void **ppvMessage)
{
    int i;
    tsSerialLink *psSerialLink = &sSerialLink;
    
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++){
        eLockLock(&psSerialLink->asReaderMessageQueue[i].mutex);

        if (psSerialLink->asReaderMessageQueue[i].u16Type == 0){
            struct timeval sNow;
            struct timespec sTimeout;
                    
            DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Found free slot %d to message 0x%04X\n", i, u16Type);
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
            DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Time now    %lu s, %lu ns\n", sNow.tv_sec, sNow.tv_usec * 1000);
            DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Wait until  %lu s, %lu ns\n", sTimeout.tv_sec, sTimeout.tv_nsec);
            //Waiting Msg Recv, this func will unlock asReaderMessageQueue[i].mutex!!!
            switch (pthread_cond_timedwait(&psSerialLink->asReaderMessageQueue[i].cond_data_available, 
                                                &psSerialLink->asReaderMessageQueue[i].mutex, &sTimeout))
            {
                case (0):
                    DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Got message type 0x%04x, length %d\n",
                                    psSerialLink->asReaderMessageQueue[i].u16Type, 
                                    psSerialLink->asReaderMessageQueue[i].u16Length);
                    *pu16Length = psSerialLink->asReaderMessageQueue[i].u16Length;
                    *ppvMessage = psSerialLink->asReaderMessageQueue[i].pu8Message;
                    
                    /* Reset queue for next user */
                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                    return E_SL_OK;
                
                case (ETIMEDOUT):
                    ERR_vPrintln(T_TRUE, "Timed out for waiting msg:0x%04x\n", u16Type);
                    /* Reset queue for next user */
                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                    return E_SL_NOMESSAGE;

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
    DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Error, no free queue slots\n");
    return E_SL_ERROR;
}

teSL_Status eSL_AddListener(uint16 u16Type, tprSL_MessageCallback prCallback, void *pvUser)
{
    tsSL_CallbackEntry *psNewEntry;
    
    DBG_vPrintln(DBG_SERIAL_LINK_CB, "Register handler %p for message type 0x%04x\n", prCallback, u16Type);
    
    psNewEntry = malloc(sizeof(tsSL_CallbackEntry));
    memset(psNewEntry, 0, sizeof(tsSL_CallbackEntry));
    if (!psNewEntry){
        return E_SL_ERROR_NOMEM;
    }
    psNewEntry->u16Type     = u16Type;
    psNewEntry->prCallback  = prCallback;
    psNewEntry->pvUser      = pvUser;
    CHECK_RESULT(eLockCreate(&psNewEntry->mutex), E_THREAD_OK, E_SL_ERROR);
    dl_list_add(&sSL_CallBack.sCallbackHead.list, &psNewEntry->list);
    
    return E_SL_OK;
}

teSL_Status eSL_RemoveListener(uint16 u16Type, tprSL_MessageCallback prCallback)
{
    DBG_vPrintln(DBG_SERIAL_LINK_CB, "Remove handler %p for message type 0x%04x\n", prCallback, u16Type);
    
    tsSL_CallbackEntry *psCurrentEntryTemp1, *psCurrentEntryTemp2;
    dl_list_for_each_safe(psCurrentEntryTemp1, psCurrentEntryTemp2, &sSL_CallBack.sCallbackHead.list, tsSL_CallbackEntry, list){
        if(u16Type == psCurrentEntryTemp1->u16Type){
            dl_list_del(&psCurrentEntryTemp1->list);
            free(psCurrentEntryTemp1); psCurrentEntryTemp1 = NULL;
        }
    }
    return E_SL_OK;
}

teSL_Status eSL_RemoveAllListener(void)
{
    tsSL_CallbackEntry *psCurrentEntryTemp1, *psCurrentEntryTemp2;
    dl_list_for_each_safe(psCurrentEntryTemp1, psCurrentEntryTemp2, &sSL_CallBack.sCallbackHead.list, tsSL_CallbackEntry, list){
        dl_list_del(&psCurrentEntryTemp1->list);
        free(psCurrentEntryTemp1); psCurrentEntryTemp1 = NULL;
    }    
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
        DBG_vPrintln(DBG_SERIAL_LINK_COMM, "0x%02x\n", u8Data);
        switch(u8Data)
        {
        case SL_START_CHAR:
            u16Bytes = 0;
            bInEsc = T_FALSE;
            DBG_vPrintln(DBG_SERIAL_LINK_COMM, "RX Start\n");
            eRxState = E_STATE_RX_WAIT_TYPEMSB;
            break;

        case SL_ESC_CHAR:
            DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Got ESC\n");
            bInEsc = T_TRUE;
            break;

        case SL_END_CHAR:
            DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Got END\n");
            if(*pu16Length > u16MaxLength) {
                /* Sanity check length before attempting to CRC the message */
                DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Length > MaxLength\n");
                eRxState = E_STATE_RX_WAIT_START;
                break;
            }
            
            if(u8CRC == u8SL_CalculateCRC(*pu16Type, *pu16Length, pu8Message)) {

                int i;
                DBG_vPrintln(DBG_SERIAL_LINK_COMM, "RX Message type 0x%04x length %d: { ", *pu16Type, *pu16Length);
                if(DBG_SERIAL_LINK_COMM){
                    for (i = 0; i < *pu16Length; i++) {
                        printf("0x%02x ", pu8Message[i]);
                    }
                    printf("}\n");
                }
                
                eRxState = E_STATE_RX_WAIT_START;
                return E_SL_OK;
            }
            DBG_vPrintln(DBG_SERIAL_LINK_COMM, "CRC BAD\n");
            break;

        default:
            if(bInEsc) {
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
                    DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Length %d\n", *pu16Length);
                    if(*pu16Length > u16MaxLength) {
                        DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Length > MaxLength\n");
                        eRxState = E_STATE_RX_WAIT_START;
                    } else {
                        eRxState++;
                    }
                    break;

                case E_STATE_RX_WAIT_CRC:
                    DBG_vPrintln(DBG_SERIAL_LINK_COMM, "CRC %02x\n", u8Data);
                    u8CRC = u8Data;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_DATA:
                    if(u16Bytes < *pu16Length) {
                        DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Data\n");
                        pu8Message[u16Bytes++] = u8Data;
                    }
                    break;

                default:
                    DBG_vPrintln(DBG_SERIAL_LINK_COMM, "Unknown state\n");
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

    DBG_vPrintln(DBG_SERIAL_LINK_COMM, "(%d, %d, %02x)\n", u16Type, u16Length, u8CRC);

    if (DBG_SERIAL_LINK)
    {
        char acBuffer[4096];
        int iPosition = 0, i;
        
        iPosition = sprintf(&acBuffer[iPosition], "Host->Node 0x%04X (Length % 4d)", u16Type, u16Length);
        for (i = 0; i < u16Length; i++)
        {
            iPosition += sprintf(&acBuffer[iPosition], " 0x%02X", pu8Data[i]);
        }
        INF_vPrintln(DBG_SERIAL_LINK, "%s\n", acBuffer);
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

    for(n = 0; n < u16Length; n++) {
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
    if(!bSpecialCharacter && (u8Data < 0x10)) {
        u8Data ^= 0x10;
        if (eSerial_Write(SL_ESC_CHAR) != E_SERIAL_OK) return -1;
    }

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
    if (eSerial_Read(pu8Data) == E_SERIAL_OK) {
        return T_TRUE;
    } else {
        return T_FALSE;
    }
}

static teSL_Status eSL_MessageQueue(tsSerialLink *psSerialLink, uint16 u16Type, uint16 u16Length, uint8 *pu8Message)
{
    int i;
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    {
        pthread_mutex_lock(&psSerialLink->asReaderMessageQueue[i].mutex);

        if (psSerialLink->asReaderMessageQueue[i].u16Type == u16Type) {
            DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Found listener for message type 0x%04x in slot %d\n", u16Type, i);
                
            if (u16Type == E_SL_MSG_STATUS) {
                tsSL_Msg_Status *psRxStatus = (tsSL_Msg_Status*)pu8Message;
                tsSL_Msg_Status *psWaitStatus = (tsSL_Msg_Status*)psSerialLink->asReaderMessageQueue[i].pu8Message;
                
                /* Also check the type of the message that this is status to. */
                if (psWaitStatus) {
                    DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Status listener for message type 0x%04X, rx 0x%04X\n", psWaitStatus->u16MessageType, ntohs(psRxStatus->u16MessageType));
                    
                    if (psWaitStatus->u16MessageType != ntohs(psRxStatus->u16MessageType)) {
                        DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Not the status listener for this message\n");
                        pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                        continue;
                    }
                }
            }
            
            uint8  *pu8MessageCopy = malloc(u16Length);
            memset(pu8MessageCopy, 0, u16Length);
            if (!pu8MessageCopy) {
                ERR_vPrintln(T_TRUE, "Memory allocation failure");
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
        } else {
            pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
        }
    }
    DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "No listeners for message type 0x%04X\n", u16Type);
    return E_SL_NOMESSAGE;
}

static void *pvSerialReaderThread(void *psThreadInfoVoid)
{
    DBG_vPrintln(DBG_SERIAL_LINK, "pvReaderThread Starting\n");
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSerialLink *psSerialLink = (tsSerialLink *)psThreadInfo->pvThreadData;
    tsSL_Message  sMessage;
    int iHandled;

    psThreadInfo->eState = E_THREAD_RUNNING;
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        memset(&sMessage, 0, sizeof(tsSL_Message));
        /* Initialise length to large value so CRC is skipped if end received */
        sMessage.u16Length = 0xFFFF;

        if (eSL_ReadMessage(&sMessage.u16Type, &sMessage.u16Length, SL_MAX_MESSAGE_LENGTH, sMessage.au8Message) == E_SL_OK) {
            iHandled = 0;
            if (DBG_SERIAL_LINK){
                char acBuffer[4096];
                int iPosition = 0, i;
                iPosition = sprintf(&acBuffer[iPosition], "Node->Host 0x%04X (Length % 4d)", sMessage.u16Type, sMessage.u16Length);
                for (i = 0; i < sMessage.u16Length; i++){
                    iPosition += sprintf(&acBuffer[iPosition], " 0x%02X", sMessage.au8Message[i]);
                }
                NOT_vPrintln(DBG_SERIAL_LINK, "%s\n", acBuffer);
            }
            
            // Search for callback handlers foe this message type
            tsSL_CallbackEntry *psCurrentEntry;
            dl_list_for_each(psCurrentEntry, &sSL_CallBack.sCallbackHead.list, tsSL_CallbackEntry, list){
                if (psCurrentEntry->u16Type == sMessage.u16Type){
                    tsCallbackThreadData *psCallbackData;
                    DBG_vPrintln(DBG_SERIAL_LINK_CB, "Found callback routine %p for message 0x%04x\n", psCurrentEntry->prCallback, sMessage.u16Type);
                    
                    psCallbackData = malloc(sizeof(tsCallbackThreadData));
                    memset(psCallbackData, 0, sizeof(tsCallbackThreadData));
                    if (!psCallbackData){
                        ERR_vPrintln(T_TRUE, "Memory allocation error\n");
                    } else {
                        memcpy(&psCallbackData->sMessage, &sMessage, sizeof(tsSL_Message));
                        psCallbackData->prCallback = psCurrentEntry->prCallback;
                        psCallbackData->pvUser = psCurrentEntry->pvUser;
                        // Put the message into the queue for the callback handler thread
                        if (eQueueEnqueue(&sSL_CallBack.sQueue, psCallbackData) == E_THREAD_OK){
                            iHandled = 1;
                        } else {
                            ERR_vPrintln(T_TRUE, "Failed to queue message for callback\n");
                            FREE(psCallbackData);
                        }
                    }
                }
            }
            if(iHandled){
                continue;
            }

            if (sMessage.u16Type == E_SL_MSG_LOG){//Log doesn't handle in thread
                /* Log messages handled here first, and passsed to new thread in case user has added another handler */
                uint8 u8LogLevel = sMessage.au8Message[0];
                char *pcMessage = (char *)&sMessage.au8Message[1];
                sMessage.au8Message[sMessage.u16Length] = '\0';
                NOT_vPrintln(u8LogLevel, "Module: %s\n", pcMessage);
                iHandled = 1; /* Message handled by logger */
            } else {
                //Send Queue to Handler
                tsSL_Message *pMessageQueue;
                pMessageQueue = (tsSL_Message *)malloc(sizeof(tsSL_Message));
                memset(pMessageQueue, 0, sizeof(tsSL_Message));
                if(pMessageQueue){
                    memcpy(pMessageQueue, &sMessage, sizeof(tsSL_Message));
                    if (eQueueEnqueue(&sSL_MsgBroadcast.sQueue, pMessageQueue) == E_THREAD_OK){
                        WAR_vPrintln(DBG_SERIAL_LINK_QUEUE, "Set Queue Message:0x%04x to MessageHandleThread\n", pMessageQueue->u16Type);
                        iHandled = 1;
                    } else {
                        ERR_vPrintln(T_TRUE, "Failed to queue message for callback\n");
                        free(pMessageQueue);
                    }
                } else {
                    ERR_vPrintln(T_TRUE, "Memory allocation error\n");
                }
            }
        
            if (!iHandled){
                ERR_vPrintln(T_TRUE, "Message 0x%04X was not handled\n", sMessage.u16Type);
            }
        }
    }
    int i = 0;
    for(i = 0; i < SL_MAX_MESSAGE_QUEUES; i++){
        psSerialLink->asReaderMessageQueue[i].u16Length  = 0;
        psSerialLink->asReaderMessageQueue[i].pu8Message = NULL;
        pthread_cond_broadcast(&psSerialLink->asReaderMessageQueue[i].cond_data_available);
    }

    DBG_vPrintln(DBG_SERIAL_LINK, "pvSerialReaderThread Exit\n");

    /* Return from thread clearing resources */
    vThreadFinish(psThreadInfo);
    return NULL;
}

static void *pvCallbackHandlerThread(void *psThreadInfoVoid)
{
    DBG_vPrintln(DBG_SERIAL_LINK, "pvCallbackHandlerThread Starting\n");
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSL_CallBack *psSL_CallBack = (tsSL_CallBack *)psThreadInfo->pvThreadData;

    psThreadInfo->eState = E_THREAD_RUNNING;
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        tsCallbackThreadData *psCallbackData;
        
        if (eQueueDequeue(&psSL_CallBack->sQueue, (void**)&psCallbackData) == E_THREAD_OK) {
            if(psCallbackData->prCallback) {
                DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Calling callback %p for message 0x%04X\n", psCallbackData->prCallback, psCallbackData->sMessage.u16Type);
                psCallbackData->prCallback(psCallbackData->pvUser, psCallbackData->sMessage.u16Length, psCallbackData->sMessage.au8Message);
            }
            free(psCallbackData);
        }
    }
    DBG_vPrintln(DBG_SERIAL_LINK, "pvCallbackHandlerThread Exit\n");
    
    /* Return from thread clearing resources */
    vThreadFinish(psThreadInfo);
    return NULL;
}

static void *pvMessageQueueHandlerThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsSL_MsgBrocast *psSL_MsgBrocast = (tsSL_MsgBrocast *)psThreadInfo->pvThreadData;

    DBG_vPrintln(DBG_SERIAL_LINK, "pvMessageQueueHandlerThread Starting\n");
    
    psThreadInfo->eState = E_THREAD_RUNNING;
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        tsSL_Message *psMessageData;
        
        DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Waiting Message from SerialLinkThread\n");
        if(eQueueDequeue(&psSL_MsgBrocast->sQueue, (void**)&psMessageData) == E_THREAD_OK) {
            WAR_vPrintln(DBG_SERIAL_LINK_QUEUE, "Get Queue Message:0x%04x from SerialLinkThread:", psMessageData->u16Type);
            if(psMessageData->u16Type == E_SL_MSG_STATUS){
                tsSL_Msg_Status *sStatus;
                sStatus = (tsSL_Msg_Status *)psMessageData->au8Message;
                WAR_vPrintln(DBG_SERIAL_LINK_QUEUE, "0x%04x", sStatus->u16MessageType);
            }
            WAR_vPrintln(DBG_SERIAL_LINK_QUEUE,"\n");
            int i = 0;
            for(i = 0; i < 5; i++){
                // See if any threads are waiting for this message
                teSL_Status eStatus = eSL_MessageQueue(&sSerialLink, psMessageData->u16Type, psMessageData->u16Length, psMessageData->au8Message);
                if (eStatus == E_SL_OK) {
                    DBG_vPrintln(DBG_SERIAL_LINK_QUEUE, "Message Brocast Successful\n");
                    break;
                } else {
                    WAR_vPrintln(DBG_SERIAL_LINK_QUEUE, "No listeners for message type 0x%04X,:", psMessageData->u16Type);
                    if(psMessageData->u16Type == E_SL_MSG_STATUS){
                        tsSL_Msg_Status *sStatus;
                        sStatus = (tsSL_Msg_Status *)psMessageData->au8Message;
                        WAR_vPrintln(DBG_SERIAL_LINK_QUEUE, "0x%04x", sStatus->u16MessageType);
                    }
                    WAR_vPrintln(DBG_SERIAL_LINK_QUEUE,"\n");
                    usleep(20000);
                }
            }
            free(psMessageData);
        }
    }
    
    DBG_vPrintln(DBG_SERIAL_LINK, "pvMessageQueueHandlerThread Exit\n");
    
    /* Return from thread clearing resources */
    vThreadFinish(psThreadInfo);
    return NULL;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

