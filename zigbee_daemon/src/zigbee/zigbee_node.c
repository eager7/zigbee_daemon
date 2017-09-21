/****************************************************************************
 *
 * MODULE:             Zigbee - daemon
 *
 * COMPONENT:          Zigbee Node interface
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
#include "zigbee_node.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_NODE (verbosity >= 7)
#define DBG_NODE_PRINT (verbosity >= 8)

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern int verbosity;
extern tsZigbeeNodes sControlBridge;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
teZbStatus eZigbeeAddNode(uint16 u16ShortAddress, uint64 u64IEEEAddress, uint16 u16DeviceID, uint8 u8MacCapability,
                          tsZigbeeNodes **ppsZigbeeNode)
{
    eLockLock(&sControlBridge.mutex);   /* Lock list of nodes */
    tsZigbeeNodes *psZigbeeTemp;
    dl_list_for_each(psZigbeeTemp, &sControlBridge.list, tsZigbeeNodes, list)  //Search List
    {
        if(u16ShortAddress == psZigbeeTemp->sNode.u16ShortAddress){ //Update Mac Address
            eLockLock(&psZigbeeTemp->mutex);
            psZigbeeTemp->sNode.u64IEEEAddress = u64IEEEAddress;
            if(ppsZigbeeNode) *ppsZigbeeNode = psZigbeeTemp; //return node
            eLockunLock(&psZigbeeTemp->mutex);
            goto done;
        }
        if(u64IEEEAddress == psZigbeeTemp->sNode.u64IEEEAddress){ //Update Short Address
            eLockLock(&psZigbeeTemp->mutex);
            psZigbeeTemp->sNode.u16ShortAddress = u16ShortAddress;
            if(ppsZigbeeNode) *ppsZigbeeNode = psZigbeeTemp;
            eLockunLock(&psZigbeeTemp->mutex); 
            goto done;
        }
    }
    //Add New Zigbee Node
    WAR_vPrintln(DBG_NODE, "Add New Node:0x%04X\n", u16ShortAddress);
    psZigbeeTemp = (tsZigbeeNodes*)malloc(sizeof(tsZigbeeNodes));
    memset(psZigbeeTemp, 0, sizeof(tsZigbeeNodes));
    CHECK_POINTER(psZigbeeTemp, E_ZB_ERROR_NO_MEM);
    eLockCreate(&psZigbeeTemp->mutex);
    psZigbeeTemp->sNode.u16ShortAddress = u16ShortAddress;
    psZigbeeTemp->sNode.u64IEEEAddress = u64IEEEAddress;
    psZigbeeTemp->sNode.u8MacCapability = u8MacCapability;
    psZigbeeTemp->sNode.u16DeviceID = u16DeviceID;
    dl_list_add_tail(&sControlBridge.list, &psZigbeeTemp->list);
    if(ppsZigbeeNode) *ppsZigbeeNode = psZigbeeTemp; //return node
done:    
    eLockunLock(&sControlBridge.mutex);
    
    return E_ZB_OK;
}

teZbStatus eZigbeeRemoveNode(tsZigbeeNodes *psZigbeeNode)
{
    CHECK_POINTER(psZigbeeNode, E_ZB_ERROR);
    eLockLock(&sControlBridge.mutex);   /* Lock list of nodes */
    tsZigbeeNodes *psZigbeeTemp1, *psZigbeeTemp2;
    dl_list_for_each_safe(psZigbeeTemp1, psZigbeeTemp2, &sControlBridge.list, tsZigbeeNodes, list){  //Search List
        if(psZigbeeNode == psZigbeeTemp1){
            WAR_vPrintln(DBG_NODE, "Remove Node:0x%04X\n", psZigbeeNode->sNode.u16ShortAddress);
            dl_list_del(&psZigbeeTemp1->list);
            int i, j;
            for(i = 0; i < psZigbeeTemp1->sNode.u32NumEndpoints; i++){
                if(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters){
                    for(j = 0; j < psZigbeeTemp1->sNode.pasEndpoints[i].u32NumClusters; j ++){
                        FREE(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters[j].pau16Attributes);
                        FREE(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters[j].pau8Commands);
                    }
                    FREE(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters); /* realloc memory is continuous */
                }
            }
            FREE(psZigbeeTemp1->sNode.pasEndpoints);
            FREE(psZigbeeTemp1->sNode.pau16Groups);
            eLockDestroy(&psZigbeeTemp1->mutex);
            FREE(psZigbeeTemp1);
        }
    }
    eLockunLock(&sControlBridge.mutex);
    
    return E_ZB_OK;
}

