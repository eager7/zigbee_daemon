/****************************************************************************
 *
 * MODULE:             Zigbee - JIP daemon
 *
 * COMPONENT:          Serial interface
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

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <termios.h>

#include "Serial.h"
#include "Utils.h"

#define DGB_SERIAL 0
//extern volatile sig_atomic_t bRunning;
static int serial_fd = 0;

static struct termios options;       //place for settings for serial port
char buf[255];                       //buffer for where data is put

teSerial_Status eSerial_Init(char *name, uint32 baud, int *piserial_fd)
{
    int fd;
    DBG_vPrintf(DGB_SERIAL, "Opening serial device '%s' at baud rate %ubps\n", name, baud);
    
    switch (baud)
    {
#ifdef B50
        case (50):          baud = B50;         break;
#endif /* B50 */
#ifdef B75
        case (75):          baud = B75;         break;
#endif /* B75 */
#ifdef B110
        case (110):         baud = B110;        break;
#endif /* B110 */
#ifdef B134
        case (134):         baud = B134;        break;
#endif /* B134 */
#ifdef B150
        case (150):         baud = B150;        break;
#endif /* B150 */
#ifdef B200
        case (200):         baud = B200;        break;
#endif /* B200 */
#ifdef B300
        case (300):         baud = B300;        break;
#endif /* B300 */
#ifdef B600
        case (600):         baud = B600;        break;
#endif /* B600 */
#ifdef B1200
        case (1200):        baud = B1200;       break;
#endif /* B1200 */
#ifdef B1800
        case (1800):        baud = B1800;       break;
#endif /* B1800 */
#ifdef B2400
        case (2400):        baud = B2400;       break;
#endif /* B2400 */
#ifdef B4800
        case (4800):        baud = B4800;       break;
#endif /* B4800 */
#ifdef B9600
        case (9600):        baud = B9600;       break;
#endif /* B9600 */
#ifdef B19200
        case (19200):       baud = B19200;      break;
#endif /* B19200 */
#ifdef B38400
        case (38400):       baud = B38400;      break;
#endif /* B38400 */
#ifdef B57600
        case (57600):       baud = B57600;      break;
#endif /* B57600 */
#ifdef B115200
        case (115200):      baud = B115200;     break;
#endif /* B115200 */
#ifdef B230400
        case (230400):      baud = B230400;     break;
#endif /* B230400 */
#ifdef B460800
        case (460800):      baud = B460800;     break;
#endif /* B460800 */
#ifdef B500000
        case (500000):      baud = B500000;     break;
#endif /* B500000 */
#ifdef B576000
        case (576000):      baud = B576000;     break;
#endif /* B576000 */
#ifdef B921600
        case (921600):      baud = B921600;     break;
#endif /* B921600 */
#ifdef B1000000
        case (1000000):     baud = B1000000;    break;
#endif /* B1000000 */
#ifdef B1152000
        case (1152000):     baud = B1152000;    break;
#endif /* B1152000 */
#ifdef B1500000
        case (1500000):     baud = B1500000;    break;
#endif /* B1500000 */
#ifdef B2000000
        case (2000000):     baud = B2000000;    break;
#endif /* B2000000 */
#ifdef B2500000
        case (2500000):     baud = B2500000;    break;
#endif /* B2500000 */
#ifdef B3000000
        case (3000000):     baud = B3000000;    break;
#endif /* B3000000 */
#ifdef B3500000
        case (3500000):     baud = B3500000;    break;
#endif /* B3500000 */
#ifdef B4000000
        case (4000000):     baud = B4000000;    break;
