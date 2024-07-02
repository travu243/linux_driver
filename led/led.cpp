/*
gpio led control for orange pi zero 3

toggle gpio pc7

*/

#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/io.h>
//#include <linux/delay.h>

#define GPIO_ADDR_BASE  				0x0300B000
#define ADDR_SIZE       				(0x1023)

#define PC_DATA_OFFSET          		0x0058

#define PC_CFG0_OFFSET                  0x0048
#define LED_BIT_CONFIG                  (0b001 << 28)

#define LED_STAGE_ON					(1<<7)
#define LED_STAGE_OFF					~(1<<7)

//#define LED_STAGE_ON					(1<<24)
//#define LED_STAGE_OFF					~(1<<24)


void __iomem *base_addr;
uint32_t old_pin_mode;
uint32_t reg_data = 0;


static int __init init_module(void){
	
	printk(KERN_INFO "turn on\n");
	
	base_addr = ioremap(GPIO_ADDR_BASE, ADDR_SIZE);

	reg_data = readl_relaxed(base_addr + PC_CFG0_OFFSET);
	old_pin_mode = reg_data;
	reg_data &= LED_BIT_CONFIG;
	writel_relaxed(reg_data, base_addr + PC_CFG0_OFFSET);
	
	writel_relaxed(LED_STAGE_ON,  base_addr + PC_DATA_OFFSET);

	return 0;
}

static void __exit cleanup_module(void){
	
	printk(KERN INFO "turn off\n");
	
	writel_relaxed(LED_STAGE_OFF, base_addr + PC_DATA_OFFSET);
	
	writel_relaxed(old_pin_mode, base_addr + PC_CFG0_OFFSET);
	return;
}

module_init(init_module);
module_exit(cleanup_module);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("TRA VU");
MODULE_DESCRIPTION("LED OPZ3");





