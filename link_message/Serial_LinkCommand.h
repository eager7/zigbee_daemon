#ifndef __H__SERIAL_LINKCOMMAND__
#define __H__SERIAL_LINKCOMMAND__

typedef unsigned int uint16_t ;
typedef unsigned char uint8_t ;

/** Serial link message types */
typedef enum
{
/* Common Commands */
    E_SL_MSG_STATUS                         =   0x8000,
    E_SL_MSG_LOG                            =   0x8001,
 
    E_SL_MSG_DATA_INDICATION                =   0x8002,
 
    E_SL_MSG_NODE_CLUSTER_LIST              =   0x8003,
    E_SL_MSG_NODE_ATTRIBUTE_LIST            =   0x8004,
    E_SL_MSG_NODE_COMMAND_ID_LIST           =   0x8005,
    E_SL_MSG_RESTART_PROVISIONED            =   0x8006,
    E_SL_MSG_RESTART_FACTORY_NEW            =   0x8007,
 
    E_SL_MSG_GET_VERSION                    =   0x0010,
    E_SL_MSG_VERSION_LIST                   =   0x8010,
 
    E_SL_MSG_SET_EXT_PANID                  =   0x0020,
    E_SL_MSG_SET_CHANNELMASK                =   0x0021,
    E_SL_MSG_SET_SECURITY                   =   0x0022,
    E_SL_MSG_SET_DEVICETYPE                 =   0x0023,
    E_SL_MSG_START_NETWORK                  =   0x0024,
    E_SL_MSG_NETWORK_JOINED_FORMED          =   0x8024,
 
    E_SL_MSG_RESET                          =   0x0011,
    E_SL_MSG_ERASE_PERSISTENT_DATA          =   0x0012,
    E_SL_MSG_GET_PERMIT_JOIN                =   0x0014,
    E_SL_MSG_GET_PERMIT_JOIN_RESPONSE       =   0x8014,
    E_SL_MSG_BIND                           =   0x0030,
    E_SL_MSG_UNBIND                         =   0x0031,
 
    E_SL_MSG_NETWORK_ADDRESS_REQUEST        =   0x0040,
    E_SL_MSG_IEEE_ADDRESS_REQUEST           =   0x0041,
    E_SL_MSG_IEEE_ADDRESS_RESPONSE          =   0x8041,
    E_SL_MSG_NODE_DESCRIPTOR_REQUEST        =   0x0042,
    E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST      =   0x0043,
    E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE     =   0x8043,
    E_SL_MSG_POWER_DESCRIPTOR_REQUEST       =   0x0044,
    E_SL_MSG_ACTIVE_ENDPOINT_REQUEST        =   0x0045,
    E_SL_MSG_MATCH_DESCRIPTOR_REQUEST       =   0x0046,
    E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE      =   0x8046,
    E_SL_MSG_MANAGEMENT_LEAVE_REQUEST       =   0x0047,
    E_SL_MSG_LEAVE_CONFIRMATION             =   0x8047,
    E_SL_MSG_LEAVE_INDICATION               =   0x8048,
    E_SL_MSG_PERMIT_JOINING_REQUEST         =   0x0049,
    E_SL_MSG_MANAGEMENT_NETWPRK_UPDATE_REQUEST =0x004A,
    E_SL_MSG_SYSTEM_SERVER_DISCOVERY        =   0x004B,
    E_SL_MSG_COMPLEX_DESCRIPTOR_REQUEST     =   0x004C,
    E_SL_MSG_DEVICE_ANNOUNCE                =   0x004D,
    
    E_SL_MSG_READ_ATTRIBUTE_REQUEST         =   0x0100,
    E_SL_MSG_READ_ATTRIBUTE_RESPONSE        =   0x8100,
    E_SL_MSG_DEFAULT_RESPONSE               =   0x8101,
 
    /* Group Cluster */
    E_SL_MSG_ADD_GROUP_REQUEST              =   0x0060,
    E_SL_MSG_ADD_GROUP_RESPONSE             =   0x8060,
    E_SL_MSG_VIEW_GROUP                     =   0x0061,
    E_SL_MSG_GET_GROUP_MEMBERSHIP_REQUEST   =   0x0062,
    E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE  =   0x8062,
    E_SL_MSG_REMOVE_GROUP_REQUEST           =   0x0063,
    E_SL_MSG_REMOVE_GROUP_RESPONSE          =   0x8063,
    E_SL_MSG_REMOVE_ALL_GROUPS              =   0x0064,
    E_SL_MSG_ADD_GROUP_IF_IDENTIFY          =   0x0065,
 
    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_SEND                  =   0x0070,
    E_SL_MSG_IDENTIFY_QUERY                 =   0x0071,
 
    /* Level Cluster */
    E_SL_MSG_MOVE_TO_LEVEL                  =   0x0080,
    E_SL_MSG_MOVE_TO_LEVEL_ONOFF            =   0x0081,
    E_SL_MSG_MOVE_STEP                      =   0x0082,
    E_SL_MSG_MOVE_STOP_MOVE                 =   0x0083,
    E_SL_MSG_MOVE_STOP_ONOFF                =   0x0084,
 
    /* On/Off Cluster */
    E_SL_MSG_ONOFF                          =   0x0092,
 
    /* Scenes Cluster */
    E_SL_MSG_VIEW_SCENE                     =   0x00A0,
    E_SL_MSG_ADD_SCENE                      =   0x00A1,
    E_SL_MSG_REMOVE_SCENE                   =   0x00A2,
    E_SL_MSG_REMOVE_SCENE_RESPONSE          =   0x80A2,
    E_SL_MSG_REMOVE_ALL_SCENES              =   0x00A3,
    E_SL_MSG_STORE_SCENE                    =   0x00A4,
    E_SL_MSG_STORE_SCENE_RESPONSE           =   0x80A4,
    E_SL_MSG_RECALL_SCENE                   =   0x00A5,
    E_SL_MSG_SCENE_MEMBERSHIP_REQUEST       =   0x00A6,
    E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE      =   0x80A6,
 
    /* Colour Cluster */
    E_SL_MSG_MOVE_TO_HUE                    =   0x00B0,
    E_SL_MSG_MOVE_HUE                       =   0x00B1,
    E_SL_MSG_STEP_HUE                       =   0x00B2,
    E_SL_MSG_MOVE_TO_SATURATION             =   0x00B3,
    E_SL_MSG_MOVE_SATURATION                =   0x00B4,
    E_SL_MSG_STEP_SATURATION                =   0x00B5,
    E_SL_MSG_MOVE_TO_HUE_SATURATION         =   0x00B6,
    E_SL_MSG_MOVE_TO_COLOUR                 =   0x00B7,
    E_SL_MSG_MOVE_COLOUR                    =   0x00B8,
    E_SL_MSG_STEP_COLOUR                    =   0x00B9,
 
/* ZLL Commands */
    /* Touchlink */
    E_SL_MSG_INITIATE_TOUCHLINK             =   0x00D0,
    E_SL_MSG_TOUCHLINK_STATUS               =   0x00D1,
 
    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_TRIGGER_EFFECT        =   0x00E0,
 
    /* On/Off Cluster */
    E_SL_MSG_ONOFF_TIMED                    =   0x0093,
    E_SL_MSG_ONOFF_EFFECTS                  =   0x0094,
 
    /* Scenes Cluster */
    E_SL_MSG_ADD_ENHANCED_SCENE             =   0x00A7,
    E_SL_MSG_VIEW_ENHANCED_SCENE            =   0x00A8,
    E_SL_MSG_COPY_SCENE                     =   0x00A9,
 
    /* Colour Cluster */
    E_SL_MSG_ENHANCED_MOVE_TO_HUE           =   0x00BA,
    E_SL_MSG_ENHANCED_MOVE_HUE              =   0x00BB,
    E_SL_MSG_ENHANCED_STEP_HUE              =   0x00BC,
    E_SL_MSG_ENHANCED_MOVE_TO_HUE_SATURATION =  0x00BD,
    E_SL_MSG_COLOUR_LOOP_SET                =   0x00BE,
    E_SL_MSG_STOP_MOVE_STEP                 =   0x00BF,
    E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE     =   0x00C0,
    E_SL_MSG_MOVE_COLOUR_TEMPERATURE        =   0x00C1,
    E_SL_MSG_STEP_COLOUR_TEMPERATURE        =   0x00C2,
 
/* ZHA Commands */
    /* Door Lock Cluster */
    E_SL_MSG_LOCK_UNLOCK_DOOR               =   0x00F0,
    
/* Persistant data manager messages */
    E_SL_MSG_PDM_AVAILABLE_REQUEST          =   0x0300,
    E_SL_MSG_PDM_AVAILABLE_RESPONSE         =   0x8300,
    E_SL_MSG_PDM_SAVE_RECORD_REQUEST        =   0x0200,
    E_SL_MSG_PDM_SAVE_RECORD_RESPONSE       =   0x8200,
    E_SL_MSG_PDM_LOAD_RECORD_REQUEST        =   0x0201,
    E_SL_MSG_PDM_LOAD_RECORD_RESPONSE       =   0x8201,
    E_SL_MSG_PDM_DELETE_ALL_RECORDS_REQUEST =   0x0202,
    E_SL_MSG_PDM_DELETE_ALL_RECORDS_RESPONSE=   0x8202,
	
	E_SL_MSG_WRITE_ATTRIBUTE_REQUEST             =      0x0400,
    E_SL_MSG_WRITE_ATTRIBUTE_RESPONSE            =      0x8400
}teSL_MsgType;


