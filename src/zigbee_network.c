/****************************************************************************
 *
 * MODULE:             Linux 6LoWPAN Routing daemon
 *
 * COMPONENT:          Interface to module
 *
 * REVISION:           $Revision: 37647 $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>

#include <syslog.h>

#include "zigbee_network.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZBNETWORK 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
tsZigbee_Network sZigbee_Network;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void DBG_PrintNode(tsZigbee_Node *psNode)
{
    int i, j, k;
    DBG_vPrintf(DBG_PCT, "Node Short Address: 0x%04X, IEEE Address: 0x%016llX MAC Capability 0x%02X Device ID 0x%04X\n", 
                psNode->u16ShortAddress, 
                (unsigned long long int)psNode->u64IEEEAddress,
                psNode->u8MacCapability,
                psNode->u16DeviceID
               );
    for (i = 0; i < psNode->u32NumEndpoints; i++)
    {
        const char *pcProfileName = NULL;
        tsNodeEndpoint  *psEndpoint = &psNode->pasEndpoints[i];

        switch (psEndpoint->u16ProfileID)
        {
            case (0x0104):  pcProfileName = "ZHA"; break;
            case (0xC05E):  pcProfileName = "ZLL"; break;
            default:        pcProfileName = "Unknown"; break;
        }
            
        DBG_vPrintf(DBG_PCT, "  Endpoint %d - Profile 0x%04X (%s)\n", 
                    psEndpoint->u8Endpoint, 
                    psEndpoint->u16ProfileID,
                    pcProfileName
                   );
        
        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            tsNodeCluster *psCluster = &psEndpoint->pasClusters[j];
            DBG_vPrintf(DBG_PCT, "    Cluster ID 0x%04X\n", psCluster->u16ClusterID);
            
            DBG_vPrintf(DBG_PCT, "      Attributes:\n");
            for (k = 0; k < psCluster->u32NumAttributes; k++)
            {
                DBG_vPrintf(DBG_PCT, "        Attribute ID 0x%04X\n", psCluster->pau16Attributes[k]);
            }
            
            DBG_vPrintf(DBG_PCT, "      Commands:\n");
            for (k = 0; k < psCluster->u32NumCommands; k++)
            {
                DBG_vPrintf(DBG_PCT, "        Command ID 0x%02X\n", psCluster->pau8Commands[k]);
            }
        }
    }
}


teZbStatus eZigbee_AddNode(uint16 u16ShortAddress, uint64 u64IEEEAddress, 
                                uint16 u16DeviceID, uint8 u8MacCapability, tsZigbee_Node **ppsZCBNode)
{
    teZbStatus eStatus = E_ZB_OK;
    tsZigbee_Node *psZigbeeNode = &sZigbee_Network.sNodes;
    
    mLockLock(&sZigbee_Network.mutex);
    
    while (psZigbeeNode->psNext)//if device into network again, then update it's info
    {
        if (u64IEEEAddress)
        {
            if (psZigbeeNode->psNext->u64IEEEAddress == u64IEEEAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "IEEE address already in network - update short address\n");
                mLockLock(&psZigbeeNode->psNext->mutex);
                psZigbeeNode->psNext->u16ShortAddress = u16ShortAddress;
                
                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZigbeeNode->psNext;
                }
                mLockUnlock(&psZigbeeNode->psNext->mutex);
                
                goto done;
            }
            if (psZigbeeNode->psNext->u16ShortAddress == u16ShortAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Short address already in network - update IEEE address\n");
                mLockLock(&psZigbeeNode->psNext->mutex);
                psZigbeeNode->psNext->u64IEEEAddress = u64IEEEAddress;

                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZigbeeNode->psNext;
                }
                mLockUnlock(&psZigbeeNode->psNext->mutex);
                
                goto done;
            }
        }
        else
        {
            if (psZigbeeNode->psNext->u16ShortAddress == u16ShortAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Short address already in network\n");
                mLockLock(&psZigbeeNode->psNext->mutex);

                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZigbeeNode->psNext;
                }
                mLockUnlock(&psZigbeeNode->psNext->mutex);
                
                goto done;
            }
            
        }
        psZigbeeNode = psZigbeeNode->psNext;//find a new space for new node
    }
    
    psZigbeeNode->psNext = malloc(sizeof(tsZigbee_Node));
    
    if (!psZigbeeNode->psNext)
    {
        ERR_vPrintf(T_TRUE, "Memory allocation failure allocating node\n");
        eStatus = E_ZB_ERROR_NO_MEM;
        goto done;
    }
    
    memset(psZigbeeNode->psNext, 0, sizeof(tsZigbee_Node));

    /* Got to end of list without finding existing node - add it at the end of the list */
    mLockCreate(&psZigbeeNode->psNext->mutex);
    psZigbeeNode->psNext->u16ShortAddress  = u16ShortAddress;
    psZigbeeNode->psNext->u64IEEEAddress   = u64IEEEAddress;
    psZigbeeNode->u8MacCapability          = u8MacCapability;
    psZigbeeNode->psNext->u16DeviceID      = u16DeviceID;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Created new Node & Add into DataBase\n");
