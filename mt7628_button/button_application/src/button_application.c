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

int main()
{
	const char *device_cmd = "mknod /dev/button c 250 0";
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
		printf("Open Led2\n");
		ioctl(button_fd,0x01,&val);
		sleep(1);
		printf("Close Led2\n");
		ioctl(button_fd,0x02,&val);
		sleep(1);
	}

	return 0;
}
