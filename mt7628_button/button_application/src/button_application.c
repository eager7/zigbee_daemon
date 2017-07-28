#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#define DEV "/dev/button"

typedef enum {
	E_GPIO_DRIVER_LED2_CONTROL = 0x01,
	E_GPIO_DRIVER_LED3_CONTROL,
	E_GPIO_DRIVER_LED2_FLSAH,
	E_GPIO_DRIVER_LED3_FLSAH,
	E_GPIO_DRIVER_STOP_FLSAH,

}button_driver_e;

int main()
{
	const char *device_cmd = "/etc/init.d/button_mknod.sh";
    if(access(DEV,F_OK) < 0)
        system(device_cmd);
    int button_fd = open(DEV,O_RDWR);
    if(button_fd < 0){ 
        printf("can't open %s, return %d\n", DEV, button_fd);
        return -1;
    }
	int val = 0;
	int i;
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
	sleep(2);
	printf("-----------Flash LED2\n");
	val = 10;
	ioctl(button_fd, E_GPIO_DRIVER_LED2_FLSAH, &val);

	close(button_fd);
	return 0;
}