#ifdef ZIGBEE_SQLITE
    if(eZigbeeSqliteAddNewDevice(u64IEEEAddress, u16ShortAddress, u16DeviceID, "NULL", "NULL"))
    {
        ERR_vPrintf(T_TRUE, "eZigbeeSqliteAddNewDevice:0x%04x Error\n", u16DeviceID);     
    }
#endif
    
    if (ppsZCBNode)
    {
        mLockLock(&psZigbeeNode->psNext->mutex);
        *ppsZCBNode = psZigbeeNode->psNext;
        mLockUnlock(&psZigbeeNode->psNext->mutex);
    }

done:
    //DBG_PrintNode(psZigbeeNode->psNext);
    mLockUnlock(&sZigbee_Network.mutex);
    return eStatus;
}


teZbStatus eZigbee_RemoveNode(tsZigbee_Node *psZigbeeNode)
{
    DBG_vPrintf(T_TRUE, "eZigbee_RemoveNode\n");
    teZbStatus eStatus = E_ZB_ERROR;
    tsZigbee_Node *psZCBCurrentNode = &sZigbee_Network.sNodes;
    int iNodeFreeable = 0;
    
    /* lock the list mutex and node mutex in the same order as everywhere else to avoid deadlock */
    //mLockUnlock(&psZigbeeNode->mutex);
    //mLockLock(&sZigbee_Network.mutex);
    //mLockLock(&psZigbeeNode->mutex);

    DBG_vPrintf(DBG_ZBNETWORK, "eZigbee_RemoveNode Get Mutex\n");
    if (psZigbeeNode == &sZigbee_Network.sNodes)//coordinator
    {
        eStatus = E_ZB_OK;
        iNodeFreeable = 0;
    }
    else
    {
        while (psZCBCurrentNode)
        {
            if (psZCBCurrentNode->psNext == psZigbeeNode)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Found node to remove\n");
                //DBG_PrintNode(psZigbeeNode);
                
                psZCBCurrentNode->psNext = psZCBCurrentNode->psNext->psNext;
                eStatus = E_ZB_OK;
                iNodeFreeable = 1;
                break;
            }
            psZCBCurrentNode = psZCBCurrentNode->psNext;
        }
    }
    
    if (eStatus == E_ZB_OK)
    {
        int i, j;
        for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Free endpoint %d\n", psZigbeeNode->pasEndpoints[i].u8Endpoint);
            if (psZigbeeNode->pasEndpoints[i].pasClusters)
            {
                for (j = 0; j < psZigbeeNode->pasEndpoints[i].u32NumClusters; j++)
                {
                    DBG_vPrintf(DBG_ZBNETWORK, "Free cluster 0x%04X\n", psZigbeeNode->pasEndpoints[i].pasClusters[j].u16ClusterID);
                    free(psZigbeeNode->pasEndpoints[i].pasClusters[j].pau16Attributes);
                    free(psZigbeeNode->pasEndpoints[i].pasClusters[j].pau8Commands);
                }
                free(psZigbeeNode->pasEndpoints[i].pasClusters);
            }
        }
        free(psZigbeeNode->pasEndpoints);
        free(psZigbeeNode->pau16Groups);
        
        /* Unlock the node first so that it may be free'd */
        mLockUnlock(&psZigbeeNode->mutex);
        mLockDestroy(&psZigbeeNode->mutex);
        if (iNodeFreeable)
        {
            free(psZigbeeNode);
        }
    }
    //else
    //{
    //    mLockUnlock(&psZigbeeNode->mutex);
    //}
    //mLockUnlock(&sZigbee_Network.mutex);
    return eStatus;
}


