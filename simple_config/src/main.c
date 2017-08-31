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
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#include "utils.h"
#include "wifi_parse.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DBG_MAIN (verbosity >= 1)
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
volatile sig_atomic_t bRunning = 1;
int verbosity = 7;
int daemonize = 1;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/****************************************************************************/
/***        Locate   Functions                                            ***/
/****************************************************************************/
/**
 * Receive signal of ctrl + c, and exit the progress safely
 * */
static void vQuitSignalHandler (int sig)
{
    DBG_vPrintln(DBG_MAIN, "Got signal %d, exit program\n", sig);
    bRunning = 0;
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

    if (daemonize){
        WAR_vPrintln(T_TRUE, "Enter Daemon Mode...\n");
        vDaemonizeInit("ZigbeeDaemon");
    }

    signal(SIGINT,  vQuitSignalHandler);/* Install signal handlers */
    signal(SIGTERM, vQuitSignalHandler);

    while(bRunning){
        wifi_thread_init();
        wifi_receive_cmd();
        sleep(1);
    }

    DBG_vPrintln(DBG_MAIN, "Main thread will exiting\n");

    return 0;
}
