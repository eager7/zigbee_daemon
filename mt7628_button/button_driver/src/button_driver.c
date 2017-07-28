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

#include "button_driver.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

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
static button_driver_t button_driver;
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


/****************************************************************************/
/***        Local    Functions                                            ***/
/****************************************************************************/
static void gpio_init()
{
	//Set SD digital mode
	(*(volatile u32 *)RALINK_AGPIO_CFG) 	&= cpu_to_le32(~(0x0f<<17));
	(*(volatile u32 *)RALINK_AGPIO_CFG) 	|= cpu_to_le32(0x0f<<17);
	//Set SD_Mode as GPIO
	(*(volatile u32 *)RALINK_REG_GPIOMODE) 	&= 	cpu_to_le32(~(0x03<<10));	//clear the 10&11 bit
	(*(volatile u32 *)RALINK_REG_GPIOMODE) 	|=  cpu_to_le32((0x01<<10));	//set the 10&11 bit,GPIO
	//Set LED I/O
	(*(volatile u32 *)RALINK_REG_PIODIR) 	&= 	cpu_to_le32(~(0x01<<25));//LED2
	(*(volatile u32 *)RALINK_REG_PIODIR) 	|=	cpu_to_le32((0x01<<25));	
	(*(volatile u32 *)RALINK_REG_PIODIR) 	&= 	cpu_to_le32(~(0x01<<27));//LED3
	(*(volatile u32 *)RALINK_REG_PIODIR) 	|=	cpu_to_le32((0x01<<27));
	
	//Set Key I/O
	(*(volatile u32 *)RALINK_REG_PIODIR) 	&= 	cpu_to_le32(~(0x01<<28));//KEY2
	(*(volatile u32 *)RALINK_REG_PIODIR) 	&= 	cpu_to_le32(~(0x01<<24));//KEY3
	(*(volatile u32 *)RALINK_REG_PIODIR) 	&= 	cpu_to_le32(~(0x01<<22));//KEY4
}

static void led2_on()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) &= cpu_to_le32(~(0x01<<25));
}
static void led2_off()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) |= cpu_to_le32((0x01<<25));
}

static void led3_on()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) &= cpu_to_le32(~(0x01<<27));
}
static void led3_off()
{
	(*(volatile u32 *)RALINK_REG_PIODATA) |= cpu_to_le32((0x01<<27));
}

static unsigned char get_button_data()
{
	unsigned int u32Data = (*(volatile u32 *)(RALINK_REG_PIODATA));	
	unsigned char u8Data = (u32Data >> 22) & 0xff;
	return u8Data;
}
void timer_function(unsigned long arg)
{
	printk(KERN_DEBUG "Timer Trigger\n");
    if(button_driver.u8FlagLed2){
        button_driver.bStateLed2 ? led2_on():led2_off();
        button_driver.bStateLed2 = !button_driver.bStateLed2;
    }
    if(button_driver.u8FlagLed3){
        button_driver.bStateLed3 ? led3_on():led3_off();
        button_driver.bStateLed3 = !button_driver.bStateLed3;
    }
    button_driver.timer_times ++;
    if(button_driver.timer_times > button_driver.timer_led.data * 2){
        if(timer_pending(&button_driver.timer_led)){
            del_timer(&button_driver.timer_led);
        }
    }else{
        mod_timer(&button_driver.timer_led, jiffies + (50));//设置定时器，每隔0.5秒执行一次
    }
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

static int __init button_driver_init(void)
{
	dev_t dev_no = MKDEV(GPIO_MAJOR, GPIO_MINOR);
	int result = 0;
    printk(KERN_DEBUG "Hello Kernel Init!\n");
	
	/*region device version*/
	if(GPIO_MAJOR){
		printk(KERN_DEBUG "static region device\n");
		result = register_chrdev_region(dev_no, GPIO_NUM, GPIO_NAME);//static region
	}else{
		printk(KERN_DEBUG "alloc region device\n");
		result = alloc_chrdev_region(&dev_no, GPIO_MAJOR, GPIO_NUM, GPIO_NAME);//alloc region
		button_driver.driver_major = MAJOR(dev_no);//get major
	}
	if(result < 0){
		printk(KERN_ERR "Region Device Error, %d\n", result);
		return result;
	}
	
	cdev_init(&button_driver.device, &gpio_optns);//将内核设备和文件节点链接起来
    button_driver.device.owner = THIS_MODULE;
    button_driver.device.ops = &gpio_optns;
	
	printk(KERN_DEBUG "cdev_add\n");
	cdev_add(&button_driver.device, (dev_t)MKDEV(button_driver.driver_major, GPIO_MINOR), GPIO_NUM);//regedit the device

    gpio_init(); led2_on(); led3_on();
	
	init_timer(&button_driver.timer_led);
    button_driver.timer_led.function = timer_function;//设置定时器超时函数
	//reset_on();
	//spimiso_on();
    return 0;
}

static void button_driver_exit(void)
{
    printk(KERN_ALERT "unregion the gpio device\n");
	cdev_del(&button_driver.device);//delete the device
	unregister_chrdev_region((dev_t)MKDEV(button_driver.driver_major, GPIO_MINOR), GPIO_NUM);//unregion device
	//del_timer(&timer_key);
}

static int gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	printk(KERN_DEBUG "gpio_write\n");
	
	return 0;
}

static int gpio_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	printk(KERN_DEBUG "gpio_read\n");
	if(size != 1){
        return -EINVAL;	
	}
	
    //if(put_user(put_keyValue, buf)){
    //    return -EFAULT;		 
	//}
    //put_keyValue = 0;
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
    unsigned long val = 0;
    if(copy_from_user(&val, &arg, sizeof(val))){
        return -EFAULT;
    }
	switch(cmd){
		case E_GPIO_DRIVER_LED2_CONTROL:{
			printk(KERN_DEBUG "E_GPIO_DRIVER_LED2_CONTROL, arg %ld\n", arg);
			val ? led2_on() : led2_off();
		}break;
		case E_GPIO_DRIVER_LED3_CONTROL:{
			printk(KERN_DEBUG "E_GPIO_DRIVER_LED3_CONTROL, arg %ld\n", arg);
            val ? led3_on() : led3_off();
        }break;
		case E_GPIO_DRIVER_LED2_FLSAH:{
			printk(KERN_DEBUG "E_GPIO_DRIVER_LED2_FLSAH, arg %ld\n", arg);
            button_driver.timer_led.data = val;//传递给定时器超时函数的值，这里传进来的是闪烁时间
            return mod_timer(&button_driver.timer_led, jiffies + 50);
		}break;
        default:break;
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
module_init(button_driver_init);
module_exit(button_driver_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Pan Chang Tao");
MODULE_DESCRIPTION("The IO Control Driver");
MODULE_ALIAS("A IO Module");