tsZigbee_Node *psZigbee_FindNodeByIEEEAddress(uint64 u64IEEEAddress)
{
    tsZigbee_Node *psZigbeeNode = &sZigbee_Network.sNodes;
    
    mLockLock(&sZigbee_Network.mutex);

    while (psZigbeeNode)
    {
        if (psZigbeeNode->u64IEEEAddress == u64IEEEAddress)
        {
            break;
        }
        psZigbeeNode = psZigbeeNode->psNext;
    }
    
    mLockUnlock(&sZigbee_Network.mutex);
    return psZigbeeNode;
}



tsZigbee_Node *psZigbee_FindNodeByShortAddress(uint16 u16ShortAddress)
{
    tsZigbee_Node *psZigbeeNode = &sZigbee_Network.sNodes;
    
    mLockLock(&sZigbee_Network.mutex);

    while (psZigbeeNode)
    {
        if (psZigbeeNode->u16ShortAddress == u16ShortAddress)
        {
            INF_vPrintf(DBG_ZBNETWORK, "psZigbee_FindNodeByShortAddress Success\n");
            break;
        }
        psZigbeeNode = psZigbeeNode->psNext;
    }
    
    mLockUnlock(&sZigbee_Network.mutex);
    return psZigbeeNode;
}


tsZigbee_Node *psZigbee_FindNodeControlBridge(void)
{
    tsZigbee_Node *psZigbeeNode = &sZigbee_Network.sNodes;
    //eUtils_LockLock(&psZigbeeNode->sLock);
    return psZigbeeNode;
}


teZbStatus eZigbee_NodeAddEndpoint(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ProfileID, tsNodeEndpoint **ppsEndpoint)
{
    tsNodeEndpoint *psNewEndpoint;
    int i;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Add Endpoint %d, profile 0x%04X to node 0x%04X\n", u8Endpoint, u16ProfileID, psZigbeeNode->u16ShortAddress);
    
    for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
    {
        if (psZigbeeNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Endpoint\n");
            if (u16ProfileID)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Set Endpoint %d profile to 0x%04X\n", u8Endpoint, u16ProfileID);
                psZigbeeNode->pasEndpoints[i].u16ProfileID = u16ProfileID;
            }
            return E_ZB_OK;
        }
    }
    
    DBG_vPrintf(DBG_ZBNETWORK, "Creating new endpoint %d\n", u8Endpoint);
    
    psNewEndpoint = realloc(psZigbeeNode->pasEndpoints, sizeof(tsNodeEndpoint) * (psZigbeeNode->u32NumEndpoints+1));
    
    if (!psNewEndpoint)
    {
        WAR_vPrintf(T_TRUE, "Memory allocation failure allocating endpoint\n");
        return E_ZB_ERROR_NO_MEM;
    }
    
    psZigbeeNode->pasEndpoints = psNewEndpoint;
    
    memset(&psZigbeeNode->pasEndpoints[psZigbeeNode->u32NumEndpoints], 0, sizeof(tsNodeEndpoint));
    psNewEndpoint = &psZigbeeNode->pasEndpoints[psZigbeeNode->u32NumEndpoints];
    psZigbeeNode->u32NumEndpoints++;
    
    psNewEndpoint->u8Endpoint = u8Endpoint;
    psNewEndpoint->u16ProfileID = u16ProfileID;

    if (ppsEndpoint)
    {
        *ppsEndpoint = psNewEndpoint;
    }
    
    return E_ZB_OK;
}


teZbStatus eZigbee_NodeAddCluster(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID)
{
    int i;
    tsNodeEndpoint *psEndpoint = NULL;
    tsNodeCluster  *psNewClusters;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add cluster 0x%04X to Endpoint %d\n", psZigbeeNode->u16ShortAddress, u16ClusterID, u8Endpoint);
    
    for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
    {
        if (psZigbeeNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZigbeeNode->pasEndpoints[i];
            break;
        }
    }
    if (!psEndpoint)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Endpoint not found\n");
        return E_ZB_UNKNOWN_ENDPOINT;
    }
    
    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Cluster ID\n");
            return E_ZB_OK;
        }
    }
    
    psNewClusters = realloc(psEndpoint->pasClusters, sizeof(tsNodeCluster) * (psEndpoint->u32NumClusters+1));
    if (!psNewClusters)
    {
        ERR_vPrintf(T_TRUE, "Memory allocation failure allocating clusters\n");
        return E_ZB_ERROR_NO_MEM;
    }
    psEndpoint->pasClusters = psNewClusters;
    
    memset(&psEndpoint->pasClusters[psEndpoint->u32NumClusters], 0, sizeof(tsNodeCluster));
    psEndpoint->pasClusters[psEndpoint->u32NumClusters].u16ClusterID = u16ClusterID;
    psEndpoint->u32NumClusters++;

    return E_ZB_OK;
}


