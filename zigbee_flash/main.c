/****************************************************************************
 *
 * MODULE:             main
 *
 * COMPONENT:          main function
 *
 * REVISION:           $Revision:  1.0 $
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
#include <getopt.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#include "utils.h"
#include "programmer.h"
#include "uart.h"
#include "JN51xx_BootLoader.h"
#include "ChipID.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_MAIN (verbosity >= 1)
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const char *Version = "1.0";
volatile sig_atomic_t bRunning = 1;

int verbosity = 0;
int daemonize = 0;

int iVerify = 0;
int iInitialSpeed = 0;
int iProgramSpeed = 0;
char *cpSerialDevice = "/dev/ttyUSB0";
char *pcFirmwareFile = NULL;
char *pcMAC_Address = NULL;
uint64 u64MAC_Address;
uint64 *pu64MAC_Address = NULL;
char * flashExtension = NULL;

/** Import binary data from FlashProgrammerExtension_JN5168.bin */
int _binary_FlashProgrammerExtension_JN5168_bin_start;
int _binary_FlashProgrammerExtension_JN5168_bin_size;
int _binary_FlashProgrammerExtension_JN5169_bin_start;
int _binary_FlashProgrammerExtension_JN5169_bin_size;
int _binary_FlashProgrammerExtension_JN5179_bin_start;
int _binary_FlashProgrammerExtension_JN5179_bin_size;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void print_usage_exit(char *argv[]);
static void vQuitSignalHandler (int sig);
static void vGetOption(int argc, char *argv[]);
teStatus cbProgress(void *pvUser, const char *pcTitle, const char *pcText, int iNumSteps, int iProgress);
static teStatus ePRG_ImportExtension(tsPRG_Context *psContext);

/****************************************************************************/
/***        Locate   Functions                                            ***/
/****************************************************************************/
int main(int argc, char *argv[])
{
    mLogInitSetPid("[TB]");
    DBG_vPrintf(DBG_MAIN, "This is zigbee flash program...\n");
    vGetOption(argc, argv);
    
    signal(SIGINT,  vQuitSignalHandler);/* Install signal handlers */
    signal(SIGTERM, vQuitSignalHandler);

    tsPRG_Context   sPRG_Context;
    if (eUART_Initialise((char *)cpSerialDevice, iInitialSpeed, &sPRG_Context.iUartFD, &sPRG_Context.sUartOptions) != E_PRG_OK)
    {
        ERR_vPrintf(T_TRUE, "Error opening serial port" );
        return -1;
    }
    DBG_vPrintf(DBG_MAIN, "Serial port opened at iInitialSpeed (= 38400)" );

    //if (iInitialSpeed != iProgramSpeed)
    {
        printf("Setting baudrate for port %d to %d\n", sPRG_Context.iUartFD, iInitialSpeed);

        /* Talking at initial speed - change bootloader to programming speed */
        int retry = 3;
        while ( retry-- > 0 && eBL_SetBaudrate(&sPRG_Context, iInitialSpeed) != E_PRG_OK) {
            ERR_vPrintf(T_TRUE, "Error setting (bootloader) baudrate to iProgramSpeed (%d) (%d)\n", iProgramSpeed, retry);
        }
        if ( retry <= 0 ) return -1;
        
        /* change local port to programming speed */
        //if (eUART_SetBaudRate(sPRG_Context.iUartFD, &sPRG_Context.sUartOptions, iProgramSpeed) != E_PRG_OK)
        {
        //    ERR_vPrintf(T_TRUE, "Error setting (local port) baudrate to iProgramSpeed (%d)\n", iProgramSpeed);
        //    return -1;
        }
    }
    /* Read module details at initial baud rate */
    if (ePRG_ChipGetDetails(&sPRG_Context) != E_PRG_OK)
    {
        ERR_vPrintf(T_TRUE, "Error reading module information - check cabling and power\n");
        return -1;
    }
    const char *pcPartName;
    
    switch (sPRG_Context.sChipDetails.u32ChipId)
    {
        case (CHIP_ID_JN5148_REV2A):    pcPartName = "JN5148";      break;
        case (CHIP_ID_JN5148_REV2B):    pcPartName = "JN5148";      break;
        case (CHIP_ID_JN5148_REV2C):    pcPartName = "JN5148";      break;
        case (CHIP_ID_JN5148_REV2D):    pcPartName = "JN5148J01";   break;
        case (CHIP_ID_JN5148_REV2E):    pcPartName = "JN5148Z01";   break;
    
        case (CHIP_ID_JN5142_REV1A):    pcPartName = "JN5142";      break;
        case (CHIP_ID_JN5142_REV1B):    pcPartName = "JN5142";      break;
        case (CHIP_ID_JN5142_REV1C):    pcPartName = "JN5142J01";   break;
    
        case (CHIP_ID_JN5168):          pcPartName = "JN5168";      break;
        case (CHIP_ID_JN5168_COG07A):   pcPartName = "JN5168";      break;
        case (CHIP_ID_JN5168_COG07B):   pcPartName = "JN5168";      break;
        
        case (CHIP_ID_JN5169):          pcPartName = "JN5169";      break;
        case (CHIP_ID_JN5169_DONGLE):   pcPartName = "JN5169";      break;
    
        case (CHIP_ID_JN5172):          pcPartName = "JN5172";      break;
    
        case (CHIP_ID_JN5179):          pcPartName = "JN5179";      break;
    
        default:                        pcPartName = "Unknown";     break;
    }
    
    DBG_vPrintf(DBG_MAIN,"Detected Chip: %s\n", pcPartName);
    
    DBG_vPrintf(DBG_MAIN,"MAC Address:   %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n", 
            sPRG_Context.sChipDetails.au8MacAddress[0] & 0xFF, 
            sPRG_Context.sChipDetails.au8MacAddress[1] & 0xFF, 
            sPRG_Context.sChipDetails.au8MacAddress[2] & 0xFF, 
            sPRG_Context.sChipDetails.au8MacAddress[3] & 0xFF, 
            sPRG_Context.sChipDetails.au8MacAddress[4] & 0xFF, 
            sPRG_Context.sChipDetails.au8MacAddress[5] & 0xFF, 
            sPRG_Context.sChipDetails.au8MacAddress[6] & 0xFF, 
            sPRG_Context.sChipDetails.au8MacAddress[7] & 0xFF);

    if ( pcFirmwareFile)
    {
        /* Have file to program */
    
        if (ePRG_FwOpen(&sPRG_Context, (char *)pcFirmwareFile)) {
            /* Error with file. FW module has displayed error so just exit. */
            ERR_vPrintf(T_TRUE,"Error with firmware file" );
            return -1;
        } else if ( ePRG_FlashProgram(&sPRG_Context, cbProgress, NULL, NULL) != E_PRG_OK ) {
            ERR_vPrintf(T_TRUE, "Error with flashing" );
            return -1;
        } else if ( iVerify && ePRG_FlashVerify(&sPRG_Context, cbProgress, NULL) != E_PRG_OK ) {
            ERR_vPrintf(T_TRUE, "Error in verification" );
            return -1;
        }
        
        
            if ( ePRG_ImportExtension(&sPRG_Context) != E_PRG_OK ) {
                ERR_vPrintf(T_TRUE, "Error importing extension" );
                return -1;
            } else if ( ePRG_EepromErase(&sPRG_Context, E_ERASE_EEPROM_PDM, cbProgress, NULL) != E_PRG_OK) {
                ERR_vPrintf(T_TRUE, "Error erasing EEPROM" );
                return -1;
            }
        
    }
    // Set BL and local port back to initial speed (in reverse order)
    eBL_SetBaudrate(&sPRG_Context, iInitialSpeed);
    eUART_SetBaudRate(sPRG_Context.iUartFD, &sPRG_Context.sUartOptions, iInitialSpeed);
	return 0;
}

