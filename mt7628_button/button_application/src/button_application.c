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
int button_fd = 0;

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

void signal_handler(int signum)
{
    unsigned char btn[2] = {0};
    read(button_fd, &btn, sizeof(btn));

    if(signum == SIGUSR2){
        printf("key[%d][%d] hold\n", btn[0], btn[2]);
    }

}

int main()
{
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
	sleep(2);
	printf("-----------Flash LED2\n");
	val = 10;
	ioctl(button_fd, E_GPIO_DRIVER_LED2_FLSAH, &val);
    sleep(5);
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

#if 1
    signal(SIGUSR2,signal_handler);
    printf("enable interrupt\n");
    val = getpid();
    ioctl(button_fd, E_GPIO_DRIVER_ENABLE_KEY_INTERUPT, &val);
    printf("waiting....\n");
    pause();
    printf("disable interrupt\n");
    ioctl(button_fd, E_GPIO_DRIVER_DISABLE_KEY_INTERUPT, &val);
#endif


	close(button_fd);
	return 0;
}
