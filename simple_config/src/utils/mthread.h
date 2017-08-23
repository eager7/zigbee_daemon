/****************************************************************************
 *
 * MODULE:             thread lib interface
 *
 * COMPONENT:          mthreads.h
 *
 * REVISION:           $Revision: 1.0 $
 *
 * DATED:              $Date: 2016-12-02 15:13:17 +0100 (Fri, 12 Dec 2016 $
 *
 * MODIFICATION:       $Modification: 2017-06-25
 *
 * AUTHOR:             PCT
 *
 ****************************************************************************
 *
 * Copyright panchangtao@gmail.com 2016. All rights reserved
 *
 ***************************************************************************/


#ifndef __M_THREADS_H__
#define __M_THREADS_H__

#if defined __cplusplus   
extern "C" {
#endif
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <pthread.h>
#include "utils.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define THREAD_SIGNAL SIGUSR1
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** Enumerated type of thread status's */
typedef enum
{
    E_THREAD_OK,
    E_THREAD_ERROR_FAILED,
    E_THREAD_ERROR_TIMEOUT,
    E_THREAD_ERROR_NO_MEM,
} teThreadStatus;

/** Enumerated type for thread detached / joinable states */
typedef enum
{
    E_THREAD_JOINABLE,      /*< Thread is created so that it can be waited on and joined */
    E_THREAD_DETACHED,      /*< Thread is created detached so all resources are automatically free'd at exit. */
} teThreadDetachState;
/**< Enumerated type of thread states */
typedef volatile enum
{
    E_THREAD_STOPPED,   /*< Thread stopped */
    E_THREAD_RUNNING,   /*< Thread running */
    E_THREAD_STOPPING,  /*< Thread signaled to stop */
} teState;

/** Structure to represent a thread */
typedef struct
{
    volatile teState eState;                /*< State of the thread */
    teThreadDetachState eThreadDetachState; /*< Detach state of the thread */
    pthread_t pThread_Id;                   /*< Implementation specific private structure */
    void *pvThreadData;                     /*< Pointer to threads data parameter */
} tsThread;
/** the func of thread */
typedef void *(*tprThreadFunction)(void *psThreadInfoVoid);

typedef struct
{
    void **apvBuffer;                       /* the buffer of data store */
    uint32 u32Length;                       /* the length of queue */
    uint32 u32Front;                        /* the front pointer of queue */
    uint32 u32Rear;                         /* the last pointer of queue */

    pthread_mutex_t mutex;                  /* lock to operator queue */
    pthread_cond_t cond_space_available;    /* the indicate of queue space can be used */
    pthread_cond_t cond_data_available;     /* the indicate of queue data can be used */
} tsQueue;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/*****************************************************************************
** Prototype    : eThreadStart
** Description  : Start a thread object
** Input        : prThreadFunction, the function of thread
 *                psThreadInfo, the parameter of thread will receive
 *                eDetachState, the run state of thread
** Output       : none
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eThreadStart(tprThreadFunction prThreadFunction, tsThread *psThreadInfo, teThreadDetachState eDetachState);
/*****************************************************************************
** Prototype    : eThreadStop
** Description  : Stop a thread object
** Input        : psThreadInfo, the handler of the thread
** Output       : none
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eThreadStop(tsThread *psThreadInfo);
/*****************************************************************************
** Prototype    : vThreadFinish
** Description  : Function to be called within the thread when it is finished to clean up memory
** Input        : psThreadInfo, the handler of the thread
** Output       : none
** Return Value : None

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
void vThreadFinish(tsThread *psThreadInfo);
/*****************************************************************************
** Prototype    : eThreadYield
** Description  : Function to yield the CPU to another thread,
 * when a thread sleep, then call this func to make other thread running
** Input        : None
** Output       : None
** Return Value : Always return E_THREAD_OK

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eThreadYield(void);
/*****************************************************************************
** Prototype    : eLockCreate
** Description  : Create a mutex lock to synchronization the threads' data
** Input        : psLock, the pointer of mutex
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eLockCreate(pthread_mutex_t *psLock);
/*****************************************************************************
** Prototype    : eLockDestroy
** Description  : Destroy a mutex lock to free memory
** Input        : psLock, the pointer of mutex
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eLockDestroy(pthread_mutex_t *psLock);
/*****************************************************************************
** Prototype    : eLockLock
** Description  : Lock a mutex, if the mutex been locked, the thread will be hang up
** Input        : psLock, the pointer of mutex
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eLockLock(pthread_mutex_t *psLock);
/*****************************************************************************
** Prototype    : eLockLockTimed
** Description  : Try to lock a mutex for in due time, if the mutex been locked in due time,
 * the thread will be hang up, if timeout, return
** Input        : psLock, the pointer of mutex
 *                u32WaitTimeout, the time of try to lock mutex
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eLockLockTimed(pthread_mutex_t *psLock, uint32 u32WaitTimeout);
/*****************************************************************************
** Prototype    : eLockLockTimed
** Description  : Try to lock a mutex, if the mutex been locked, return
** Input        : psLock, the pointer of mutex
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eLockTryLock(pthread_mutex_t *psLock);
/*****************************************************************************
** Prototype    : eLockunLock
** Description  : Unlock the mutex, allow multiple unlock
** Input        : psLock, the pointer of mutex
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eLockunLock(pthread_mutex_t *psLock);
/*****************************************************************************
** Prototype    : eQueueCreate
** Description  : Create a queue, used to communication between threads,will be
 * allocate memory
** Input        : psQueue, the structure of queue
 *                u32Length, the length of queue
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eQueueCreate(tsQueue *psQueue, uint32 u32Length);
/*****************************************************************************
** Prototype    : eQueueDestroy
** Description  : Destroy a queue to free memory
** Input        : psQueue, the structure of queue
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eQueueDestroy(tsQueue *psQueue);
/*****************************************************************************
** Prototype    : eQueueEnqueue
** Description  : Push a data into queue
** Input        : psQueue, the structure of queue
 *                pvData, the pointer of data
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eQueueEnqueue(tsQueue *psQueue, void *pvData);
/*****************************************************************************
** Prototype    : eQueueDequeue
** Description  : Pull a data output queue
** Input        : psQueue, the structure of queue
 *                ppvData, the pointer of data
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eQueueDequeue(tsQueue *psQueue, void **ppvData);
/*****************************************************************************
** Prototype    : eQueueDequeueTimed
** Description  : Wait a data in due time
** Input        : psQueue, the structure of queue
 *                u32WaitTimeMil, the time will waiting
 *                ppvData, the pointer of data
** Output       : None
** Return Value : if success, return E_THREAD_OK, otherwise return E_THREAD_ERROR_FAILED

** History      :
** Date         : 2017/6/25
** Author       : PCT
*****************************************************************************/
teThreadStatus eQueueDequeueTimed(tsQueue *psQueue, uint32 u32WaitTimeMil, void **ppvData);

#if defined __cplusplus
}
#endif

#endif /* __THREADS_H__ */