teZbStatus eZigbeeRemoveAllNodes(void)
{
    int i, j;
    eLockLock(&sControlBridge.mutex);   /* Lock list of nodes */
    tsZigbeeNodes *psZigbeeTemp1, *psZigbeeTemp2;
    dl_list_for_each_safe(psZigbeeTemp1, psZigbeeTemp2, &sControlBridge.list, tsZigbeeNodes, list){  //Search List
        dl_list_del(&psZigbeeTemp1->list);
        for(i = 0; i < psZigbeeTemp1->sNode.u32NumEndpoints; i++){
            if(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters){
                for(j = 0; j < psZigbeeTemp1->sNode.pasEndpoints[i].u32NumClusters; j ++){
                    FREE(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters[j].pau16Attributes);
                    FREE(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters[j].pau8Commands);
                }
                FREE(psZigbeeTemp1->sNode.pasEndpoints[i].pasClusters); /* realloc memory is continuous */
            }
        }
        FREE(psZigbeeTemp1->sNode.pasEndpoints);
        FREE(psZigbeeTemp1->sNode.pau16Groups);
        eLockDestroy(&psZigbeeTemp1->mutex);
        FREE(psZigbeeTemp1);
    }
    eLockunLock(&sControlBridge.mutex);
    /* remove control bridge */
    for(i = 0; i < sControlBridge.sNode.u32NumEndpoints; i++){
        if(sControlBridge.sNode.pasEndpoints[i].pasClusters){
            for(j = 0; j < sControlBridge.sNode.pasEndpoints[i].u32NumClusters; j ++){
                FREE(sControlBridge.sNode.pasEndpoints[i].pasClusters[j].pau16Attributes);
                FREE(sControlBridge.sNode.pasEndpoints[i].pasClusters[j].pau8Commands);
            }
            FREE(sControlBridge.sNode.pasEndpoints[i].pasClusters); /* realloc memory is continuous */
        }
    }
    FREE(sControlBridge.sNode.pasEndpoints);
    FREE(sControlBridge.sNode.pau16Groups);
    return E_ZB_OK;
}

