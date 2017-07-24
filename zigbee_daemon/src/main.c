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
 * MODIFICATION:       $Modification: 2017-06-25
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
#include "door_lock.h"
#include "zigbee_devices.h"
#include "zigbee_socket.h"
#include "zigbee_discovery.h"
//#include "zigbee_cloud.h"
#include "coordinator.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_MAIN (verbosity >= 1)
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const char *pVersion = "1.0";
volatile sig_atomic_t bRunning = 1;

int verbosity = 7;
int daemonize = 1;
uint32 u32BaudRate = 115200;
uint32 u32Channel = E_CHANNEL_DEFAULT;
char *pSerialDevice = "/dev/ttyUSB0";
char *pZigbeeSqlitePath = "./ZigbeeDaemon.DB";

tsDeviceIDMap asDeviceIDMap[] = 
{
    { E_ZBD_COORDINATOR,            eControlBridgeInitialize },
    { E_ZBD_ON_OFF_LIGHT,           eOnOffLightInitialize    },
    { E_ZBD_DIMMER_LIGHT,           eDimmerLightInitialize   },
    { E_ZBD_COLOUR_DIMMER_LIGHT,    eColourLightInitialize   },
    { E_ZBD_WINDOW_COVERING_DEVICE, eWindowCoveringInitialize},
    { E_ZBD_TEMPERATURE_SENSOR,     eTemperatureSensorInitialize},
    { E_ZBD_LIGHT_SENSOR,           eLightSensorInitialize },
    { E_ZBD_SIMPLE_SENSOR,          eSimpleSensorInitialize },
    { E_ZBD_SMART_PLUG,             eColourLightInitialize },
    { E_ZBD_DOOR_LOCK,              eDoorLockInitialize },
    { E_ZBD_DOOR_LOCK_CONTROLLER,   eDoorLockControllerInitialize },
    { E_ZBD_END_DEVICE_DEVICE,      eEndDeviceInitialize },
};

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/****************************************************************************/
/***        Locate   Functions                                            ***/
/****************************************************************************/
static void vPrintUsage(char *argv[])
{
    fprintf(stderr, "\t******************************************************\n");
    fprintf(stderr, "\t*         zigbee-daemon pVersion: %s              *\n", pVersion);
    fprintf(stderr, "\t******************************************************\n");
    fprintf(stderr, "\t************************Release***********************\n");

    fprintf(stderr, "Usage: %s\n", argv[0]);
    fprintf(stderr, "  Arguments:\n");
    fprintf(stderr, "    -s --serial        <serial device>     Serial device for 15.4 module, e.g. /dev/tts/1\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -h --help                              Print this help.\n");
    fprintf(stderr, "    -f --front                             Run in front.\n");
    fprintf(stderr, "    -v --DBG_MAIN     <DBG_MAIN>         Verbosity level. Increases amount of debug information. Default off.\n");
    fprintf(stderr, "    \t 01----main \n");
    fprintf(stderr, "    \t 03----discovery \n");
    fprintf(stderr, "    \t 04----sqlite \n");
    fprintf(stderr, "    \t 05----devices init & operator\n");
    fprintf(stderr, "    \t 06----host & node communication\n");
    fprintf(stderr, "    \t 07----socket\n");
    fprintf(stderr, "    \t 07----control bridge & node\n");
    fprintf(stderr, "    \t 08----print node\n");
    fprintf(stderr, "    \t 10----queue\n");
    fprintf(stderr, "    \t 11----thread & lock \n");
    fprintf(stderr, "    \t 12----serial communication & serial callback\n");
    fprintf(stderr, "    \t 13----serial\n");

    fprintf(stderr, "    -B --baud          <baud rate>         Baud rate to communicate with border router node at. Default 230400\n");
    fprintf(stderr, "    -D --datebase      <sqlite>            Datebase of sqlite3. Default None\n");

    fprintf(stderr, "  Zigbee Network options:\n");
    fprintf(stderr, "    -c --channel       <channel>           802.15.4 channel to run on. Default %d.\n", u32Channel);
    exit(0);
}

/**
 * Parse the user's parameters, and set the progress state
 * */