#endif /* B4000000 */
        default:
            ERR_vPrintf(T_TRUE, "Unsupported baud rate speccified (%d)\n", baud);
            return E_SERIAL_ERROR;
    }
    
    fd = open(name, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        ERR_vPrintf(T_TRUE, "Couldn't open serial device \"%s\"(%s)\n", name, strerror(errno));
        return E_SERIAL_ERROR;
    }
    DBG_vPrintf(DGB_SERIAL, "open serial device %s (%d) success\n", name, baud);

    if (tcgetattr(fd,&options) == -1)
    {
        ERR_vPrintf(T_TRUE, "Error getting port settings (%s)", strerror(errno));
        return E_SERIAL_ERROR;
    }

    options.c_iflag &= ~(INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IUCLC | IXON | IXANY | IXOFF);
    options.c_iflag = IGNBRK | IGNPAR;
    options.c_oflag &= ~(OPOST | OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
    options.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
    options.c_cflag |= CS8 | CREAD | HUPCL | CLOCAL;
    options.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN);

    /*Linux 库函数，设置串口信息*/
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    DBG_vPrintf(DGB_SERIAL, "set serial device option\n");
    if (tcsetattr(fd,TCSAFLUSH,&options) == -1)/*设置终端参数*/
    {
        ERR_vPrintf(T_TRUE, "Error setting port settings (%s)", strerror(errno));
        return E_SERIAL_ERROR;
    }
    
    *piserial_fd = serial_fd = fd;
    return E_SERIAL_OK;
}


teSerial_Status eSerial_Read(uint8 *data)
{
    signed char res;
    
    res = read(serial_fd,data,1);
    if (res > 0)
    {
        DBG_vPrintf(DGB_SERIAL, "RX 0x%02x\n", *data);
    }
    else
    {
        //printf("Serial read: %d\n", res);
        if (res == 0)
        {
            //daemon_log(LOG_ERR, "Serial connection to module interrupted");
            //bRunning = 0;
        }
        return E_SERIAL_NODATA;
    }
    return E_SERIAL_OK;
}

teSerial_Status eSerial_Write(const uint8 data)
{
    int err, attempts = 0;
    
    DBG_vPrintf(DGB_SERIAL, "TX 0x%02x\n", data);
    
    err = write(serial_fd,&data,1);
    if (err < 0)
    {
        if (errno == EAGAIN)/*try again*/
        {
            for (attempts = 0; attempts <= 5; attempts++)
            {
                usleep(1000);
                err = write(serial_fd,&data,1);
                if (err < 0) 
                {
                    if ((errno == EAGAIN) && (attempts == 5))
                    {
                        ERR_vPrintf(T_TRUE, "Error writing to module after %d attempts(%s)", attempts, strerror(errno));
                        exit(-1);
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            ERR_vPrintf(T_TRUE, "Error writing to module(%s)", strerror(errno));
            exit(-1);
        }
    }
    return E_SERIAL_OK;
}


teSerial_Status eSerial_ReadBuffer(uint8 *data, uint32 *count)
{
    int res;
    
    res = read(serial_fd, data, *count);
    if (res > 0)
    {
        *count = res;
        return E_SERIAL_OK;
    }
    else
    {
#if DEBUG
        if (DGB_SERIAL >= LOG_DEBUG) DBG_vPrintf(DGB_SERIAL, "Serial read: %d\n", res);
#endif /* DEBUG */
        if (res == 0)
        {
            ERR_vPrintf(T_TRUE, "Serial connection to module interrupted");
        }
        res = *count = 0;
        return E_SERIAL_NODATA;
    }
}


teSerial_Status eSerial_WriteBuffer(uint8 *data, uint32 count)
{
    int attempts = 0;
    int total_sent_bytes = 0, sent_bytes = 0;
    
    while (total_sent_bytes < count)
    {
        sent_bytes = write(serial_fd, &data[total_sent_bytes], count - total_sent_bytes);
        if (sent_bytes <= 0)
        {
            if (errno == EAGAIN)
            {
                if (++attempts >= 5)
                {
                    ERR_vPrintf(T_TRUE, "Error writing to module(%s)", strerror(errno));
                    return E_SERIAL_ERROR;
                }
                usleep(1000);
            }
            else
            {
                ERR_vPrintf(T_TRUE, "Error writing to module(%s)", strerror(errno));
                return -1;
            }
        }
        else
        {
            attempts = 0;
            total_sent_bytes += sent_bytes;
        }
    }
    return E_SERIAL_OK;
}


