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
#include "serial_link.h"
#include "zigbee_devices.h"
#include "zigbee_socket.h"
#include "zigbee_discovery.h"
#include "zigbee_cloud.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_MAIN (verbosity >= 1)
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const char *Version = "1.0";
volatile sig_atomic_t bRunning = 1;

int verbosity = 7;
int daemonize = 0;
uint32 u32BaudRate = 1000000;
uint32 u32Channel = CONFIG_DEFAULT_CHANNEL;
char *cpSerialDevice = "/dev/ttyUSB0";
char *pZigbeeSqlitePath = "/tmp/ZigbeeDaemon.DB";

tsDeviceIDMap asDeviceIDMap[] = 
{
    { E_ZBD_COORDINATOR,            eControlBridgeInitalise },
    { E_ZBD_ON_OFF_LIGHT,           eOnOffLightInitalise    },
    { E_ZBD_DIMMER_LIGHT,           eDimmerLightInitalise   },
    { E_ZBD_COLOUR_DIMMER_LIGHT,    eColourLightInitalise   },
    { E_ZBD_WINDOW_COVERING_DEVICE, eWindowCoveringInitalise},
    { E_ZBD_TEMPERATURE_SENSOR,     eTemperatureSensorInitalise},
    { E_ZBD_LIGHT_SENSOR,           eLightSensorInitalise },
    { E_ZBD_SIMPLE_SENSOR,          eSimpleSensorInitalise },
    { E_ZBD_SMART_PLUG,             eColourLightInitalise },
    { E_ZBD_END_DEVICE_DEVICE,      eEndDeviceInitalise },
};

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void vPrintUsage(char *argv[]);
static void vQuitSignalHandler (int sig);
static void vDaemonizeInit(const char *cmd);
static void vGetOption(int argc, char *argv[]);

/****************************************************************************/
/***        Locate   Functions                                            ***/
/****************************************************************************/
int main(int argc, char *argv[])
{
    DBG_vPrintf(DBG_MAIN, "This is zigbee daemon program...\n");
    vGetOption(argc, argv);
    
    if ((!cpSerialDevice) || (0 == u32BaudRate)){
        vPrintUsage(argv);
    }
    
    if (daemonize){
        WAR_vPrintf(T_TRUE, "Enter Daemon Mode...\n");
        vDaemonizeInit("ZigbeeDaemon");
    }

    signal(SIGINT,  vQuitSignalHandler);/* Install signal handlers */
    signal(SIGTERM, vQuitSignalHandler);
    mLogInitSetPid("[TB]");
        
    DBG_vPrintf(DBG_MAIN, "Init The Program with dev = %s, braud = %d\n", cpSerialDevice, u32BaudRate);
    CHECK_RESULT(eZCB_Init(cpSerialDevice, u32BaudRate), E_ZB_OK, -1);
    CHECK_RESULT(eZigbeeSqliteInit(pZigbeeSqlitePath), E_SQ_OK, -1);
    CHECK_RESULT(eSocketServer_Init(), E_SS_OK, -1);
    CHECK_RESULT(eZigbeeDiscoveryInit(), E_DISCOVERY_OK, -1);
    CHECK_RESULT(eZigbeeCloudInit(), E_CLOUD_OK, -1);

    while(bRunning){
        DBG_vPrintf(DBG_MAIN, "Communication with Coordinator...\n");
        if(E_ZB_OK == eZCB_EstablishComms()){
            DBG_vPrintf(DBG_MAIN, "eZCB_EstablishComms Success\n");
            break;
        }
    }

    int iStart= 0;
    while(bRunning){
        sleep(10);
        tsZigbeeBase psZigbeeNode, *psZigbeeItem = NULL;
        memset(&psZigbeeNode, 0, sizeof(psZigbeeNode));
        eZigbeeSqliteRetrieveDevicesList(&psZigbeeNode);
        dl_list_for_each(psZigbeeItem, &psZigbeeNode.list, tsZigbeeBase, list)
        {
            INF_vPrintf(DBG_MAIN, "[DEVICEID]:0X%04X, [NAME]:%s, [MAC]:0X%016llX, [ADDR]:0X%04X, [OnLine]:%d, [Type]:%d\n", 
                psZigbeeItem->u16DeviceID, psZigbeeItem->auDeviceName, psZigbeeItem->u64IEEEAddress,
                psZigbeeItem->u16ShortAddress, psZigbeeItem->u8DeviceOnline, psZigbeeItem->u8MacCapability);
            //use u8DeviceOnline to avoid inited repetitily
            if((!(psZigbeeItem->u8MacCapability & E_ZB_MAC_CAPABILITY_FFD))&&(psZigbeeItem->u8DeviceOnline == 0)){
                tsZigbeeNodes *psZigbeeAdd = NULL;
                eZigbee_AddNode(psZigbeeItem->u16ShortAddress, psZigbeeItem->u64IEEEAddress, psZigbeeItem->u16DeviceID, psZigbeeItem->u8MacCapability, &psZigbeeAdd);
                eEndDeviceInitalise(psZigbeeAdd);
            }
        }
        eZigbeeSqliteRetrieveDevicesListFree(&psZigbeeNode);
        eZCB_NeighbourTableRequest(&iStart);
        //eCloudPushAllDevicesList();
        sleep(10);
    }
    eZCB_Finish();
    eZigbeeSqliteFinished();
    eSocketServer_Destroy();
    eZigbeeDiscoveryFinished();
    eZigbeeCloudFinished();

    sleep(1);
    DBG_vPrintf(DBG_MAIN, "Main thread will exiting\n");

	return 0;
}