// ZCL command address modes (from ZCL spec)
typedef enum PACK
{
    E_ZCL_AM_BOUND,
    E_ZCL_AM_GROUP,
    E_ZCL_AM_SHORT,
    E_ZCL_AM_IEEE,
    E_ZCL_AM_BROADCAST,
    E_ZCL_AM_NO_TRANSMIT,
    E_ZCL_AM_BOUND_NO_ACK,
    E_ZCL_AM_SHORT_NO_ACK,
    E_ZCL_AM_IEEE_NO_ACK,
    E_ZCL_AM_ENUM_END,
} teZCL_AddressMode;

typedef struct {
uint16_t index;
char *name;
uint8_t *message;
uint8_t length;
}LinkMessage_t;

uint8_t PermitJoiningRequest[] = {0x00,0x49,0x00,0x04,0x00,0xFF,0xFC,0x64/*Permit time*/,0x00};
uint8_t ReadAttributeRequest[] = {0x01,0x00,/*Type*/0x00,14,/*Length*/
									0x00,E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/
									0x01,0x01,/*ED*/0x00,0x00/*Cluster*/,0x00/*Client*/,0x00,0x00,0x00/*No Manufacture*/,
									0x01,0x00,0x00/*Attribute List*/};
									
uint8_t WriteAttributeRequest[] = {0x04,0x00,/*Type*/0x00,18,/*Length*/
									0x00,E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/
									0x01,0x01,/*ED*/0x04,0x00/*Cluster*/,0x00/*Client*/,0x00,0x00,0x00/*No Manufacture*/,
									0x01,0x00,0x01/*Attribute List*/};
									
