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
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/rt2880/surfboardint.h>
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
static ssize_t gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos);
static ssize_t gpio_read(struct file *filp, char __user*buf, size_t size, loff_t *ppos);
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
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
static struct work_struct gpio_event_hold;
static struct work_struct gpio_event_click;
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
	//.ioctl 			= NULL,		//executive the cmd, int the later version of linux, used fun unlocked_ioctl replace this fun
	.unlocked_ioctl = gpio_ioctl,	//if system doens't use BLK filesystem ,the use this fun indeeded iotcl
	.compat_ioctl 	= NULL,			//the 32bit program will use this fun replace the ioctl in the 64bit platform
	.mmap			= NULL,			//memory mapping
	.open  			= gpio_open,	//open device
	.flush			= NULL,			//flush device
	.release		= mem_release,	//close the device
	//.synch			= NULL,		//refresh the data
	.aio_fsync		= NULL,			//asynchronouse .synch
	.fasync			= NULL,			//notifacation the device's Flag changed
};
/****************************************************************************/
/***        Local    Functions                                            ***/
/****************************************************************************/
static void gpio_init(void)
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

    //enable gpio interrupt
    //*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
}

static void inline led2_on(void)
{
	(*(volatile u32 *)RALINK_REG_PIODATA) &= cpu_to_le32(~(0x01<<25));
}
static void inline led2_off(void)
{
	(*(volatile u32 *)RALINK_REG_PIODATA) |= cpu_to_le32((0x01<<25));
}
static void inline led3_on(void)
{
	(*(volatile u32 *)RALINK_REG_PIODATA) &= cpu_to_le32(~(0x01<<27));
}
static void inline led3_off(void)
{
	(*(volatile u32 *)RALINK_REG_PIODATA) |= cpu_to_le32((0x01<<27));
}

static unsigned char get_button_data(void)
{
	unsigned int u32Data = (*(volatile u32 *)(RALINK_REG_PIODATA));	
	unsigned char u8Data = (unsigned char)((u32Data >> 22) & 0xff);
	return u8Data;
}
void button_timer_function(unsigned long arg)
{
    unsigned char btn = get_button_data();
    if(!(btn & BUTTON_SW2) || !(btn & BUTTON_SW3) || !(btn & BUTTON_SW4)){
        printk(KERN_DEBUG "get key value[%d]\n", btn);
        button_driver.btn.btn_value = btn;
        del_timer(&button_driver.btn.timer_btn);
        wake_up_interruptible(&button_waitq);
    }
    mod_timer(&button_driver.btn.timer_btn, jiffies + 5);
}
void led2_timer_function(unsigned long arg)
{
    button_driver.led2.bLedState ? led2_on():led2_off();
    button_driver.led2.bLedState = !button_driver.led2.bLedState;

    button_driver.led2.timer_times ++;
    if(button_driver.led2.timer_times > button_driver.led2.timer_led.data * 2){
        del_timer(&button_driver.led2.timer_led);
        button_driver.led2.timer_times = 0;
        button_driver.led2.timer_led.data = 0;
    }else{
        mod_timer(&button_driver.led2.timer_led, jiffies + (50));//设置定时器，每隔0.5秒执行一次
    }
}
void led3_timer_function(unsigned long arg)
{
    printk(KERN_DEBUG "led3_timer_function Trigger\n");
    button_driver.led3.bLedState ? led3_on():led3_off();
    button_driver.led3.bLedState = !button_driver.led3.bLedState;

    button_driver.led3.timer_times ++;
    if(button_driver.led3.timer_times > button_driver.led3.timer_led.data * 2){
        del_timer(&button_driver.led3.timer_led);
        button_driver.led3.timer_times = 0;
        button_driver.led3.timer_led.data = 0;
    }else{
        mod_timer(&button_driver.led3.timer_led, jiffies + (50));//设置定时器，每隔0.5秒执行一次
    }
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
	
	init_timer(&button_driver.led2.timer_led);
	init_timer(&button_driver.led3.timer_led);
    button_driver.led2.timer_led.function = led2_timer_function;//设置定时器超时函数
    button_driver.led3.timer_led.function = led3_timer_function;//设置定时器超时函数

    init_timer(&button_driver.btn.timer_btn);
    button_driver.btn.timer_btn.function = button_timer_function;

    return 0;
}

static void button_driver_exit(void)
{
    printk(KERN_ALERT "unregion the gpio device\n");
    cdev_del(&button_driver.device);//delete the device
	unregister_chrdev_region((dev_t)MKDEV(button_driver.driver_major, GPIO_MINOR), GPIO_NUM);//unregion device
    //disable gpio interrupt
    //*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
}