static void vGetOption(int argc, char *argv[])
{
    int opt = 0;
    int option_index = 0;
    static struct option long_options[] = {
        {"serial",                  required_argument,  NULL, 's'},
        {"help",                    no_argument,        NULL, 'h'},
        {"front",                   no_argument,        NULL, 'f'},
        {"verbosity",               required_argument,  NULL, 'v'},
        {"baud",                    required_argument,  NULL, 'B'},
        {"channel",                 required_argument,  NULL, 'c'},
        {"database",                required_argument,  NULL, 'D'},
        { NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "s:hfv:B:c:D:", long_options, &option_index)) != -1)
    {
        switch (opt) 
        {
            case 'h':
                vPrintUsage(argv);
            break;
            case 'f':
                daemonize = 0;
            break;
            case 'v':
                verbosity = atoi(optarg);
            break;
            case 'B':
            {
                char *pcEnd;
                errno = 0;
                u32BaudRate = (uint32)strtoul(optarg, &pcEnd, 0);
                if (errno){
                    printf("Baud rate '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                    vPrintUsage(argv);
                }
                if (*pcEnd != '\0'){
                    printf("Baud rate '%s' contains invalid characters\n", optarg);
                    vPrintUsage(argv);
                }
                break;
            }
            case 's':
                pSerialDevice = optarg;
            break;
            case 'D':
                pZigbeeSqlitePath = optarg;
            break;
            case 'c':
            {
                char *pcEnd;
                uint32 u32Channel;
                errno = 0;
                u32Channel = (uint32)strtoul(optarg, &pcEnd, 0);
                if (errno){
                    printf("Channel '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                    vPrintUsage(argv);
                }
                if (*pcEnd != '\0'){
                    printf("Channel '%s' contains invalid characters\n", optarg);
                    vPrintUsage(argv);
                }
                eChannel = (teChannel)u32Channel;
                break;
            }
            case 0:
            break;
            default: /* '?' */
            vPrintUsage(argv);
        }
    }
}
/**
 * Receive signal of ctrl + c, and exit the progress safely
 * */
static void vQuitSignalHandler (int sig)
{
    DBG_vPrintln(DBG_MAIN, "Got signal %d, exit program\n", sig);
    bRunning = 0;
    
    return;
}
/**
 * Fork a new progress and replaced current progress, then the progress turn to background
 * */
static void vDaemonizeInit(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    umask(0);   //Clear file creation mask.
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) { //Get maximum number of file descriptors
        ERR_vPrintln(T_TRUE,"%s: can't get file limit", cmd);
        exit(-1);
    }
    if ((pid = fork()) < 0) { //Become a session leader to lose controlling TTY
        ERR_vPrintln(T_TRUE,"%s: can't fork, exit(-1)\n", cmd);
        exit(-1);
    } else if (pid != 0) { /* parent */
        DBG_vPrintln(T_TRUE,"This is Parent Program, exit(0)\n");
        exit(0);
    }
    setsid();
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask); //Ensure future opens won't allocate controlling TTYs
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        ERR_vPrintln(T_TRUE,"%s: can't ignore SIGHUP", cmd);
        exit(-1);
    }
    if ((pid = fork()) < 0) {
        ERR_vPrintln(T_TRUE,"%s: can't fork, exit(-1)\n", cmd);
        exit(-1);
    } else if (pid != 0) { /* parent */
        DBG_vPrintln(T_TRUE,"This is Parent Program, exit(0)\n");
        exit(0);
    }
    /*
    * Change the current working directory to the root so
    * we won't prevent file systems from being unmounted.
    */
    if (chdir("/") < 0) {
        ERR_vPrintln(T_TRUE,"%s: can,t change directory to /", cmd);
        exit(-1);
    }
    if (rl.rlim_max == RLIM_INFINITY) { //Close all open file descriptors
        rl.rlim_max = 1024;
    }
    for (i = 0; i < rl.rlim_max; i++) {
        close(i);
    }
    fd0 = open("/dev/null", O_RDWR); //Attach file descriptors 0, 1, and 2 to /dev/null
    fd1 = dup(0);
    fd2 = dup(0);
    openlog(cmd, LOG_CONS, LOG_DAEMON); //Initialize the log file
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        ERR_vPrintln(T_TRUE, "unexpected file descriptors %d %d %d",fd0, fd1, fd2);
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    DBG_vPrintln(DBG_MAIN, "This is zigbee daemon program...\n");
    vGetOption(argc, argv);

    if ((!pSerialDevice) || (0 == u32BaudRate)){
        vPrintUsage(argv);
    }

    if (daemonize){
        WAR_vPrintln(T_TRUE, "Enter Daemon Mode...\n");
        vDaemonizeInit("ZigbeeDaemon");
    }

    signal(SIGINT,  vQuitSignalHandler);/* Install signal handlers */
    signal(SIGTERM, vQuitSignalHandler);
    mLogInitSetPid("[TB]");

    DBG_vPrintln(DBG_MAIN, "Init The Program with dev = %s, baud = %d\n", pSerialDevice, u32BaudRate);
    CHECK_RESULT(eZCB_Init(pSerialDevice, u32BaudRate), E_ZB_OK, -1);
    CHECK_RESULT(eZigbeeSqliteInit(pZigbeeSqlitePath), E_SQ_OK, -1);
    CHECK_RESULT(eSocketServer_Init(), E_SS_OK, -1);
    CHECK_RESULT(eZigbeeDiscoveryInit(), E_DISCOVERY_OK, -1);
    //CHECK_RESULT(eZigbeeCloudInit(), E_CLOUD_OK, -1);

    while(bRunning){
        DBG_vPrintln(DBG_MAIN, "Communication with Coordinator...\n");
        if(E_ZB_OK == eZCB_EstablishComm()){
            DBG_vPrintln(DBG_MAIN, "eZCB_EstablishComm Success\n");
            break;
        }
        sleep(1);
    }

    /** Initialize End Device */
    tsZigbeeBase psZigbeeNode, *psZigbeeItem = NULL;
    memset(&psZigbeeNode, 0, sizeof(psZigbeeNode));
    eZigbeeSqliteRetrieveDevicesList(&psZigbeeNode);
    dl_list_for_each(psZigbeeItem, &psZigbeeNode.list, tsZigbeeBase, list)
    {
        INF_vPrintln(DBG_MAIN, "[DEVICEID]:0X%04X, [NAME]:%s, [MAC]:0X%016llX, [ADDR]:0X%04X, [OnLine]:%d, [Type]:%d",
                     psZigbeeItem->u16DeviceID, psZigbeeItem->auDeviceName, psZigbeeItem->u64IEEEAddress,
                     psZigbeeItem->u16ShortAddress, psZigbeeItem->u8DeviceOnline, psZigbeeItem->u8MacCapability);
        /** use u8DeviceOnline to avoid inited repetitive */
        if((!(psZigbeeItem->u8MacCapability & E_ZB_MAC_CAPABILITY_FFD))&&(psZigbeeItem->u8DeviceOnline == 0)){
            tsZigbeeNodes *psZigbeeAdd = NULL;
            eZigbeeAddNode(psZigbeeItem->u16ShortAddress,
                           psZigbeeItem->u64IEEEAddress,
                           psZigbeeItem->u16DeviceID,
                           psZigbeeItem->u8MacCapability,
                           &psZigbeeAdd);
            eEndDeviceInitialize(psZigbeeAdd);
        }
    }
    eZigbeeSqliteRetrieveDevicesListFree(&psZigbeeNode);

    /** Check temporary password per 10 seconds */
    while(bRunning){
        sleep(10);

        uint32 u32TimeNow = (uint32)time((time_t*)NULL);
        tsTemporaryPassword sPasswordHeader, *psTemp;
        eZigbeeSqliteDoorLockRetrievePasswordList(&sPasswordHeader);
        dl_list_for_each(psTemp, &sPasswordHeader.list, tsTemporaryPassword, list){
            if(psTemp->u8Worked == 0){
                if(psTemp->u32TimeStart <= u32TimeNow && psTemp->u32TimeEnd >= u32TimeNow){
                    eZigbeeSqliteUpdateDoorLockPassword(psTemp->u8PasswordId, psTemp->u8AvailableNum, 1, psTemp->u8UseNum);
                    eZCB_SetDoorLockPassword(NULL,psTemp->u8PasswordId,T_TRUE,psTemp->u8PasswordLen, (const char*)psTemp->auPassword);
                }
            } else {
                if((psTemp->u8AvailableNum == 0) || (psTemp->u32TimeEnd < u32TimeNow)){
                    eZCB_SetDoorLockPassword(NULL, psTemp->u8PasswordId, T_FALSE, psTemp->u8PasswordLen, (const char*)psTemp->auPassword);
                }
            }
        }
        //eSocketDoorAlarmReport(0);
    }
    eZCB_Finish();
    eZigbeeSqliteFinished();
    eSocketServer_Destroy();
    eZigbeeDiscoveryFinished();
    //eZigbeeCloudFinished();

    DBG_vPrintln(DBG_MAIN, "Main thread will exiting\n");

    return 0;
}