uint8_t OnOffLight[] = {0x00,0x92,0x00,0x06,0x00,E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/
					0x01,0x02,/*ED*/0x00/*OFF*/};
					
					
//Scene					
uint8_t ViewScene[] = {0x00,0xA0,0x00,0x09,0x00,E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/0x01,0x01,/*ED*/0x00,0x00/*Group ID,Scene ID*/};
uint8_t SotreScene[] = {0x00,0xA4,0x00,0x09,0x00,E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/0x01,0x01,/*ED*/0x00,0x00/*Group ID,Scene ID*/};
uint8_t RemoveScene[] = {0x00,0xA2,0x00,0x09,0x00,E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/0x01,0x01,/*ED*/0x00,0x00/*Group ID,Scene ID*/};
uint8_t RecallScene[] = {0x00,0xA5,0x00,0x09,0x00,E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/0x01,0x01,/*ED*/0x00,0x00/*Group ID,Scene ID*/};
uint8_t BindRequest[] = {0x00,0x30,0x00,21,0x00,
						 0x00,0x15,0x8d,0x00,0x00,0x35,0xf4,0x2f,/*Target Address*/
						 0x01,/*ED*/0x00,0x06/*OnOff Cluster*/,0x03,/*16-Mode*/
						 0x00,0x15,0x8d,0x00,0x00,0x53,0x66,0x4a,/*Destination Address*/0x01/*ED*/};			
