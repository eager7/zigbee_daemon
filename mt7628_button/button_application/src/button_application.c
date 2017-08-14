#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>

#define DBG_vPrintln(a,b,ARGS...)  \
    do {printf(("\e[34;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);} while(0)
#define INF_vPrintln(a,b,ARGS...)  \
    do {printf(("\e[33;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);} while(0)
#define NOT_vPrintln(a,b,ARGS...)  \
    do {printf(("\e[32;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);} while(0)
#define WAR_vPrintln(a,b,ARGS...)  \
    do {printf(("\e[35;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);} while(0)
#define ERR_vPrintln(a,b,ARGS...)  \
    do {printf(("\e[31;1m""[%d]" b "\n""\e[0m"), __LINE__, ## ARGS);} while(0)

#define DEV "/dev/gpio"

#define KEY2 28
#define KEY3 24
#define KEY4 22
#define LED_ZIGBEE 25

/*
 * ioctl commands
 */
#define	RALINK_GPIO_SET_DIR		0x01
#define RALINK_GPIO_SET_DIR_IN		0x11
#define RALINK_GPIO_SET_DIR_OUT		0x12
#define	RALINK_GPIO_READ		0x02
#define	RALINK_GPIO_WRITE		0x03
#define	RALINK_GPIO_SET			0x21
#define	RALINK_GPIO_CLEAR		0x31
#define	RALINK_GPIO_READ_INT		0x02 //same as read
#define	RALINK_GPIO_WRITE_INT		0x03 //same as write
#define	RALINK_GPIO_SET_INT		0x21 //same as set
#define	RALINK_GPIO_CLEAR_INT		0x31 //same as clear
#define RALINK_GPIO_ENABLE_INTP		0x08
#define RALINK_GPIO_DISABLE_INTP	0x09
#define RALINK_GPIO_REG_IRQ		0x0A
#define RALINK_GPIO_LED_SET		0x41
#define RALINK_GPIO_INIT		0x42 //PCT


int button_fd = 0;
typedef struct _btn_control{
    unsigned char state;//0x01,short press; 0x02,long press
    unsigned char value;
} btn_control_t;

typedef enum {
    E_GPIO_DRIVER_LED2_CONTROL = 0x01,
    E_GPIO_DRIVER_LED3_CONTROL,
    E_GPIO_DRIVER_LED2_FLSAH,
    E_GPIO_DRIVER_LED3_FLSAH,
    E_GPIO_DRIVER_STOP_FLSAH,
    E_GPIO_DRIVER_ENABLE_KEY_INTERUPT,
    E_GPIO_DRIVER_DISABLE_KEY_INTERUPT,

    E_GPIO_DRIVER_INIT,
}button_driver_e;

/*
 * structure used at regsitration
 */
typedef struct {
    unsigned int irq;		//request irq pin number
    pid_t pid;			//process id to notify
} ralink_gpio_reg_info;
#define RALINK_GPIO_LED_INFINITY	4000
typedef struct {
    int gpio;			//gpio number (0 ~ 23)
    unsigned int on;		//interval of led on
    unsigned int off;		//interval of led off
    unsigned int blinks;		//number of blinking cycles
    unsigned int rests;		//number of break cycles
    unsigned int times;		//blinking times
} ralink_gpio_led_info;

void signal_handler(int signum)
{
	printf("get signal [%d]\n", signum);
#if 1
    btn_control_t btn;
    read(button_fd, &btn, sizeof(btn));

    if(signum == SIGUSR2){
        printf("key[%d][%d] hold\n", btn.state, btn.value);
        ralink_gpio_led_info led_info;
        led_info.gpio = LED_ZIGBEE;
        led_info.blinks = 10;
        led_info.times = 10;
        ioctl(button_fd, RALINK_GPIO_LED_SET, &led_info);
    }
#endif
}

int main()
{
    int val = 0;
    const char *device_cmd = "/etc/init.d/button_mknod.sh";
    if(access(DEV,F_OK) < 0){
        system(device_cmd);
    }
    if(access(DEV,F_OK) < 0){
        WAR_vPrintln(1, "Can't mknod /dev/button");
        return -1;
    }
    button_fd = open(DEV,O_RDWR);
    if(button_fd < 0){
        ERR_vPrintln(1, "can't open /dev/gpio, return %d, err:%s\n", button_fd, strerror(errno));
        return -1;
    }
    ioctl(button_fd, RALINK_GPIO_INIT, &val);
    /** LED */
    val = 25;
    ioctl(button_fd, RALINK_GPIO_SET_DIR_OUT, &val);
    ralink_gpio_led_info led_info;
    led_info.gpio = LED_ZIGBEE;
    led_info.blinks = 10;
    led_info.times = 10;
    ioctl(button_fd, RALINK_GPIO_LED_SET, &led_info);

    /** GPIO */
    val = KEY2;
    ioctl(button_fd, RALINK_GPIO_SET_DIR_IN, &val);
    val = KEY3;
    ioctl(button_fd, RALINK_GPIO_SET_DIR_IN, &val);
    val = KEY4;
    ioctl(button_fd, RALINK_GPIO_SET_DIR_IN, &val);

    //enable gpio interrupt
    printf("enable the button interrupt\n");
    if (ioctl(button_fd, RALINK_GPIO_ENABLE_INTP) < 0) {
        perror("ioctl");
        close(button_fd);
        return -1;
    }

    //register my information
    ralink_gpio_reg_info info;
    info.pid = getpid();
    info.irq = KEY2;
    if (ioctl(button_fd, RALINK_GPIO_REG_IRQ, &info) < 0) {
        perror("ioctl");
        close(button_fd);
        return -1;
    }
    info.irq = KEY3;
    if (ioctl(button_fd, RALINK_GPIO_REG_IRQ, &info) < 0) {
        perror("ioctl");
        close(button_fd);
        return -1;
    }
    info.irq = KEY4;
    if (ioctl(button_fd, RALINK_GPIO_REG_IRQ, &info) < 0) {
        perror("ioctl");
        close(button_fd);
        return -1;
    }
    printf("install signal handler\n");
    signal(SIGUSR2, signal_handler);

    //wait for signal
    pause();

    //disable gpio interrupt
    if (ioctl(button_fd, RALINK_GPIO_DISABLE_INTP) < 0) {
        perror("ioctl");
        close(button_fd);
        return -1;
    }

	close(button_fd);
	return 0;
}
