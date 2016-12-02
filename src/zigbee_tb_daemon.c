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
#include "serial.h"
#include "zigbee_control_bridge.h"
#include "serial_link.h"
#include "zigbee_network.h"
#include "zigbee_devices.h"
#include "zigbee_socket_server.h"
#include "zigbee_sqlite.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const char *Version = "1.0";
int verbosity = 0;

volatile sig_atomic_t bRunning = 1;

tsDeviceIDMap asDeviceIDMap[] = /** Map of supported Zigbee and JIP devices */ 
{
    { 0x0840, 0x08010010, eControlBridgeInitalise },
    { 0x0100, 0x08011175, eOnOffLightInitalise },       /* ZLL mono lamp / HA on/off lamp */
    { 0x0101, 0x08011175, eDimmerLightInitalise },      /* HA dimmable lamp */
    { 0x0102, 0x08011750, eColourLightInitalise },      /* HA dimmable colour lamp */
    { 0x0200, 0x08011750, eColourLightInitalise },      /* ZLL dimmable colour lamp */
    { 0x0210, 0x08011750, eColourLightInitalise },      /* ZLL extended colour lamp */
    { 0x0220, 0x08011750, eColourLightInitalise },      /* ZLL colour temperature lamp */
    { 0x0108, 0x08011755, eWarmColdLigthInitalise},     /* PCT Cold & Warm Light*/
    { 0x0302, 0x80011178, eTemperatureSensorInitalise },/* Temp Humidity sensor */
    { 0x0106, 0x80011179, eLightSensorInitalise },      /* Light sensor */
    { 0x000c, 0x80011180, eSimpleSensorInitalise },     /* Simple sensor */
    { 0x0051, 0x08011181, eColourLightInitalise },      /* Smart Plug */
#ifdef SWITCH
    { 0x0104, 0x08011194, eDimmerSwitchInitalise },     /* eDimmer Switch */
#endif        
    { 0x0000, 0x00000000},
};
static uint16 au16ProfileHA = E_ZB_PROFILEID_HA;
static uint16 au16Cluster[] = {
                            E_ZB_CLUSTERID_ONOFF,                   /*Light*/
                            E_ZB_CLUSTERID_BINARY_INPUT_BASIC,      /*binary sensor*/
                            E_ZB_CLUSTERID_TEMPERATURE,             /*tempertuare*/
                            E_ZB_CLUSTERID_ILLUMINANCE              /*light sensor*/
                            };


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void print_usage_exit(char *argv[]);
static void vQuitSignalHandler (int sig);
static void daemonize_init(const char *cmd);

/****************************************************************************/
/***        Locate   Functions                                            ***/
/****************************************************************************/
int main(int argc, char *argv[])
{
    signed char opt = 0;
    int option_index = 0;
    int daemonize = 1;
    uint32 u32BaudRate = 1000000;
    uint32 eChannel = CONFIG_DEFAULT_CHANNEL;
    char *cpSerialDevice = "/dev/ttyUSB0";
    char *pZigbeeSqlitePath = "/tmp/ZigbeeDaemon.DB";

    DBG_vPrintf(T_TRUE, "This is zigbee daemon program...\n");

    static struct option long_options[] = {
        {"serial",                  required_argument,  NULL, 's'},
        {"help",                    no_argument,        NULL, 'h'},
        {"front",                   no_argument,        NULL, 'f'},
        {"verbosity",               required_argument,  NULL, 'v'},
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
                print_usage_exit(argv);
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
                    print_usage_exit(argv);
                }
                if (*pcEnd != '\0'){
                    printf("Baud rate '%s' contains invalid characters\n", optarg);
                    print_usage_exit(argv);
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
                    print_usage_exit(argv);
                }
                if (*pcEnd != '\0'){
                    printf("Channel '%s' contains invalid characters\n", optarg);
                    print_usage_exit(argv);
                }
                eChannel = u32Channel;
                break;
            }
            case 0:
            break;
            default: /* '?' */
            print_usage_exit(argv);
        }
    }

    if ((!cpSerialDevice) || (0 == u32BaudRate)){
        print_usage_exit(argv);
    }
    
    if (daemonize){
        WAR_vPrintf(T_TRUE, "Enter Daemon Mode...\n");
        daemonize_init("ZigbeeDaemon");
    }

    signal(SIGTERM, vQuitSignalHandler);/* Install signal handlers */
    signal(SIGINT,  vQuitSignalHandler);
    
    DBG_vPrintf(verbosity, "Init The Program with dev = %s, braud = %d\n", cpSerialDevice, u32BaudRate);
    if ((eSL_Init(cpSerialDevice, u32BaudRate) != E_SL_OK) || 
        (eZCB_Init() != E_ZB_OK) || 
#ifdef ZIGBEE_SQLITE
        (eZigbeeSqliteInit(pZigbeeSqlitePath) != E_SQ_OK) ||
#endif
        (eSocketServer_Init() != E_SS_OK)){
        ERR_vPrintf(T_TRUE, "Init compents failed \n");
        goto finish;
    }

    while (bRunning)
    {
        /* Keep attempting to connect to the control bridge */
        if (eZCB_EstablishComms() == E_ZB_OK){
            sleep(2);
            break;
        } else {
            ERR_vPrintf(T_TRUE, "can't connect with coordinator \n");
        }
    }

    while (bRunning)
    {
        static uint8 time_search = 0;
        static uint8 time_num = 1;
        ++time_search;
        if(time_num*2 == time_search) {
            if(time_num < 5) {
                ++time_num;
            }
            mLockLock(&sZigbee_Network.mutex);
            tsZigbee_Node* tempNodes = &sZigbee_Network.sNodes;
            while(tempNodes != NULL)
            {
                if((strcmp(tempNodes->device_name, "")) && (tempNodes->u64IEEEAddress != 0))
                {
                    if(0 == iZigbee_DeviceTimedOut(tempNodes))
                    {
                        INF_vPrintf(verbosity, "Nodes name is %s, Id is 0x%04x, MAC is 0x%016llX\n",
                            tempNodes->device_name,tempNodes->u16DeviceID,tempNodes->u64IEEEAddress);
                    }
                    else
                    {
                        mLockLock(&tempNodes->mutex);
                        if (eZigbee_RemoveNode(tempNodes) != E_ZB_OK)
                        {
                            ERR_vPrintf(T_TRUE, "Error removing node from ZCB\n");
                            mLockUnlock(&tempNodes->mutex); 
                        }
                        break;//Devices remove success
                    }
                }
                tempNodes = tempNodes->psNext;
            }
            mLockUnlock(&sZigbee_Network.mutex);                   
            
            //retry search the device
            if(eZCB_MatchDescriptorRequest(E_ZB_BROADCAST_ADDRESS_RXONWHENIDLE, au16ProfileHA,
                                        sizeof(au16Cluster) / sizeof(uint16), au16Cluster, 
                                        0, NULL, NULL) != E_ZB_OK)
            {
                ERR_vPrintf(DBG_PCT, "Error sending match descriptor request\n");
            }
            time_search = 0;
        }
        sleep(3);
    }

    eZCB_Finish();
    eSL_Destroy();
    eSocketServer_Destroy();