static void vGetOption(int argc, char *argv[])
{
    signed char opt = 0;
    int option_index = 0;
    static struct option long_options[] = {
        {"serial",                  required_argument,  NULL, 's'},
        {"help",                    no_argument,        NULL, 'h'},
        {"front",                   no_argument,        NULL, 'f'},
        {"DBG_MAIN",                required_argument,  NULL, 'v'},
        {"baud",                    required_argument,  NULL, 'B'},
        {"channel",                 required_argument,  NULL, 'c'},
        {"datebase",                required_argument,  NULL, 'D'},
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
                u32BaudRate = strtoul(optarg, &pcEnd, 0);
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
                cpSerialDevice = optarg;
            break;
            case 'D':
                pZigbeeSqlitePath = optarg;
            break;
            case 'c':
            {
                char *pcEnd;
                uint32 u32Channel;
                errno = 0;
                u32Channel = strtoul(optarg, &pcEnd, 0);
                if (errno){
                    printf("Channel '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                    vPrintUsage(argv);
                }
                if (*pcEnd != '\0'){
                    printf("Channel '%s' contains invalid characters\n", optarg);
                    vPrintUsage(argv);
                }
                u32Channel = u32Channel;
                break;
            }
            case 0:
            break;
            default: /* '?' */
            vPrintUsage(argv);
        }
    }
}

static void vQuitSignalHandler (int sig)
{
    DBG_vPrintf(DBG_MAIN, "Got signal %d\n", sig); 
    bRunning = 0;
    
    return;
}
static void vDaemonizeInit(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    umask(0);   //Clear file creation mask.
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) { //Get maximum number of file descriptors
        ERR_vPrintf(T_TRUE,"%s: can't get file limit", cmd);
        exit(-1);
    }
    if ((pid = fork()) < 0) { //Become a session leader to lose controlling TTY
        ERR_vPrintf(T_TRUE,"%s: can't fork, exit(-1)\n", cmd);
        exit(-1);
    } else if (pid != 0) { /* parent */
        DBG_vPrintf(T_TRUE,"This is Parent Program, exit(0)\n");
        exit(0);
    }
    setsid();
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask); //Ensure future opens won't allocate controlling TTYs
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        ERR_vPrintf(T_TRUE,"%s: can't ignore SIGHUP", cmd);
        exit(-1);
    }
    if ((pid = fork()) < 0) {
        ERR_vPrintf(T_TRUE,"%s: can't fork, exit(-1)\n", cmd);
        exit(-1);
    } else if (pid != 0) { /* parent */
        DBG_vPrintf(T_TRUE,"This is Parent Program, exit(0)\n");
        exit(0);
    }
    /*
    * Change the current working directory to the root so
    * we won't prevent file systems from being unmounted.
    */
    if (chdir("/") < 0) {
        ERR_vPrintf(T_TRUE,"%s: can,t change directory to /", cmd);
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
        ERR_vPrintf(T_TRUE, "unexpected file descriptors %d %d %d",fd0, fd1, fd2);
        exit(1);
    }
}

static void vPrintUsage(char *argv[])
{
    fprintf(stderr, "\t******************************************************\n");
    fprintf(stderr, "\t*         zigbee-daemon Version: %s              *\n", Version);
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


