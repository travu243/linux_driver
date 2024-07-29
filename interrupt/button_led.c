/*
gpio led control for orange pi zero 3

button external interrupt control gpio pc7

*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/init.h>


#define GPIO_ADDR_BASE                          0x0300B000
#define ADDR_SIZE                               (0x1023)

#define PC_CFG0_OFFSET                          0x0048
#define LED_BIT_CONFIG                          (0b001 << 28)
#define PC_DATA_OFFSET                          0x0058

#define LED_STAGE_ON                            (1<<7)
#define LED_STAGE_OFF                           ~(1<<7)

#define PH_CFG0_OFFSET                          0x00FC
#define BUTTON_BIT_CONFIG                       (0b110 << 8)    //set gpio ph2 to external interrupt
#define PH_DATA_OFFSET                          0x10C           //PH2_STAGE = (PH_DATA_OFFSET>>2)&0b1

#define PH_EINT_CTL                             0x02F0          //to enable/disable external interrupt, enable eint PH2>#define PH_EINT_STATUS                          0x02F4          //bit 0:no irq pending, bit 1:irq pending
#define PH_EINT_DEB                             0x02F8

#define PH_EINT_CFG0                            0x02E0          //external interrupt config port H
#define EINT2_CFG_BIT                           (0b0000 <<8)    //positive edge

#define PC_PULL0                                0x0064

void __iomem *base_addr;
uint32_t old_pin_mode_led;
uint32_t reg_data_led = 0;

uint32_t old_pin_mode_button;
uint32_t reg_data_button = 0;

unsigned int irq_eint;

uint8_t current_led_stage;


irqreturn_t external_interrupt_handler(int irq,void *dev_id){

        printk("trigger!\n");

        if(current_led_stage==0){
                writel_relaxed(LED_STAGE_ON, base_addr + PC_DATA_OFFSET);
                current_led_stage=1;
        }
		 else{
                writel_relaxed(LED_STAGE_OFF, base_addr + PC_DATA_OFFSET);
                current_led_stage=0;
        }
        return IRQ_HANDLED;
}


static int __init Init_module(void){

        printk(KERN_INFO "start\n");

        base_addr = ioremap(GPIO_ADDR_BASE, ADDR_SIZE);

        //led config********************************************
        reg_data_led = readl_relaxed(base_addr + PC_CFG0_OFFSET);
        old_pin_mode_led = reg_data_led;
        reg_data_led &= LED_BIT_CONFIG;
        writel_relaxed(reg_data_led, base_addr + PC_CFG0_OFFSET);

        //button config*****************************************
//      reg_data_button = readl_relaxed(base_addr + PH_CFG0_OFFSET);
//      old_pin_mode_button = reg_data_button;
//      reg_data_button &= BUTTON_BIT_CONFIG;
//      writel_relaxed(reg_data_button, base_addr + PH_CFG0_OFFSET);

		if(gpio_request(72,"button")){
                printk("can't allocate button\n");
                return -1;
        }
        if(gpio_direction_input(72)) {
                printk("Error!\nCan not set GPIO 72 to input!\n");
                gpio_free(72);
                return -1;
        }

        writel_relaxed((1<<16),base_addr+PC_PULL0);             //pull-up

        //external config***************************************
//      writel_relaxed(EINT2_CFG_BIT, base_addr + PH_EINT_CFG0);
//      writel_relaxed((1<<2), base_addr + PH_EINT_CTL);


        irq_eint=gpio_to_irq(72);
        if(request_irq(irq_eint,(irq_handler_t)external_interrupt_handler,IRQF_SHARED,"EINT2",(void *)external_interrup>                printk("init EINT2 false\n");
                gpio_free(72);
                return -1;

        }
		
		current_led_stage = 0;

//      writel_relaxed(LED_STAGE_ON,  base_addr + PC_DATA_OFFSET);

        return 0;
}

static void __exit Cleanup_module(void){

        printk(KERN_INFO "end\n");

        writel_relaxed(LED_STAGE_OFF, base_addr + PC_DATA_OFFSET);

        writel_relaxed(old_pin_mode_led, base_addr + PC_CFG0_OFFSET);

//      writel_relaxed(old_pin_mode_button, base_addr + PH_CFG0_OFFSET);

        iounmap(base_addr);
        free_irq(irq_eint,(void *)external_interrupt_handler);
        gpio_free(72);

        return;
}

module_init(Init_module);
module_exit(Cleanup_module);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("TRA VU");
MODULE_DESCRIPTION("LED OPZ3");		