teZbStatus eZigbeeNodeAddEndpoint(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ProfileID,
                                  tsNodeEndpoint **ppsEndpoint)
{
    tsNodeEndpoint *psNewEndpoint;
    int i;
    
    DBG_vPrintln(DBG_NODE, "Add Endpoint %d, profile 0x%04X to node 0x%04X\n", u8Endpoint, u16ProfileID, psZigbeeNode->u16ShortAddress);
    
    for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
    {
        if (psZigbeeNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            DBG_vPrintln(DBG_NODE, "Duplicate Endpoint\n");
            if (u16ProfileID)
            {
                DBG_vPrintln(DBG_NODE, "Set Endpoint %d profile to 0x%04X\n", u8Endpoint, u16ProfileID);
                psZigbeeNode->pasEndpoints[i].u16ProfileID = u16ProfileID;
            }
            return E_ZB_OK;
        }
    }
    
    DBG_vPrintln(DBG_NODE, "Creating new endpoint %d\n", u8Endpoint);
    
    psNewEndpoint = realloc(psZigbeeNode->pasEndpoints, sizeof(tsNodeEndpoint) * (psZigbeeNode->u32NumEndpoints+1));
    
    if (!psNewEndpoint)
    {
        WAR_vPrintln(T_TRUE, "Memory allocation failure allocating endpoint\n");
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
teZbStatus eZigbeeNodeAddCluster(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID)
{
    int i;
    tsNodeEndpoint *psEndpoint = NULL;
    tsNodeCluster  *psNewClusters;
    
    DBG_vPrintln(DBG_NODE, "Node 0x%04X: Add cluster 0x%04X to Endpoint %d\n", psZigbeeNode->u16ShortAddress, u16ClusterID, u8Endpoint);
    
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
        DBG_vPrintln(DBG_NODE, "Endpoint not found\n");
        return E_ZB_UNKNOWN_ENDPOINT;
    }
    
    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            DBG_vPrintln(DBG_NODE, "Duplicate Cluster ID\n");
            return E_ZB_OK;
        }
    }
    
    psNewClusters = realloc(psEndpoint->pasClusters, sizeof(tsNodeCluster) * (psEndpoint->u32NumClusters+1));
    if (!psNewClusters)
    {
        ERR_vPrintln(T_TRUE, "Memory allocation failure allocating clusters\n");
        return E_ZB_ERROR_NO_MEM;
    }
    psEndpoint->pasClusters = psNewClusters;
    
    memset(&psEndpoint->pasClusters[psEndpoint->u32NumClusters], 0, sizeof(tsNodeCluster));
    psEndpoint->pasClusters[psEndpoint->u32NumClusters].u16ClusterID = u16ClusterID;
    psEndpoint->u32NumClusters++;
    return E_ZB_OK;
}
teZbStatus eZigbeeNodeAddAttribute(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID,
                                   uint16 u16AttributeID)
{
    int i;
    tsNodeEndpoint *psEndpoint = NULL;
    tsNodeCluster  *psCluster = NULL;
    uint16 *pu16NewAttributeList;
    
    DBG_vPrintln(DBG_NODE, "Node 0x%04X: Add Attribute 0x%04X to cluster 0x%04X on Endpoint %d\n",
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
        DBG_vPrintln(DBG_NODE, "Endpoint not found\n");
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
        DBG_vPrintln(DBG_NODE, "Cluster not found\n");
        return E_ZB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumAttributes; i++)
    {
        if (psCluster->pau16Attributes[i] == u16AttributeID)
        {
            DBG_vPrintln(DBG_NODE, "Duplicate Attribute ID\n");
            return E_ZB_ERROR;
        }
    }

    pu16NewAttributeList = realloc(psCluster->pau16Attributes, sizeof(uint16) * (psCluster->u32NumAttributes + 1));
    
    if (!pu16NewAttributeList)
    {
        ERR_vPrintln(T_TRUE, "Memory allocation failure allocating attributes\n");
        return E_ZB_ERROR_NO_MEM;
    }
    psCluster->pau16Attributes = pu16NewAttributeList;
    
    psCluster->pau16Attributes[psCluster->u32NumAttributes] = u16AttributeID;
    psCluster->u32NumAttributes++;
    return E_ZB_OK;
}
teZbStatus eZigbeeNodeAddCommand(tsZigbeeBase *psZigbeeNode, uint8 u8Endpoint, uint16 u16ClusterID, uint8 u8CommandID)
{
    int i;
    tsNodeEndpoint *psEndpoint = NULL;
    tsNodeCluster  *psCluster = NULL;
    uint8 *pu8NewCommandList;
    
    DBG_vPrintln(DBG_NODE, "Node 0x%04X: Add Command 0x%02X to cluster 0x%04X on Endpoint %d\n",
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
        DBG_vPrintln(DBG_NODE, "Endpoint not found\n");
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
        DBG_vPrintln(DBG_NODE, "Cluster not found\n");
        return E_ZB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumCommands; i++)
    {
        if (psCluster->pau8Commands[i] == u8CommandID)
        {
            DBG_vPrintln(DBG_NODE, "Duplicate Command ID\n");
            return E_ZB_ERROR;
        }
    }

    pu8NewCommandList = realloc(psCluster->pau8Commands, sizeof(uint8) * (psCluster->u32NumCommands + 1));
    
    if (!pu8NewCommandList)
    {
        ERR_vPrintln(T_TRUE, "Memory allocation failure allocating commands\n");
        return E_ZB_ERROR_NO_MEM;
    }
    psCluster->pau8Commands = pu8NewCommandList;
    
    psCluster->pau8Commands[psCluster->u32NumCommands] = u8CommandID;
    psCluster->u32NumCommands++;
    return E_ZB_OK;
}

tsNodeEndpoint *psZigbeeNodeFindEndpoint(tsZigbeeBase *psZigbeeNode, uint16 u16ClusterID)
{
    int i, j;
    
    DBG_vPrintln(DBG_NODE, "Node 0x%04X: Find cluster 0x%04X\n", psZigbeeNode->u16ShortAddress, u16ClusterID);
    
    for (i = 0; i < psZigbeeNode->u32NumEndpoints; i++)
    {
        tsNodeEndpoint *psEndpoint = NULL;
        psEndpoint = &psZigbeeNode->pasEndpoints[i];
        
        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            if (psEndpoint->pasClusters[j].u16ClusterID == u16ClusterID)
            {
                DBG_vPrintln(DBG_NODE, "Found Cluster ID on Endpoint %d\n", psEndpoint->u8Endpoint);
                return psEndpoint;
            }
        }
    }
    DBG_vPrintln(DBG_NODE, "Cluster 0x%04X not found on node 0x%04X\n", u16ClusterID, psZigbeeNode->u16ShortAddress);
    return NULL;
}