static ssize_t gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	printk(KERN_DEBUG "gpio_write\n");
	
	return 0;
}

static ssize_t gpio_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	printk(KERN_DEBUG "gpio_read\n");
	if(size != 1){
        return -EINVAL;	
	}
	
    if(copy_to_user(buf, button_driver.btn.btn_value, 1)){
        return -EFAULT;
	}
    button_driver.btn.btn_value = 0;
	return 0;
}

void ralink_gpio_notify_user(int usr)
{
    struct task_struct *ptask = NULL;
    printk(KERN_DEBUG "ralink_gpio_notify_user:%d\n", usr);

    if(button_driver.pid < 2){
        printk(KERN_ERR GPIO_NAME ":can't send any signal if pid is 0 or 1\n");
        return;;
    }
    ptask = pid_task(find_vpid(button_driver.pid), PIDTYPE_PID);
    if(NULL == ptask){
        printk(KERN_ERR GPIO_NAME ":no registered process to notify\n");
        return;
    }
    if (usr == 1) {
        printk(KERN_NOTICE GPIO_NAME ": sending a SIGUSR1 to process %d\n",button_driver.pid);
        send_sig(SIGUSR1, ptask, 0);
    }
    else if (usr == 2) {
        printk(KERN_NOTICE GPIO_NAME ": sending a SIGUSR2 to process %d\n",button_driver.pid);
        send_sig(SIGUSR2, ptask, 0);
    }
}

void gpio_click_notify(struct work_struct *work)
{
    printk("<hua-dbg> %s, 1\n", __FUNCTION__);
    ralink_gpio_notify_user(1);
}


void gpio_hold_notify(struct work_struct *work)
{
    printk("<hua-dbg> %s, 2\n", __FUNCTION__);
    ralink_gpio_notify_user(2);
}

irqreturn_t ralink_gpio_irq_handler(int irq, void *irqaction)
{
    unsigned char btn = 0;
    unsigned int ralink_gpio_intp =0;
    unsigned int ralink_gpio_edge = 0;
    struct gpio_time_record {
        unsigned long falling;
        unsigned long rising;
    };
    static struct gpio_time_record record;
    printk(KERN_DEBUG "ralink_gpio_irq_handler\n");

    ralink_gpio_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOINT));
    ralink_gpio_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOEDGE));
    *(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0xFFFFFFFF);
    *(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0xFFFFFFFF);
    if(ralink_gpio_edge & KEYS){
        if (time_before(jiffies, record.falling + 200L)) {
            //one click
            schedule_work(&gpio_event_click);
        }
        else {
            //press for several seconds
            schedule_work(&gpio_event_hold);
        }
    } else {
        btn = get_button_data();
        if(!(btn & BUTTON_SW2) || !(btn & BUTTON_SW3) || !(btn & BUTTON_SW4)){
            printk(KERN_DEBUG "get key value[%d]\n", btn);
            button_driver.btn.btn_value = btn;
        }
        record.falling = jiffies;
    }

    return IRQ_HANDLED;
}

static int gpio_open(struct inode *inode, struct file *filp)
{
    int result = 0;
	printk(KERN_DEBUG "gpio_open\n");
    try_module_get(THIS_MODULE);

    result = request_irq(SURFBOARDINT_GPIO, ralink_gpio_irq_handler, IRQF_DISABLED, "ralink_gpio", NULL);
    if(result){
        printk(KERN_INFO "request irq error:%d\n", result);
        return result;
    }

    INIT_WORK(&gpio_event_hold, gpio_hold_notify);
    INIT_WORK(&gpio_event_click, gpio_click_notify);

	return 0;
}

inline static unsigned gpio_poll(struct file *filp, poll_table *pwait)
{
	printk(KERN_DEBUG "gpio_poll\n");
    //button_driver.btn.timer_btn.expires = jiffies + 5;
    //add_timer(&button_driver.btn.timer_btn);//启动定时器

    poll_wait(filp, &button_waitq, pwait);
    if(button_driver.btn.btn_value != 0){
        return (POLL_IN | POLLRDNORM);
    }
	return 0;
}

