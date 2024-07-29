#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/kthread.h>


#define PC_DATA_OFFSET                          0x0058

#define PC_CFG0_OFFSET                          0x0048
#define LED_BIT_CONFIG                          (0b001 << 28)

#define LED_STAGE_ON                            (1<<7)
#define LED_STAGE_OFF                           (0<<7)

void __iomem *base_addr;
uint32_t old_pin_mode;


static const struct of_device_id blink_led_of_match[] = {
	{ .compatible = "test_led",},
	{},
};

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


int blink_led_probe(struct platform_device *pdev){
	uint32_t reg_data = 0;
//	struct resource *res = NULL;
	int addr;

	printk(KERN_INFO "Hello world\n");
	
	ret = device_property_read_u32(dev, "reg", &addr);
    if(ret) {
            printk("Error! Could not read 'reg'\n");
            return -1;
    }
    
    base_addr = ioremap(addr, 0x1000);

//	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
//	base_addr = ioremap(res->start, res->end - res->start);
	reg_data = readl_relaxed(base_addr + PC_CFG0_OFFSET);
    old_pin_mode = reg_data;
    reg_data &= LED_BIT_CONFIG;
    writel_relaxed(reg_data, base_addr + PC_CFG0_OFFSET);
	
	
	kthread = kthread_create(func_thread,NULL,"led_toggle");
    wake_up_process(kthread);

	return 0;
}

int blink_led_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "Goodbye world\n");
	
	writel_relaxed(LED_STAGE_OFF, base_addr + PC_DATA_OFFSET);
	
	writel_relaxed(old_pin_mode, base_addr + PC_CFG0_OFFSET);

    kthread_stop(kthread);
	return 0;
}

static struct platform_driver blink_led_driver = {
	.probe		= blink_led_probe,
	.remove		= blink_led_remove,
	.driver		= {
		.name	= "blink_led",
		.of_match_table = blink_led_of_match,
	},
};


module_platform_driver(blink_led_driver);




MODULE_LICENSE("GPL");