//Color
uint8_t MoveHueToSaturation[] = {
								0x00,0xB3,0x00,0x08,0x00,/*Header*/
								E_ZCL_AM_BROADCAST,0xFF,0xFE,/*Dest*/
								0x01,0x01,/*ED*/};
//温度								
uint8_t MatchDescriptorRequest1[] = {0x00,0x46,0x00,0x08,0x00,/*Header*/
									/*0xA1,SN*/0xFF,0xFD,/*Address*/0x01,0x04,/*Profile*/0x01,0x04,0x02,0x00/*Cluster*/};
//ONOFF									
uint8_t MatchDescriptorRequest[] = {0x00,0x46,0x00,0x08,0x00,/*Header*/
									/*0xA1,SN*/0xFF,0xFD,/*Address*/0x01,0x04,/*Profile*/0x01,0x00,0x05,0x01/*Cluster*/};
//光感
uint8_t MatchDescriptorRequest3[] = {0x00,0x46,0x00,0x08,0x00,/*Header*/
									/*0xA1,SN*/0xFF,0xFD,/*Address*/0x01,0x04,/*Profile*/0x01,0x00,0x04,0x00/*Cluster*/};

//简单描述符请求
uint8_t SimpleDescriptorRequest[] = {0x00,0x43,0x00,0x03,0x00,
									0x00,0x00,0x01};
//彩灯
uint8_t MoveToHueAndStaturation[] = {0x00,0xb6,0x00,0x09,0x00,
									E_ZCL_AM_BROADCAST,0xFF,0xFD,0x01,0x01,/*AD&ED*/87,0xff,0x00,0x05};
uint8_t MoveToColor[] = {0x00,0xb7,0x00,11,0x00,
						E_ZCL_AM_BROADCAST,0xFF,0xFD,0x01,0x01,/*AD&ED*/0x00,0xFF,0x00,0xff,0x00,0x05};								
uint8_t MoveToHue[] = {0x00,0xb0,0x00,0x09,0x00,/*Header*/
						E_ZCL_AM_BROADCAST,0xFF,0xFD,0x01,0x01,/*AD&ED*/0xaa,/*Hue*/0x01,/*Direction*/0x00,0x05/*time*/};						
//启动设置
uint8_t ErasePersistentData[] = {0x00,0x12,0x00,0x00,0x00};
uint8_t Reset[] = {0x00,0x11,0x00,0x00,0x00};
uint8_t GetVersion[] = {0x00,0x10,0x00,0x00,0x00};
uint8_t SetExtendedPanID[] = {0x00,0x20,0x00,0x08,0x00,/*Header*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00/*PANID*/};
uint8_t SetChannelMask[] = {0x00,0x21,0x00,0x04,0x00,/*Header*/0x00,0x00,0x00,0x0f/*Channel*/};
uint8_t SetSecurityAndKey[] = {0x00,0x22,0x00,0x02,0x00,/*Header*/0x00,0x00};
uint8_t SetDeviceType[] = {0x00,0x23,0x00,0x01,0x00,/*Header*/0x00/*HA Coordinator*/};
uint8_t StartNetwork[] = {0x00,0x24,0x00,0x00,0x00};

uint8_t ManageLeaveDevice[] = {0x00,0x47,0x00,11,0x00,/*Header*/
								0x70,0x9f,/*Short Address*/
								0x00,0x15,0x8d,0x00,0x00,0x53,0x66,0x32/*Extended Address*/};
uint8_t Status[] = {0x80,0x00,0x00,0x00,0x00,/*Header*/};
uint8_t CurrentPosition[] = {0x01,0x01,0x00,0x01,0x00,/*Header*/
								0x64};
