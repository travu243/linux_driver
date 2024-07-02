/*
gpio led control for orange pi zero 3

turn on/off gpio pc7 by writing to /dev/led_char_device char '1'/'0'

*/

//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("TRA VU");
//MODULE_DESCRIPTION("LED OPZ3");


#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GPIO_ADDR_BASE                          0x0300B000
#define ADDR_SIZE                               (0x1023)

#define PC_DATA_OFFSET                          0x0058

#define PC_CFG0_OFFSET                          0x0048
#define LED_BIT_CONFIG                          (0b001 << 28)

#define LED_STAGE_ON                            (1<<7)
#define LED_STAGE_OFF                           (0<<7)

#define IOCTL_DATA_LEN                          1024
#define SEND_DATA_CMD                           _IOW(100,1,char*)

#define DEVICE                                  2
#define DEVICE_NAME                             "led_char_device"


struct class *my_class;
struct device *my_dev;
struct cdev my_cdev;
static dev_t dev_num;
bool first_oper;
char data[4096];
char config_data[IOCTL_DATA_LEN];

void __iomem *base_addr;
uint32_t old_pin_mode;
uint32_t reg_data =0;


static int my_open(struct inode *inode, struct file *file){
    printk(KERN_INFO "open %s, %d\n",__func__,__LINE__);
    
    reg_data = readl_relaxed(base_addr + PC_CFG0_OFFSET);
    old_pin_mode = reg_data;
    reg_data &= LED_BIT_CONFIG;
    writel_relaxed(reg_data, base_addr + PC_CFG0_OFFSET);
    first_oper = true;

    return 0;
}

static int my_close(struct inode *inode, struct file *file){
	
    printk(KERN_INFO "close %s, %d\n", __func__, __LINE__);
//  writel_relaxed(old_pin_mode, base_addr + PC_CFG0_OFFSET);

    return 0;
}

static ssize_t my_read(struct file *flip, char __user *user_buf, size_t len, loff_t *offs){
	
    char *data = NULL;
    
    data = kmalloc(len, GFP_KERNEL);                //malloc buffer optimized for kernel
    memset(data, 0, len);
    reg_data = readl_relaxed(base_addr + PC_DATA_OFFSET);
    data[0] = ((reg_data>>7)&0b1)+48;                       //led+48->asscii=0/1
    if(copy_to_user(user_buf, data, len))
            return -EFAULT;

    printk(KERN_INFO "pc7 stage: %c",data[0]);

    if(true == first_oper){
            first_oper = false;
            return 1;
    }
    else
            return 0;
}

static ssize_t my_write(struct file *flip, const char __user *user_buf, size_t len, loff_t *offs){
    char data = '\0';

    if(copy_from_user(&data, &user_buf[0], sizeof(data)))
            return -EFAULT;

    switch (data){
        case '1':
            writel_relaxed(LED_STAGE_ON,  base_addr + PC_DATA_OFFSET);
            break;
        case '0':
            writel_relaxed(LED_STAGE_OFF,  base_addr + PC_DATA_OFFSET);
            break;
        default:
            printk(KERN_INFO "unsupport operation");
    };
    printk(KERN_INFO "write %c to gpio pc7\n",data);
    return len;
}

static long my_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){          //i/o control system call
    switch (cmd){
        case SEND_DATA_CMD:
        memset(config_data, 0, IOCTL_DATA_LEN);
        if(copy_from_user(config_data, (char*)arg, IOCTL_DATA_LEN))             //command cp into config_data
                return -EFAULT;

        printk(KERN_INFO "%s, %d, ioctl message = %s\n", __func__, __LINE__, config_data);
        break;
    default:
        return -ENOTTY;
    }
        return 0;
}


static struct file_operations fops = {
    .owner=THIS_MODULE,
    .open=my_open,
    .release=my_close,
    .read=my_read,
    .write=my_write,
    .unlocked_ioctl=my_ioctl,
};


static int __init Init_Module(void){
	
    printk(KERN_INFO "init\n");

    memset(data, 0, sizeof(data));

    alloc_chrdev_region(&dev_num,0,DEVICE, DEVICE_NAME);            //register major, minor number for cdevice
    my_class=class_create(THIS_MODULE, DEVICE_NAME);                //create class device
    cdev_init(&my_cdev, &fops);                                     //init cdevice with file operation struct
    my_cdev.owner=THIS_MODULE;
    cdev_add(&my_cdev,dev_num,1);                                   //add cdevice with major,minor number to system
    device_create(my_class,NULL,dev_num,NULL,DEVICE_NAME);          //create device with class dev has initialized by class_create

    base_addr = ioremap(GPIO_ADDR_BASE, ADDR_SIZE);

    return 0;
}

static void __exit Cleanup_Module(void){

    printk(KERN_INFO "exit\n");

    writel_relaxed(LED_STAGE_OFF,  base_addr + PC_DATA_OFFSET);

    cdev_del(&my_cdev);
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    unregister_chrdev(dev_num,DEVICE_NAME);

    writel_relaxed(old_pin_mode, base_addr + PC_CFG0_OFFSET);

    return ;
}

module_init(Init_Module);
module_exit(Cleanup_Module);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("TRA VU");