teZbStatus eZigbeeGetEndpoints(tsZigbeeBase *psZigbee_Node, teZigbee_ClusterID eClusterID, uint8 *pu8Src, uint8 *pu8Dst)
{
    tsNodeEndpoint  *psSourceEndpoint;
    tsNodeEndpoint  *psDestinationEndpoint;
    
    if (pu8Src)
    {
        psSourceEndpoint = psZigbeeNodeFindEndpoint(&sControlBridge.sNode, eClusterID);
        if (!psSourceEndpoint)
        {
            DBG_vPrintln(DBG_NODE, "Cluster ID 0x%04X not found on control bridge\n", eClusterID);
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
        psDestinationEndpoint = psZigbeeNodeFindEndpoint(psZigbee_Node, eClusterID);

        if (!psDestinationEndpoint)
        {
            DBG_vPrintln(DBG_NODE, "Cluster ID 0x%04X not found on node 0x%04X\n", eClusterID, psZigbee_Node->u16ShortAddress);
            return E_ZB_UNKNOWN_CLUSTER;
        }
        *pu8Dst = psDestinationEndpoint->u8Endpoint;
    }
    return E_ZB_OK;
}

tsZigbeeNodes *psZigbeeFindNodeByShortAddress(uint16 u16ShortAddress)
{    
    eLockLock(&sControlBridge.mutex);
    tsZigbeeNodes *psZigbeeRet = NULL, *psZigbeeTemp = NULL;
    dl_list_for_each(psZigbeeTemp, &sControlBridge.list, tsZigbeeNodes, list){  //Search List
        if(u16ShortAddress == psZigbeeTemp->sNode.u16ShortAddress){
            psZigbeeRet = psZigbeeTemp;
        }
    }
    eLockunLock(&sControlBridge.mutex);
    return psZigbeeRet;
}

tsZigbeeNodes *psZigbeeFindNodeByIEEEAddress(uint64 u64IEEEAddress)
{
    if(0x0000 == u64IEEEAddress){
        return &sControlBridge;
    } else {
        eLockLock(&sControlBridge.mutex);
        tsZigbeeNodes *psZigbeeRet = NULL;
        tsZigbeeNodes *psZigbeeTemp;
        dl_list_for_each(psZigbeeTemp, &sControlBridge.list, tsZigbeeNodes, list){  //Search List
            if(u64IEEEAddress == psZigbeeTemp->sNode.u64IEEEAddress){
                psZigbeeRet = psZigbeeTemp;
            }
        }
        eLockunLock(&sControlBridge.mutex);
        return psZigbeeRet;
    }
}

void vZigbeePrintNode(tsZigbeeBase *psNode)
{
    int i, j, k;
    INF_vPrintln(DBG_NODE_PRINT, "Node Short Address: 0x%04X, IEEE Address: 0x%016llX MAC Capability 0x%02X Device ID 0x%04X\n",
                psNode->u16ShortAddress, (unsigned long long int)psNode->u64IEEEAddress,psNode->u8MacCapability,psNode->u16DeviceID);
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
            
        INF_vPrintln(DBG_NODE_PRINT, "  Endpoint %d - Profile 0x%04X (%s)\n",
                    psEndpoint->u8Endpoint, psEndpoint->u16ProfileID,pcProfileName);
        
        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            tsNodeCluster *psCluster = &psEndpoint->pasClusters[j];
            INF_vPrintln(DBG_NODE_PRINT, "    Cluster ID 0x%04X\n", psCluster->u16ClusterID);
            
            INF_vPrintln(DBG_NODE_PRINT, "      Attributes:\n");
            for (k = 0; k < psCluster->u32NumAttributes; k++)
            {
                DBG_vPrintln(DBG_NODE_PRINT, "        Attribute ID 0x%04X\n", psCluster->pau16Attributes[k]);
            }
            
            INF_vPrintln(DBG_NODE_PRINT, "      Commands:\n");
            for (k = 0; k < psCluster->u32NumCommands; k++)
            {
                INF_vPrintln(DBG_NODE_PRINT, "        Command ID 0x%02X\n", psCluster->pau8Commands[k]);
            }
        }
    }
}


