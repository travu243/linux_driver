/*
gpio led control for orange pi zero 3

toggle gpio pc7

*/

//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("TRA VU");
//MODULE_DESCRIPTION("LED OPZ3");


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>



#define GPIO_ADDR_BASE                          0x0300B000
#define ADDR_SIZE                               (0x1023)

#define PC_DATA_OFFSET                          0x0058
#define PH_DATA_OFFSET                          0x10C

#define PC_CFG0_OFFSET                          0x0048	//for pc7
#define PC_CFG1_OFFSET                          0x004C	//for pc9 and pc11

#define PH_CFG0_OFFSET                          0x00FC	//for ph2, ph3, ph6, ph7
#define PH_CFG1_OFFSET                          0x0100	//for ph8

#define pwm1_BIT_CONFIG                         (0b001 << 12)	//pc11
#define pwm2_BIT_CONFIG                         (0b001 << 12)	//ph3
#define pwm3_BIT_CONFIG                         (0b001 << 8)	//ph2
#define pwm4_BIT_CONFIG                         (0b001 << 4)	//pc9

#define dir1_BIT_CONFIG                         (0b001 << 28)	//pc7
#define dir2_BIT_CONFIG                         (0b001 << 24)	//ph6
#define dir3_BIT_CONFIG                         (0b001 << 0)	//ph8
#define dir4_BIT_CONFIG                         (0b001 << 28)	//ph7

#define pwm1_stage	                            11		//pc
#define pwm2_stage	                            3		//ph
#define pwm3_stage	                            2		//ph
#define pwm4_stage	                            9		//pc

#define dir1_on		                            (1<<7)	//pc
#define dir1_off	                            ~(1<<7)
#define dir2_on		                            (1<<6)	//ph
#define dir2_off	                            ~(1<<6)
#define dir3_on		                            (1<<8)	//ph
#define dir3_off	                            ~(1<<8)
#define dir4_on		                            (1<<7)	//ph
#define dir4_off	                            ~(1<<7)


#define IOCTL_DATA_LEN                          1024
#define SEND_DATA_CMD                           _IOW(100,1,char*)

#define DEVICE                                  2
#define DEVICE_NAME                             "softpwm"





#define mor_FrontRight	 									duty_cycle2										
#define mor_FrontRight_back			  				writel_relaxed(dir2_on,  base_addr + PH_DATA_OFFSET)
#define mor_FrontRight_next							writel_relaxed(dir2_off,  base_addr + PH_DATA_OFFSET)

#define mor_RearRight	 									duty_cycle4								
#define mor_RearRight_next							writel_relaxed(dir4_off,  base_addr + PH_DATA_OFFSET)	
#define mor_RearRight_back							writel_relaxed(dir4_on,  base_addr + PH_DATA_OFFSET)

#define mor_RearLeft	 									duty_cycle3			
#define mor_RearLeft_next							writel_relaxed(dir3_on,  base_addr + PH_DATA_OFFSET)
#define mor_RearLeft_back							writel_relaxed(dir3_off,  base_addr + PH_DATA_OFFSET)	

#define mor_FrontLeft	 									duty_cycle1	 							
#define mor_FrontLeft_back							writel_relaxed(dir1_off,  base_addr + PC_DATA_OFFSET)	
#define mor_FrontLeft_next							writel_relaxed(dir1_on,  base_addr + PC_DATA_OFFSET)