teStatus cbProgress(void *pvUser, const char *pcTitle, const char *pcText, int iNumSteps, int iProgress)
{
    int progress;
    if (iNumSteps > 0)
    {
        progress = ((iProgress * 100) / iNumSteps);
    }
    else
    {
        // Begin marker
        progress = 0;
        printf( "\n" );
    }
    printf( "%c[A%s = %d%%\n", 0x1B, pcText, progress );
        
    return E_PRG_OK;
}
static int importExtension( char * file, int * start, int * size ) {
    struct stat sb;
    if (stat(file, &sb) == -1) {
        perror("stat");
        return 0;
    }
    
    printf("File size: %lld bytes\n", (long long) sb.st_size);
    size_t bytestoread = sb.st_size;
    
    if ( ( flashExtension = malloc( sb.st_size + 100 ) ) == NULL ) {
        perror("malloc");
        return 0;
    }
    
    int fp, bytesread;
    if ( ( fp = open(file,O_RDONLY) ) < 0 ) {
        perror("open");
        return 0;
    }
    
    char * pbuf = flashExtension;
    while ( bytestoread > 0 ) {
        if ( ( bytesread = read( fp, pbuf, bytestoread ) ) < 0 ) {
            break;
        }
        bytestoread -= bytesread;
        pbuf += bytesread;
        }
        
    if ( bytestoread == 0 ) {
        *start = (int *)flashExtension;
        *size  = sb.st_size;
        printf( "Loaded binary of %d bytes\n", *size );
        return 1;
    }
    
    return 0;
}