teZbStatus eZigbee_NodeAddAttribute(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint16 u16AttributeID)
{
    int i;
    tsNodeEndpoint *psEndpoint = NULL;
    tsNodeCluster  *psCluster = NULL;
    uint16 *pu16NewAttributeList;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add Attribute 0x%04X to cluster 0x%04X on Endpoint %d\n",
                psZigbeeNode->u16ShortAddress, u16AttributeID, u16ClusterID, u8Endpoint);
    
    for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
    {
        if (psZigbeeNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZigbeeNode->pasEndpoints[i];
        }
    }
    if (!psEndpoint)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Endpoint not found\n");
        return E_ZB_UNKNOWN_ENDPOINT;
    }
    
    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            psCluster = &psEndpoint->pasClusters[i];
        }
    }
    if (!psCluster)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Cluster not found\n");
        return E_ZB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumAttributes; i++)
    {
        if (psCluster->pau16Attributes[i] == u16AttributeID)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Attribute ID\n");
            return E_ZB_ERROR;
        }
    }

    pu16NewAttributeList = realloc(psCluster->pau16Attributes, sizeof(uint16) * (psCluster->u32NumAttributes + 1));
    
    if (!pu16NewAttributeList)
    {
        ERR_vPrintf(T_TRUE, "Memory allocation failure allocating attributes\n");
        return E_ZB_ERROR_NO_MEM;
    }
    psCluster->pau16Attributes = pu16NewAttributeList;
    
    psCluster->pau16Attributes[psCluster->u32NumAttributes] = u16AttributeID;
    psCluster->u32NumAttributes++;
    
    return E_ZB_OK;
}


teZbStatus eZigbee_NodeAddCommand(tsZigbee_Node *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint8 u8CommandID)
{
    int i;
    tsNodeEndpoint *psEndpoint = NULL;
    tsNodeCluster  *psCluster = NULL;
    uint8 *pu8NewCommandList;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add Command 0x%02X to cluster 0x%04X on Endpoint %d\n",
                psZigbeeNode->u16ShortAddress, u8CommandID, u16ClusterID, u8Endpoint);
    
    for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
    {
        if (psZigbeeNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZigbeeNode->pasEndpoints[i];
        }
    }
    if (!psEndpoint)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Endpoint not found\n");
        return E_ZB_UNKNOWN_ENDPOINT;
    }
    
    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            psCluster = &psEndpoint->pasClusters[i];
        }
    }
    if (!psCluster)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Cluster not found\n");
        return E_ZB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumCommands; i++)
    {
        if (psCluster->pau8Commands[i] == u8CommandID)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Command ID\n");
            return E_ZB_ERROR;
        }
    }

    pu8NewCommandList = realloc(psCluster->pau8Commands, sizeof(uint8) * (psCluster->u32NumCommands + 1));
    
    if (!pu8NewCommandList)
    {
        ERR_vPrintf(T_TRUE, "Memory allocation failure allocating commands\n");
        return E_ZB_ERROR_NO_MEM;
    }
    psCluster->pau8Commands = pu8NewCommandList;
    
    psCluster->pau8Commands[psCluster->u32NumCommands] = u8CommandID;
    psCluster->u32NumCommands++;
    
    return E_ZB_OK;
}


tsNodeEndpoint *psZigbee_NodeFindEndpoint(tsZigbee_Node *psZigbeeNode, uint16 u16ClusterID)
{
    int i, j;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Find cluster 0x%04X\n", psZigbeeNode->u16ShortAddress, u16ClusterID);
    
    for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
    {
        tsNodeEndpoint *psEndpoint = NULL;
        psEndpoint = &psZigbeeNode->pasEndpoints[i];
        
        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            if (psEndpoint->pasClusters[j].u16ClusterID == u16ClusterID)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Found Cluster ID on Endpoint %d\n", psEndpoint->u8Endpoint);
                return psEndpoint;
            }
        }
    }
    DBG_vPrintf(DBG_ZBNETWORK, "Cluster 0x%04X not found on node 0x%04X\n", u16ClusterID, psZigbeeNode->u16ShortAddress);
    return NULL;
}

