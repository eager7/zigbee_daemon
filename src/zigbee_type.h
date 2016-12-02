/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP daemon
 *
 * COMPONENT:          typedef 
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

#ifndef __H_ZIGBEE_TYPE_H_
#define __H_ZIGBEE_TYPE_H_
#if defined __cplusplus
extern "C" { 
#endif

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;


typedef enum
{
    T_FALSE,
    T_TRUE,    
} bool_t;

#if defined __cplusplus
}
#endif

#endif/*__H_ZIGBEE_TYPE_H_*/