static teStatus ePRG_ImportExtension(tsPRG_Context *psContext)
{
    int ret = 0;
    switch (CHIP_ID_PART(psContext->sChipDetails.u32ChipId))
    {
        case (CHIP_ID_PART(CHIP_ID_JN5168)):
            ret = importExtension( "/usr/share/iot/FlashProgrammerExtension_JN5168.bin",
                &_binary_FlashProgrammerExtension_JN5168_bin_start,
                &_binary_FlashProgrammerExtension_JN5168_bin_size );
            psContext->pu8FlashProgrammerExtensionStart    = (uint8_t *)_binary_FlashProgrammerExtension_JN5168_bin_start;
            psContext->u32FlashProgrammerExtensionLength   = (uint32_t)_binary_FlashProgrammerExtension_JN5168_bin_size;
            break;
        case (CHIP_ID_PART(CHIP_ID_JN5169)):
            ret = importExtension( "/usr/share/iot/FlashProgrammerExtension_JN5169.bin",
                &_binary_FlashProgrammerExtension_JN5169_bin_start,
                &_binary_FlashProgrammerExtension_JN5169_bin_size );
            psContext->pu8FlashProgrammerExtensionStart    = (uint8_t *)_binary_FlashProgrammerExtension_JN5169_bin_start;
            psContext->u32FlashProgrammerExtensionLength   = (uint32_t)_binary_FlashProgrammerExtension_JN5169_bin_size;
            break;
        case (CHIP_ID_PART(CHIP_ID_JN5179)):
            ret = importExtension( "/usr/share/iot/FlashProgrammerExtension_JN5179.bin",
                &_binary_FlashProgrammerExtension_JN5179_bin_start,
                &_binary_FlashProgrammerExtension_JN5179_bin_size );
            psContext->pu8FlashProgrammerExtensionStart    = (uint8_t *)_binary_FlashProgrammerExtension_JN5179_bin_start;
            psContext->u32FlashProgrammerExtensionLength   = (uint32_t)_binary_FlashProgrammerExtension_JN5179_bin_size;
            break;
    }
    if ( ret ) {
        return E_PRG_OK;
    }
        
    return E_PRG_ERROR;
}

static void vGetOption(int argc, char *argv[])
{
    signed char opt = 0;
    int option_index = 0;
    static struct option long_options[] =
    {
        {"help",                    no_argument,        NULL,       'h'},
        {"verbosity",               required_argument,  NULL,       'V'},
        {"initialbaud",             required_argument,  NULL,       'I'},
        {"programbaud",             required_argument,  NULL,       'P'},
        {"serial",                  required_argument,  NULL,       's'},
        {"firmware",                required_argument,  NULL,       'f'},
        {"verify",                  no_argument,        NULL,       'v'},
        {"mac",                     required_argument,  NULL,       'm'},
        { NULL, 0, NULL, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "hs:V:f:vI:P:m:", long_options, &option_index)) != -1) 
    {
        switch (opt) 
        {
            case 0:
                
            case 'h':
                print_usage_exit(argv);
                break;
            case 'V':
                verbosity = atoi(optarg);
                break;
            case 'v':
                iVerify = 1;
                break;
            case 'I':
            {
                char *pcEnd;
                errno = 0;
                iInitialSpeed = strtoul(optarg, &pcEnd, 0);
                if (errno)
                {
                    printf("Initial baud rate '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                    print_usage_exit(argv);
                }
                if (*pcEnd != '\0')
                {
                    printf("Initial baud rate '%s' contains invalid characters\n", optarg);
                    print_usage_exit(argv);
                }
                break;
            }
            case 'P':
            {
                char *pcEnd;
                errno = 0;
                iProgramSpeed = strtoul(optarg, &pcEnd, 0);
                if (errno)
                {
                    printf("Program baud rate '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                    print_usage_exit(argv);
                }
                if (*pcEnd != '\0')
                {
                    printf("Program baud rate '%s' contains invalid characters\n", optarg);
                    print_usage_exit(argv);
                }
                break;
            }
            case 's':
                cpSerialDevice = optarg;
                break;
            case 'f':
                pcFirmwareFile = optarg;
                break;
            case 'm':
                pcMAC_Address = optarg;
                u64MAC_Address = strtoll(pcMAC_Address, (char **) NULL, 16);
                pu64MAC_Address = &u64MAC_Address;
                break;

            default: /* '?' */
                print_usage_exit(argv);
        }
    }
}

static void vQuitSignalHandler (int sig)
{
    DBG_vPrintf(DBG_MAIN, "Got signal %d\n", sig); 
    bRunning = 0;
    
    return;
}

static void print_usage_exit(char *argv[])
{
    fprintf(stderr, "Usage: %s\n", argv[0]);
    fprintf(stderr, "  Arguments:\n");
    fprintf(stderr, "    -s --serial        <serial device> Serial device for 15.4 module, e.g. /dev/tts/1\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -V --verbosity     <verbosity>     Verbosity level. Increses amount of debug information. Default 0.\n");
    fprintf(stderr, "    -I --initialbaud   <rate>          Set initial baud rate\n");
    fprintf(stderr, "    -P --programbaud   <rate>          Set programming baud rate\n");
    fprintf(stderr, "    -f --firmware      <firmware>      Load module flash with the given firmware file.\n");
    fprintf(stderr, "    -v --verify                        Verify image. If specified, verify the image programmedwas loaded correctly.\n");
    fprintf(stderr, "    -m --mac           <MAC Address>   Set MAC address of device. If this is not specified, the address is read from flash.\n");
    exit(EXIT_FAILURE);
}