teZbStatus eZigbee_GetEndpoints(tsZigbee_Node *psZigbee_Node, eZigbee_ClusterID eClusterID, uint8 *pu8Src, uint8 *pu8Dst)
{
    tsZigbee_Node          *psControlBridge;
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    
    if (pu8Src)
    {
        psControlBridge = psZigbee_FindNodeControlBridge();
        if (!psControlBridge)
        {
            return E_ZB_ERROR;
        }
        mLockLock(&psControlBridge->mutex);
        psSourceEndpoint = psZigbee_NodeFindEndpoint(psControlBridge, eClusterID);
        mLockUnlock(&psControlBridge->mutex);
        if (!psSourceEndpoint)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Cluster ID 0x%04X not found on control bridge\n", eClusterID);
            return E_ZB_UNKNOWN_CLUSTER;
        }

        *pu8Src = psSourceEndpoint->u8Endpoint;
    }
    
    if (pu8Dst)
    {
        if (!psZigbee_Node)
        {
            return E_ZB_ERROR;
        }
        psDestinationEndpoint = psZigbee_NodeFindEndpoint(psZigbee_Node, eClusterID);

        if (!psDestinationEndpoint)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Cluster ID 0x%04X not found on node 0x%04X\n", eClusterID, psZigbee_Node->u16ShortAddress);
            return E_ZB_UNKNOWN_CLUSTER;
        }
        *pu8Dst = psDestinationEndpoint->u8Endpoint;
    }
    return E_ZB_OK;
}


teZbStatus eZigbee_NodeClearGroups(tsZigbee_Node *psZigbeeNode)
{
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Clear groups\n", psZigbeeNode->u16ShortAddress);
    
    if (psZigbeeNode->pau16Groups)
    {
        free(psZigbeeNode->pau16Groups);
        psZigbeeNode->pau16Groups = NULL;
    }
    
    psZigbeeNode->u32NumGroups = 0;
    return E_ZB_OK;
}


teZbStatus eZigbee_NodeAddGroup(tsZigbee_Node *psZigbeeNode, uint16 u16GroupAddress)
{
    uint16 *pu16NewGroups;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add group 0x%04X\n", psZigbeeNode->u16ShortAddress, u16GroupAddress);
    
    if (psZigbeeNode->pau16Groups)
    {
        int i;
        for (i = 0; i < psZigbeeNode->u32NumGroups; i++)
        {
            if (psZigbeeNode->pau16Groups[i] == u16GroupAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Node is already in group 0x%04X\n", u16GroupAddress);
                return E_ZB_OK;
            }
        }
    }

    pu16NewGroups = realloc(psZigbeeNode->pau16Groups, sizeof(uint16) * (psZigbeeNode->u32NumGroups + 1));
    
    if (!pu16NewGroups)
    {
        ERR_vPrintf(T_TRUE, "Memory allocation failure allocating groups\n");
        return E_ZB_ERROR_NO_MEM;
    }
    
    psZigbeeNode->pau16Groups = pu16NewGroups;
    psZigbeeNode->pau16Groups[psZigbeeNode->u32NumGroups] = u16GroupAddress;
    psZigbeeNode->u32NumGroups++;
    return E_ZB_OK;
}


void vZigbee_NodeUpdateComms(tsZigbee_Node *psZigbeeNode, teZbStatus eStatus)
{
    if (!psZigbeeNode)
    {
        return;
    }
    
    if (eStatus == E_ZB_OK)
    {
        gettimeofday(&psZigbeeNode->sComms.sLastSuccessful, NULL);
        psZigbeeNode->sComms.u16SequentialFailures = 0;
    }
    else if (eStatus == E_ZB_COMMS_FAILED)
    {
        psZigbeeNode->sComms.u16SequentialFailures++;
    }
#if DBG_ZBNETWORK
    {
        time_t nowtime;
        struct tm *nowtm;
        char tmbuf[64], buf[64];

        nowtime = psZigbeeNode->sComms.sLastSuccessful.tv_sec;
        nowtm = localtime(&nowtime);
        strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
        snprintf(buf, sizeof buf, "%s.%06d", tmbuf, (int)psZigbeeNode->sComms.sLastSuccessful.tv_usec);
        DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: %d sequential comms failures. Last successful comms at %s\n", 
                    psZigbeeNode->u16ShortAddress, psZigbeeNode->sComms.u16SequentialFailures, buf);
    }
