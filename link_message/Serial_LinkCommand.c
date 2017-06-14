#include <stdio.h>
#include <memory.h>

#include "Serial_LinkCommand.h"

static uint8_t eSL_WriteMessage(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data, uint8_t pu8Length);

int main( int argc, char *argv[] )
{
	while(1){
	
		uint8_t i = 0;
		uint16_t u16Index = 0;
		uint16_t u16Type = 0;
		uint16_t u16Length = 0;
		uint8_t buffer[100] = {0};
		
		printf("\n*****************Please Choose Message Type*******************\n");
		for( i = 0; i < sizeof(LinkMessage)/sizeof(LinkMessage_t); i++ )
		{
			printf("0x%x--%s\n", LinkMessage[i].index, LinkMessage[i].name);
		}
		scanf("%x",&u16Index);//获取用户输入命令号
		for( i = 0; i < sizeof(LinkMessage)/sizeof(LinkMessage_t); i++ )//查询命令
		{
			if(LinkMessage[i].index == u16Index)
			{
				if(E_SL_MSG_READ_ATTRIBUTE_REQUEST == u16Index)/*Cluster ID的获取*/
				{
					printf("\n----------Please Input Cluster ID-----------\n");
					scanf("%x",&u16Index);//获取用户输入Cluster ID
					LinkMessage[i].message[10] = u16Index>>8;
					LinkMessage[i].message[11] = u16Index & 0xff;
				}
				else if(0x0101 == u16Index)
				{
					printf("Please input Current Position:\n");
					int CurrentPosition = 0;
					scanf("%x", &CurrentPosition);
					LinkMessage[i].message[5] = (uint8_t)CurrentPosition;
				}
				u16Type = (LinkMessage[i].message[0]<<8)|LinkMessage[i].message[1];
				u16Length = (LinkMessage[i].message[2]<<8)|LinkMessage[i].message[3];
				break;
			}

		}
		memcpy(buffer,LinkMessage[i].message, LinkMessage[i].length);
		eSL_WriteMessage(u16Type, u16Length, buffer, LinkMessage[i].length);
	}	
	return 0;
}

static uint8_t u8SL_CalculateCRC(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data)
{
    int n;
    uint8_t u8CRC = 0;

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

static uint8_t eSL_WriteMessage(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data, uint8_t pu8Length)
{
	int n;
    uint8_t u8CRC;

    u8CRC = u8SL_CalculateCRC(u16Type, u16Length, &pu8Data[5]);
	pu8Data[4] = u8CRC;
	printf("\n------------The Link Message Is:------------\n");
	printf("01 ");
	printf("%02x ", u16Type >> 8);
	printf("%02x ", u16Type);
	printf("%02x ", u16Length >> 8);
	printf("%02x ", u16Length);
	printf("%02x ", u8CRC);
	
	for(n = 0; n < u16Length; n++)
	{
		printf("%02x ", pu8Data[n]);	
	}printf("03\n");


	printf("01 ");
	for ( n = 0; n < pu8Length; n++ )
	{
		if(pu8Data[n] < 0x10)
		{
			pu8Data[n] ^= 0x10;
			printf("02 ");printf("%02x ",pu8Data[n]);
		}
		else
		{
			printf("%02x ",pu8Data[n]);
		}
	}
	printf("03\n");
	
	return 0;
}








