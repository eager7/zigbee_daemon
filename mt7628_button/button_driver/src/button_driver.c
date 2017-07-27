/*************************************************************************
	> File Name: button_driver.c
	> Author: PCT
	> Mail: 
	> Created Time: 2017年07月25日
 ************************************************************************/
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
//#include "../linux-3.10.14/drivers/char/ralink_gpio.h"
#include <asm/rt2880/rt_mmap.h>
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define GPIO_MAJOR 0	//major version
#define GPIO_MINOR 0

#define GPIO_NUM 1
#define GPIO_NAME "button_driver"

//////////////////////////REGISTER ADDRESS////////////////////////////
#define RALINK_SYSCTL_ADDR		RALINK_SYSCTL_BASE	// system control
#define RALINK_REG_GPIOMODE		(RALINK_SYSCTL_ADDR + 0x60)
#define RALINK_AGPIO_CFG		(RALINK_SYSCTL_ADDR + 0x3C)
#define RALINK_REG_GPIOMODE2	(RALINK_SYSCTL_ADDR + 0x64)
#define RALINK_IRQ_ADDR			RALINK_INTCL_BASE
#define RALINK_PRGIO_ADDR		RALINK_PIO_BASE // Programmable I/O
#define RALINK_REG_PIODIR		(RALINK_PRGIO_ADDR + 0x00)
#define RALINK_REG_PIOEDGE		(RALINK_PRGIO_ADDR + 0x04)
#define RALINK_REG_PIODATA		(RALINK_PRGIO_ADDR + 0x20)
#define RALINK_REG_PIODATA2		(RALINK_PRGIO_ADDR + 0x24)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static int gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos);
static int gpio_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos);
inline static unsigned gpio_poll(struct file *filp, poll_table *pwait);
static int gpio_open(struct inode *inode, struct file *filp);
long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int mem_release(struct inode *inode, struct file *filp);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static struct file_operations gpio_optns =
{
	.owner 			= THIS_MODULE,	//the module's owner
	.llseek			= NULL,			//set the pointer of file location
	.read  			= gpio_read,	//read data
	.aio_read		= NULL,			//asynchronous read
	.write 			= gpio_write,	//write data
	.aio_write		= NULL,			//asynchronous write
	.readdir		= NULL,			//read dir, only used for filesystem
	.poll  			= gpio_poll,	//poll to judge the device whether it can non blocking read & write
	//.ioctl 			= NULL,			//executive the cmd, int the later version of linux, used fun unlocked_ioctl replace this fun
	.unlocked_ioctl = gpio_ioctl,	//if system doens't use BLK filesystem ,the use this fun indeeded iotcl
	.compat_ioctl 	= NULL,			//the 32bit program will use this fun replace the ioctl in the 64bit platform
	.mmap			= NULL,			//memory mapping
	.open  			= gpio_open,	//open device
	.flush			= NULL,			//flush device
	.release		= mem_release,			//close the device
	//.synch			= NULL,			//refresh the data
	.aio_fsync		= NULL,			//asynchronouse .synch
	.fasync			= NULL,			//notifacation the device's Flag changed
};

static int gpio_major = GPIO_MAJOR;
static int gpio_minor = GPIO_MINOR;
struct cdev cdev;
/****************************************************************************/
/***        Local    Functions                                            ***/
/****************************************************************************/
static void led_init()
{
	//Set SD digital mode
	(*(volatile u32 *)RALINK_AGPIO_CFG) 	&= cpu_to_le32(~(0x0f<<17));
	(*(volatile u32 *)RALINK_AGPIO_CFG) 	|= cpu_to_le32(0x0f<<17);
	//GPIO#25, set IO mode and output
	(*(volatile u32 *)RALINK_REG_GPIOMODE) 	&= 	cpu_to_le32(~(0x03<<10));	//clear the 10&11 bit
	(*(volatile u32 *)RALINK_REG_GPIOMODE) 	|=  cpu_to_le32((0x01<<10));	//set the 10&11 bit,GPIO
	(*(volatile u32 *)RALINK_REG_PIODIR) 	&= 	cpu_to_le32(~(0x01<<25));
	(*(volatile u32 *)RALINK_REG_PIODIR) 	|=	cpu_to_le32((0x01<<25));
}