uint8_t SystemReset[] = {0x80,0x02,0x00,0x00,0x00,/*Header*/};
uint8_t NetworkReset[] = {0x80,0x03,0x00,0x00,0x00,/*Header*/};
								
LinkMessage_t LinkMessage[] = {
	{E_SL_MSG_PERMIT_JOINING_REQUEST,"Permit Joining Request",PermitJoiningRequest,sizeof(PermitJoiningRequest)},
	{E_SL_MSG_READ_ATTRIBUTE_REQUEST,"Read Attribute Request",ReadAttributeRequest,sizeof(ReadAttributeRequest)},
	{E_SL_MSG_ONOFF,"OnOff Light",OnOffLight,sizeof(OnOffLight)},
	{E_SL_MSG_STORE_SCENE,"Store Scene",SotreScene,sizeof(SotreScene)},
	{E_SL_MSG_VIEW_SCENE,"View Scene",ViewScene,sizeof(ViewScene)},
	{E_SL_MSG_REMOVE_SCENE,"Remove Scene",RemoveScene,sizeof(RemoveScene)},
	{E_SL_MSG_RECALL_SCENE,"Recall Scene",RecallScene,sizeof(RecallScene)},
	{E_SL_MSG_BIND,"Bind Request",BindRequest,sizeof(BindRequest)},
	{E_SL_MSG_WRITE_ATTRIBUTE_REQUEST,"WriteAttributeRequest",WriteAttributeRequest,sizeof(WriteAttributeRequest)},
	{E_SL_MSG_MOVE_TO_SATURATION,"Color Light",MoveHueToSaturation,sizeof(MoveHueToSaturation)},
	{E_SL_MSG_MATCH_DESCRIPTOR_REQUEST,"MatchDescriptorRequest",MatchDescriptorRequest,sizeof(MatchDescriptorRequest)},
	{E_SL_MSG_MOVE_TO_HUE_SATURATION,"MoveToHueAndStaturation",MoveToHueAndStaturation,sizeof(MoveToHueAndStaturation)},
	{E_SL_MSG_MOVE_TO_COLOUR,"MoveToColor",MoveToColor,sizeof(MoveToColor)},
	{E_SL_MSG_MOVE_TO_HUE,"MoveToHue",MoveToHue,sizeof(MoveToHue)},
	
	//启动命令组
	{E_SL_MSG_ERASE_PERSISTENT_DATA,"ErasePersistentData",ErasePersistentData,sizeof(ErasePersistentData)},
	{E_SL_MSG_RESET,"Reset",Reset,sizeof(Reset)},
	{E_SL_MSG_GET_VERSION,"GetVersion",GetVersion,sizeof(GetVersion)},
	{E_SL_MSG_SET_EXT_PANID,"SetExtendedPanID",SetExtendedPanID,sizeof(SetExtendedPanID)},
	{E_SL_MSG_SET_CHANNELMASK,"SetChannelMask",SetChannelMask,sizeof(SetChannelMask)},
	{E_SL_MSG_SET_SECURITY,"SetSecurityAndKey",SetSecurityAndKey,sizeof(SetSecurityAndKey)},
	{E_SL_MSG_SET_DEVICETYPE,"SetDeviceType",SetDeviceType,sizeof(SetDeviceType)},
	{E_SL_MSG_START_NETWORK,"StartNetwork",StartNetwork,sizeof(StartNetwork)},
	//设备管理
	{E_SL_MSG_MANAGEMENT_LEAVE_REQUEST,"ManageLeaveDevice",ManageLeaveDevice,sizeof(ManageLeaveDevice)},
	//Mico
	{E_SL_MSG_STATUS,"Status",Status,sizeof(Status)},
	{0x0101,"CurrentPosition",CurrentPosition,sizeof(CurrentPosition)},
	{0x8002,"SystemReset",SystemReset,sizeof(SystemReset)},
	{0x8003,"NetworkReset",NetworkReset,sizeof(NetworkReset)},
	
};

#endif