long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    unsigned long val = 0;
    pid_t pid;
	printk(KERN_DEBUG "gpio_ioctl\n");

    switch(cmd){
		case E_GPIO_DRIVER_LED2_CONTROL:{
            //arg并不是用户空间传进来的值，而是一个指针
            if(copy_from_user(&val, (unsigned long*)arg, sizeof(val))){
                return -EFAULT;
            }
            printk(KERN_DEBUG "E_GPIO_DRIVER_LED2_CONTROL, arg %ld\n", arg);
			val ? led2_on() : led2_off();
		}break;
		case E_GPIO_DRIVER_LED3_CONTROL:{
            if(copy_from_user(&val, (unsigned long*)arg, sizeof(val))){
                return -EFAULT;
            }
            printk(KERN_DEBUG "E_GPIO_DRIVER_LED3_CONTROL, arg %ld\n", arg);
            val ? led3_on() : led3_off();
        }break;
		case E_GPIO_DRIVER_LED2_FLSAH:{
            if(copy_from_user(&val, (unsigned long*)arg, sizeof(val))){
                return -EFAULT;
            }
			printk(KERN_DEBUG "E_GPIO_DRIVER_LED2_FLSAH, arg %ld\n", arg);
            button_driver.led2.timer_led.data = val;//传递给定时器超时函数的值，这里传进来的是闪烁时间
            return mod_timer(&button_driver.led2.timer_led, jiffies + 50);
		}break;
        case E_GPIO_DRIVER_LED3_FLSAH:{
            if(copy_from_user(&val, (unsigned long*)arg, sizeof(val))){
                return -EFAULT;
            }
            printk(KERN_DEBUG "E_GPIO_DRIVER_LED3_FLSAH, arg %ld\n", arg);
            button_driver.led3.timer_led.data = val;//传递给定时器超时函数的值，这里传进来的是闪烁时间
            return mod_timer(&button_driver.led3.timer_led, jiffies + 50);
        }break;
        case E_GPIO_DRIVER_ENABLE_KEY_INTERUPT:{
            if(copy_from_user(&pid, (pid_t*)arg, sizeof(pid))){
                return -EFAULT;
            }
            button_driver.pid = pid;
            printk(KERN_DEBUG "E_GPIO_DRIVER_ENABLE_KEY_INTERUPT\n");
            (*(volatile u32 *)RALINK_REG_INTENA) 	= cpu_to_le32(RALINK_INTCTL_PIO);
            unsigned long tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
            tmp |= ((1<<22)|(1<<24)|(1<<28));
            (*(volatile u32 *)RALINK_REG_PIORENA) 	= cpu_to_le32(tmp);
            tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
            tmp |= ((1<<22)|(1<<24)|(1<<28));
            (*(volatile u32 *)RALINK_REG_PIOFENA) 	= cpu_to_le32(tmp);
        }break;
        case E_GPIO_DRIVER_DISABLE_KEY_INTERUPT:{
            printk(KERN_DEBUG "E_GPIO_DRIVER_DISABLE_KEY_INTERUPT\n");
            (*(volatile u32 *)RALINK_REG_INTDIS) 	= cpu_to_le32(RALINK_INTCTL_PIO);
        }break;
        default:break;
	}
	return 0;
}

int mem_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "mem_release\n");
    free_irq(SURFBOARDINT_GPIO, NULL);
    module_put(THIS_MODULE);
	return 0;
}
#if 0
irqreturn_t ralink_gpio_irq_handler(int irq, void *irqaction)
{
    printk(KERN_DEBUG "ralink_gpio_irq_handler\n");
    struct gpio_time_record {
        unsigned long falling;
        unsigned long rising;
    };
    static struct gpio_time_record record;
    unsigned int ralink_gpio_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOINT));
    unsigned int ralink_gpio_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOEDGE));
    *(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0xFFFFFFFF);
    *(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0xFFFFFFFF);
    if(ralink_gpio_edge & KEYS){
        if (time_before(jiffies, record.falling + 200L)) {
            //one click
            schedule_work(&gpio_event_click);
        }
        else {
            //press for several seconds
            schedule_work(&gpio_event_hold);
        }
    } else {
        record.falling = jiffies;
    }

    return IRQ_HANDLED;
}
struct irqaction ralink_gpio_irqaction = {
        .handler = ralink_gpio_irq_handler,
        .flags = IRQF_DISABLED,
        .name = GPIO_NAME,
};

void __init ralink_gpio_init_irq(void)
{
    printk(KERN_DEBUG "ralink_gpio_init_irq\n");
    setup_irq(SURFBOARDINT_GPIO, &ralink_gpio_irqaction);
}
#endif
/****************************************************************************/
/***        Kernel    Module                                              ***/
/****************************************************************************/
module_init(button_driver_init);
module_exit(button_driver_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Pan Chang Tao");
MODULE_DESCRIPTION("The IO Control Driver");
MODULE_ALIAS("A IO Module");
