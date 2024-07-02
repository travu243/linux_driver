/*
gpio led control for orange pi zero 3

toggle gpio pc7

*/

//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("TRA VU");
//MODULE_DESCRIPTION("LED OPZ3");


#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/time.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#define GPIO_ADDR_BASE                          0x0300B000
#define ADDR_SIZE                               (0x1023)

#define PC_DATA_OFFSET                          0x0058

#define PC_CFG0_OFFSET                          0x0048
#define LED_BIT_CONFIG                          (0b001 << 28)

#define LED_STAGE_ON                            (1<<7)
#define LED_STAGE_OFF                           (0<<7)

//#define LED_STAGE_ON                          (1<<24)
//#define LED_STAGE_OFF                         (0<<24)


void __iomem *base_addr;
uint32_t old_pin_mode;
uint32_t reg_data =0;

static struct task_struct *kthread;

static int func_thread(void *data){
        int count=0;
        while (!kthread_should_stop()){
                if((count%2==0))
                        writel_relaxed(LED_STAGE_ON,  base_addr + PC_DATA_OFFSET);
                else
                        writel_relaxed(LED_STAGE_OFF,  base_addr + PC_DATA_OFFSET);
                count++;
                if(count==100) count=0;
                msleep(1000);
        }
        return 0;
}

static int __init Init_Module(void){
	
        printk(KERN_INFO "hello\n");

        reg_data = 0;

        base_addr = ioremap(GPIO_ADDR_BASE, ADDR_SIZE);

        reg_data = readl_relaxed(base_addr + PC_CFG0_OFFSET);
        old_pin_mode = reg_data;
        reg_data &= LED_BIT_CONFIG;
        writel_relaxed(reg_data, base_addr + PC_CFG0_OFFSET);

        kthread = kthread_create(func_thread,NULL,"led_toggle");
        wake_up_process(kthread);

        return 0;
}

static void __exit Cleanup_Module(void){
        printk(KERN_INFO "goodbye\n");

        writel_relaxed(LED_STAGE_OFF, base_addr + PC_DATA_OFFSET);

        writel_relaxed(old_pin_mode, base_addr + PC_CFG0_OFFSET);

        kthread_stop(kthread);

        return ;
}

module_init(Init_Module);
module_exit(Cleanup_Module);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("TRA VU");