static void led_on()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) &= cpu_to_le32(~(0x01<<25));
}
static void led_off()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) |= cpu_to_le32((0x01<<25));
}

static void reset_on()
{
	(*(volatile u32 *)RALINK_REG_PIODATA2) |= cpu_to_le32(0x01<<4);
}
static void reset_off()
{
	(*(volatile u32 *)RALINK_REG_PIODATA2) &= cpu_to_le32(~(0x01<<4));
}

static void spimiso_on()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) |= cpu_to_le32(0x01<<6);
}
static void spimiso_off()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) &= cpu_to_le32(~(0x01<<6));
}

static int __init gpio_init(void)
{
	dev_t dev_no = MKDEV(gpio_major, gpio_minor);
	int result = 0;
    printk(KERN_DEBUG "Hello Kernel Init!\n");
	
	/*region device version*/
	if(gpio_major){
		printk(KERN_DEBUG "static region device\n");
		result = register_chrdev_region(dev_no, GPIO_NUM, GPIO_NAME);//static region
	}else{
		printk(KERN_DEBUG "alloc region device\n");
		result = alloc_chrdev_region(&dev_no, gpio_minor, GPIO_NUM, GPIO_NAME);//alloc region
		gpio_major = MAJOR(dev_no);//get major
	}
	if(result < 0){
		printk(KERN_ERR "Region Device Error, %d\n", result);
		return result;
	}
	
	cdev_init(&cdev, &gpio_optns);//init cdev
	cdev.owner = THIS_MODULE;
	cdev.ops = &gpio_optns;
	
	printk(KERN_DEBUG "cdev_add\n");
	cdev_add(&cdev, MKDEV(gpio_major, gpio_minor), GPIO_NUM);//regedit the device
	
	led_init();
	led_on();
	//reset_on();
	//spimiso_on();
    return 0;
}

static void gpio_exit(void)
{
    printk(KERN_ALERT "unregion the gpio device\n");
	cdev_del(&cdev);//delete the device
	unregister_chrdev_region(MKDEV(gpio_major, gpio_minor), GPIO_NUM);//unregion device
}

static int gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	printk(KERN_DEBUG "gpio_write\n");
	
	return 0;
}

static int gpio_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	printk(KERN_DEBUG "gpio_read\n");
	return 0;
}

static int gpio_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev = NULL;
	printk(KERN_DEBUG "gpio_open\n");
	/*
	dev = container_of(inode->i_cdev, struct scull_dev, dev);
	filp->private_data = dev;
	
	if(O_WRONLY == (filp->f_flags & O_ACCMODE)){//trim to 0 the length of device if open was write-only
		printk(KERN_ALERT "scull_trim 0\n");
		scull_trim(dev);//ignore error
	}*/
	return 0;
}

inline static unsigned gpio_poll(struct file *filp, poll_table *pwait)
{
	printk(KERN_DEBUG "gpio_poll\n");
	
	return 0;
}

long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk(KERN_DEBUG "gpio_ioctl\n");
	switch(cmd){
		case 1:{
			printk(KERN_DEBUG "gpio_ioctl 1, arg %ld\n", arg);
			led_on();
		}break;
		case 2:{
			printk(KERN_DEBUG "gpio_ioctl 2, arg %ld\n", arg);
			led_off();
		}break;
		case 3:{
			printk(KERN_DEBUG "gpio_ioctl 3, arg %ld\n", arg);
		}break;

	}
	return 0;
}

int mem_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "mem_release\n");
	return 0;
}

/****************************************************************************/
/***        Kernel    Module                                              ***/
/****************************************************************************/
module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Pan Chang Tao");
MODULE_DESCRIPTION("The IO Control Driver");
MODULE_ALIAS("A IO Module");