#endif /* DBG_ZBNETWORK */
}

int iZigbee_DeviceTimedOut(tsZigbee_Node *psZigbeeNode)
{
    //DBG_vPrintf(1, "iZigbee_DeviceTimedOut\n"); 
    struct timeval sNow;
    gettimeofday(&sNow, NULL);  
    //Change the number of checking, so we can find node leave networ quickly.
    if (((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessful.tv_sec) > 30) && (psZigbeeNode->sComms.u16SequentialFailures > 5))
    {
        return 1;
    }
    return 0;
}

void vZigbee_NodeUpdateClusterComms(tsZigbee_Node *psZigbeeNode, eZigbee_ClusterID Zigbee_ClusterID)
{
    if (!psZigbeeNode)
    {
        return;
    }
    
    switch(Zigbee_ClusterID)
    {
        case E_ZB_CLUSTERID_ONOFF:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulOnOff, NULL);
        }
        break;
        case E_ZB_CLUSTERID_LEVEL_CONTROL:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulLevel, NULL);
        }
        break;
        case E_ZB_CLUSTERID_COLOR_CONTROL:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulRGB, NULL);
        }
        break;
        case E_ZB_CLUSTERID_TEMPERATURE:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulTemp, NULL);
        }
        break;
        case E_ZB_CLUSTERID_HUMIDITY:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulHumi, NULL);
        }
        break;
        case E_ZB_CLUSTERID_BINARY_INPUT_BASIC:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulSimple, NULL);
        }
        break;
        case E_ZB_CLUSTERID_ILLUMINANCE:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulIllu, NULL);
        }
        break;
        case E_ZB_CLUSTER_ID_POWER_CONFIGURATION:
        {
            gettimeofday(&psZigbeeNode->sComms.sLastSuccessfulPower, NULL);
        }
        break;
            
        default:
            break;
    }

}

int iZigbee_ClusterTimedOut(tsZigbee_Node *psZigbeeNode, eZigbee_ClusterID Zigbee_ClusterID)
{
    int iNumberSensor = 3;//Report Sensor can use bigger value
    struct timeval sNow;
    gettimeofday(&sNow, NULL);
    //srand((unsigned)time(NULL));
    int iTimes = rand()%10; //Scattered all requests by used rand

    switch(Zigbee_ClusterID)
    {
        case E_ZB_CLUSTERID_ONOFF:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulOnOff.tv_sec) > iTimes)
            {
                return 1;
            }
        }
        break;

        case E_ZB_CLUSTERID_LEVEL_CONTROL:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulLevel.tv_sec) > iTimes)
            {
                return 1;
            }
        }
        break;
            
        case E_ZB_CLUSTERID_COLOR_CONTROL:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulRGB.tv_sec) > iTimes)
            {
                return 1;
            }
        }
        break;
            
        case E_ZB_CLUSTERID_TEMPERATURE:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulTemp.tv_sec) > (iTimes + iNumberSensor))
            {
                return 1;
            }
        }
        break;
            
        case E_ZB_CLUSTERID_HUMIDITY:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulHumi.tv_sec) > (iTimes + iNumberSensor))
            {
                return 1;
            }
        }
        break;
            
        case E_ZB_CLUSTERID_BINARY_INPUT_BASIC:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulSimple.tv_sec) > (iTimes + iNumberSensor))
            {
                return 1;
            }
        }
        break;
            
        case E_ZB_CLUSTERID_ILLUMINANCE:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulIllu.tv_sec) > (iTimes + iNumberSensor))
            {
                return 1;
            }
        }
        break;
            
        case E_ZB_CLUSTER_ID_POWER_CONFIGURATION:
        {
            if ((sNow.tv_sec - psZigbeeNode->sComms.sLastSuccessfulPower.tv_sec) > (iTimes + iNumberSensor))
            {
                return 1;
            }
        }
        break;
            
        default:
            break;
    }

    return 0;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