void setMotor(int speed_FL, int speed_FR, int speed_RL, int speed_RR)
{
	if(!speed_FL) mor_FrontLeft = motorLock;
	else if(speed_FL > 0)
 	{
	 	mor_FrontLeft = speed_FL;
		mor_FrontLeft_next;
 	}else{
		mor_FrontLeft = 254+speed_FL;
		mor_FrontLeft_back;
 	}

	if(!speed_FR) mor_FrontRight = motorLock;
	else if(speed_FR > 0)
 	{
	 	mor_FrontRight = speed_FR;
		mor_FrontRight_next;
 	}else{
		mor_FrontRight = 254+speed_FR;
		mor_FrontRight_back;
 	}

	if(!speed_RL) mor_RearLeft = motorLock;
	else if(speed_RL > 0)
 	{
	 	mor_RearLeft = speed_RL;
		mor_RearLeft_next;
 	}else{
		mor_RearLeft = 254+speed_RL;
		mor_RearLeft_back;
 	}

 	if(!speed_RR) mor_RearRight = motorLock;
	else if(speed_RR > 0)
 	{
	 	mor_RearRight = speed_RR;
		mor_RearRight_next;
 	}else{
		mor_RearRight = 254+speed_RR;
		mor_RearRight_back;
 	}
}







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

static struct task_struct *kthread1;
static struct task_struct *kthread2;
static struct task_struct *kthread3;
static struct task_struct *kthread4;

int period=1000;
int duty_cycle1=0;
int duty_cycle2=0;
int duty_cycle3=0;
int duty_cycle4=0;

void pwm(int duty_cycle,int bit_stage, int port_offset){
	writel_relaxed(1<<bit_stage,  base_addr + port_offset);
	usleep_range(duty_cycle,duty_cycle+1);
    writel_relaxed(~(1<<bit_stage),  base_addr + port_offset);
	usleep_range(period-duty_cycle,period-duty_cycle+1);
	
	return;
}

static int pwm1_thread(void *data){

    while (!kthread_should_stop()){
    	pwm(duty_cycle1,pwm1_stage,PC_DATA_OFFSET);
    }
    return 0;
}
static int pwm2_thread(void *data){

    while (!kthread_should_stop()){
    	pwm(duty_cycle2,pwm2_stage,PC_DATA_OFFSET);
    }
    return 0;
}
static int pwm3_thread(void *data){

    while (!kthread_should_stop()){
    	pwm(duty_cycle3,pwm3_stage,PC_DATA_OFFSET);
    }
    return 0;
}
static int pwm4_thread(void *data){

    while (!kthread_should_stop()){
    	pwm(duty_cycle4,pwm4_stage,PC_DATA_OFFSET);
    }
    return 0;
}

static int my_open(struct inode *inode, struct file *file){
    printk(KERN_INFO "open %s, %d\n",__func__,__LINE__);
    
    first_oper = true;

    return 0;
}

static int my_close(struct inode *inode, struct file *file){
	
    printk(KERN_INFO "close %s, %d\n", __func__, __LINE__);

    return 0;
}

static ssize_t my_read(struct file *flip, char __user *user_buf, size_t len, loff_t *offs){
	
    int *data = NULL;
    
    data = kmalloc(len, GFP_KERNEL);                //malloc buffer optimized for kernel
    memset(data, 0, len);

    *data = duty_cycle1;
    if(copy_to_user(user_buf, data, len))
            return -EFAULT;

    printk(KERN_INFO "%d",*data);

    if(true == first_oper){
            first_oper = false;
            return 1;
    }
    else
            return 0;
}

static ssize_t my_write(struct file *flip, const char __user *user_buf, size_t len, loff_t *offs){
    char *data = '\0';

    if(copy_from_user(&data, &user_buf, len))
            return -EFAULT;

    switch (data){
        case "forward left":
        case "left forward":
            setMotor(200,0,200,0);
            break;
        case "forward right":
        case "right forward":
            setMotor(0,200,0,200);
            break;
        case "backward left":
        case "left backward":
        	setMotor(0,-200,0,-200);
            break;
        case "backward right":
        case "right backward":
        	setMotor(-200,0,-200,0);
            break;
        case "forward":
        	setMotor(200,-200,200,-200);
            break;
        case "backward":
        	setMotor(-200,200,-200,200);
            break;
        case "left":
        	setMotor(-200,-200,200,200);
            break;
        case "right":
        	setMotor(200,200,-200,-200);
            break;
        default:
        	robotStop(5);
            printk(KERN_INFO "unsupport operation");
    };
    printk(KERN_INFO "%s\n",data);
    return len;
}