#ifdef ZIGBEE_SQLITE
    eZigbeeSqliteFinished();
#endif
finish:
    if (daemonize)
        INF_vPrintf(verbosity, "Daemon process exiting\n");  
    else
        INF_vPrintf(verbosity, "Exiting\n");

	return 0;
}


static void print_usage_exit(char *argv[])
{
    fprintf(stderr, "\t******************************************************\n");
    fprintf(stderr, "\t*         zigbee-tb-daemon Version: %s            *\n", Version);
    fprintf(stderr, "\t******************************************************\n");
    fprintf(stderr, "\t************************Release***********************\n");
    
    fprintf(stderr, "Usage: %s\n", argv[0]);
    fprintf(stderr, "  Arguments:\n");
    fprintf(stderr, "    -s --serial        <serial device>     Serial device for 15.4 module, e.g. /dev/tts/1\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -h --help                              Print this help.\n");
    fprintf(stderr, "    -f --front                             Run in front.\n");
    fprintf(stderr, "    -v --verbosity     <verbosity>         Verbosity level. Increases amount of debug information. Default off.\n");
    fprintf(stderr, "    -B --baud          <baud rate>         Baud rate to communicate with border router node at. Default 230400\n");
    fprintf(stderr, "    -D --datebase      <sqlite>            Datebase of sqlite3. Default None\n");

    fprintf(stderr, "  Zigbee Network options:\n");
    fprintf(stderr, "    -c --channel       <channel>           802.15.4 channel to run on. Default %d.\n", CONFIG_DEFAULT_CHANNEL);
    exit(0);
}

static void vQuitSignalHandler (int sig)
{
    DBG_vPrintf(verbosity, "Got signal %d\n", sig); 
    bRunning = 0;
    
    return;
}
static void daemonize_init(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    /*
    * Clear file creation mask.
    */
    umask(0);
    /*
    * Get maximum number of file descriptors.
    */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        ERR_vPrintf(T_TRUE,"%s: can't get file limit", cmd);
        exit(-1);
    }
    /*
    * Become a session leader to lose controlling TTY.
    */
    if ((pid = fork()) < 0)
    {
        ERR_vPrintf(T_TRUE,"%s: can't fork, exit(-1)\n", cmd);
        exit(-1);
    }
    else if (pid != 0) /* parent */
    {
        DBG_vPrintf(T_TRUE,"This is Parent Program, exit(0)\n");
        exit(0);
    }

    setsid();
    /*
    * Ensure future opens won't allocate controlling TTYs.
    */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        ERR_vPrintf(T_TRUE,"%s: can't ignore SIGHUP", cmd);
        exit(-1);
    }
    if ((pid = fork()) < 0)
    {
        ERR_vPrintf(T_TRUE,"%s: can't fork, exit(-1)\n", cmd);
        exit(-1);
    }
    else if (pid != 0) /* parent */
    {
        DBG_vPrintf(T_TRUE,"This is Parent Program, exit(0)\n");
        exit(0);
    }
    /*
    * Change the current working directory to the root so
    * we won't prevent file systems from being unmounted.
    */
    if (chdir("/") < 0)
    {
        ERR_vPrintf(T_TRUE,"%s: can,t change directory to /", cmd);
        exit(-1);
    }
    /*
    * Close all open file descriptors.
    */
    if (rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }
    for (i = 0; i < rl.rlim_max; i++)
    {
        close(i);
    }
    /*
    * Attach file descriptors 0, 1, and 2 to /dev/null.
    */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    /*
    * Initialize the log file.
    */
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) 
    {
        ERR_vPrintf(T_TRUE, "unexpected file descriptors %d %d %d",fd0, fd1, fd2);
        exit(1);
    }
}


