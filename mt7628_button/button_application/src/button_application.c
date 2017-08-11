#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>


#define DEV "/dev/button"


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


void signal_handler(int signum)
{
	printf("get signal [%d]\n", signum);
#if 0
    btn_control_t btn;
    read(button_fd, &btn, sizeof(btn));

    if(signum == SIGUSR2){
        printf("key[%d][%d] hold\n", btn.state, btn.value);
    }
#endif
}

int main()
{
#if 0
	const char *device_cmd = "/etc/init.d/button_mknod.sh";
    if(access(DEV,F_OK) < 0)
        system(device_cmd);
    button_fd = open(DEV,O_RDWR);
    if(button_fd < 0){ 
        printf("can't open %s, return %d\n", DEV, button_fd);
        return -1;
    }
	int val = 0;
    ioctl(button_fd, E_GPIO_DRIVER_INIT, &val);
    int i;
#endif
#if 0
	for(i = 0; i < 5; i++){
		printf("-----------Open Led2\n");
		val = 1;
		ioctl(button_fd,E_GPIO_DRIVER_LED2_CONTROL,&val);
		sleep(1);
		printf("-----------Close Led2\n");
		val = 0;
		ioctl(button_fd,E_GPIO_DRIVER_LED2_CONTROL,&val);
		sleep(1);
	}
#endif
#if 0
	sleep(2);
	printf("-----------Flash LED2\n");
	val = 10;
	ioctl(button_fd, E_GPIO_DRIVER_LED2_FLSAH, &val);
    sleep(5);
#endif
#if 0
    printf("-----------Poll KEY\n");
    fd_set fdSelect, fdTemp;
    FD_ZERO(&fdSelect);//Init fd
    FD_SET(button_fd, &fdSelect);//Add socketserver fd into select fd
    int iListenFD = 0;
    if(button_fd > iListenFD) {
        iListenFD = button_fd;
    }
    fdTemp = fdSelect;  /* use temp value, because this value will be clear */
    int iResult = select(iListenFD + 1, &fdTemp, NULL, NULL, NULL);
    switch(iResult)
    {
        case 0:
            printf("receive message time out \n");
            break;

        case -1:
            printf("receive message error\n");
            break;

        default: {
            unsigned char value = 0;
            read(button_fd, &value, sizeof(value));
            printf("key value:%d\n", value);
        }
    }
#endif

#if 0
    signal(SIGUSR2,signal_handler);
    printf("enable interrupt\n");
    val = getpid();
    ioctl(button_fd, E_GPIO_DRIVER_ENABLE_KEY_INTERUPT, &val);
    printf("waiting....\n");
    pause();
    pause();
    pause();
    pause();
    pause();
    pause();
    printf("disable interrupt\n");
    ioctl(button_fd, E_GPIO_DRIVER_DISABLE_KEY_INTERUPT, &val);
#endif
#if 1
    button_fd = open("/dev/gpio",O_RDONLY);
    if(button_fd < 0){
        printf("can't open \"dev/gpio\", return %d\n", button_fd);
        return -1;
    }
    int val = 23;
    ioctl(button_fd, RALINK_GPIO_INIT, &val);
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
    info.irq = 23;
    if (ioctl(button_fd, RALINK_GPIO_REG_IRQ, &info) < 0) {
        perror("ioctl");
        close(button_fd);
        return -1;
    }
    printf("install signal handler\n");
    //issue a handler to handle SIGUSR1
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    //wait for signal
    pause();

    //disable gpio interrupt
    if (ioctl(button_fd, RALINK_GPIO_DISABLE_INTP) < 0) {
        perror("ioctl");
        close(button_fd);
        return -1;
    }
#endif

	close(button_fd);
	return 0;
}