static long my_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){  //i/o control system call
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

void init_pc(){
	reg_data = readl_relaxed(base_addr + PC_CFG0_OFFSET);
    reg_data &= dir1_BIT_CONFIG;
    writel_relaxed(reg_data, base_addr + PC_CFG0_OFFSET);
    
    reg_data = readl_relaxed(base_addr + PC_CFG1_OFFSET);
    reg_data &= pwm4_BIT_CONFIG;
    reg_data &= pwm1_BIT_CONFIG;
    writel_relaxed(reg_data, base_addr + PC_CFG1_OFFSET);
    
    return;
}

void init_ph(){
	reg_data = readl_relaxed(base_addr + PH_CFG0_OFFSET);
    reg_data &= pwm3_BIT_CONFIG;
    reg_data &= pwm2_BIT_CONFIG;
    reg_data &= dir2_BIT_CONFIG;
    reg_data &= dir4_BIT_CONFIG;
    writel_relaxed(reg_data, base_addr + PH_CFG0_OFFSET);
    
    reg_data = readl_relaxed(base_addr + PH_CFG1_OFFSET);
    reg_data &= dir3_BIT_CONFIG;
    writel_relaxed(reg_data, base_addr + PH_CFG1_OFFSET);
    
    return;
}

static int __init Init_Module(void){
	
    printk(KERN_INFO "hello\n");

    reg_data = 0;

    base_addr = ioremap(GPIO_ADDR_BASE, ADDR_SIZE);

    init_pc();
    init_ph():
    
    memset(data, 0, sizeof(data));

    alloc_chrdev_region(&dev_num,0,DEVICE, DEVICE_NAME);            //register major, minor number for cdevice
    my_class=class_create(THIS_MODULE, DEVICE_NAME);                //create class device
    cdev_init(&my_cdev, &fops);                                     //init cdevice with file operation struct
    my_cdev.owner=THIS_MODULE;
    cdev_add(&my_cdev,dev_num,1);                                   //add cdevice with major,minor number to system
    device_create(my_class,NULL,dev_num,NULL,DEVICE_NAME);          //create device with class dev has initialized by class_create
        

    kthread1 = kthread_create(pwm1_thread,NULL,"pwm1");
    kthread2 = kthread_create(pwm2_thread,NULL,"pwm2");
    kthread3 = kthread_create(pwm3_thread,NULL,"pwm3");
    kthread4 = kthread_create(pwm4_thread,NULL,"pwm4");
    wake_up_process(kthread1);
    wake_up_process(kthread2);
    wake_up_process(kthread3);
    wake_up_process(kthread4);

    return 0;
}

static void __exit Cleanup_Module(void){
    printk(KERN_INFO "goodbye\n");
    
    cdev_del(&my_cdev);
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    unregister_chrdev(dev_num,DEVICE_NAME);

	writel_relaxed(0<<pwm1_stage, base_addr + PC_DATA_OFFSET);
	writel_relaxed(0<<pwm2_stage, base_addr + PH_DATA_OFFSET);
	writel_relaxed(0<<pwm3_stage, base_addr + PH_DATA_OFFSET);
	writel_relaxed(0<<pwm4_stage, base_addr + PC_DATA_OFFSET);
	
	writel_relaxed(dir1_off, base_addr + PC_DATA_OFFSET);
	writel_relaxed(dir2_off, base_addr + PH_DATA_OFFSET);
	writel_relaxed(dir3_off, base_addr + PH_DATA_OFFSET);
	writel_relaxed(dir4_off, base_addr + PH_DATA_OFFSET);
	

    kthread_stop(kthread1);
    kthread_stop(kthread2);
    kthread_stop(kthread3);
    kthread_stop(kthread4);

    return ;
}

module_init(Init_Module);
module_exit(Cleanup_Module);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("TRA VU");


