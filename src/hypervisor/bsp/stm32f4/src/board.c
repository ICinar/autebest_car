/*
 * board.c
 *
 * Board initialization for STM32F4 with Cortex-M4
 *
 * azuepke, 2015-06-26: cloned from QEMU ARM
 */

#include <kernel.h>
#include <assert.h>
#include <arm_insn.h>
#include <board.h>
#include <arm_private.h>
#include <arm_io.h>
#include <board_stuff.h>
#include <nvic.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <hm.h>
#include <gpio.h>


__cold void board_halt(haltmode_t mode __unused)
{
	if ((mode == BOARD_RESET) || (mode == BOARD_HM_RESET)) {
		/* trigger reset */
		AIRCR = AIRCR_VECTKEY | AIRCR_SYSRESETREQ;
		arm_dsb();
	}

	/* just halt the machine */
	__board_halt();
}

void board_idle(void)
{
	while (1) {
		arm_dsb();
		arm_wfi();
	}
}

void __init board_mpu_init(void)
{
	/* on Cortex-M3/M4, the kernel runs with MPU disabled */
	MPU_CTRL = MPU_CTRL_PRIVDEFENA | MPU_CTRL_HFNMIENA | MPU_CTRL_ENABLE;
}

void __init board_cpu0_up(void)
{
}

void __init board_startup_complete(void)
{
}

void board_nmi_dispatch(unsigned int vector __unused)
{
	hm_system_error(HM_ERROR_NMI, vector);
}

int board_hm_exception(
	struct arch_reg_frame *regs __unused,
	int fatal __unused,
	unsigned int hm_error_id __unused,
	unsigned long vector __unused,
	unsigned long fault_addr __unused,
	unsigned long aux __unused)
{
	return 0;	/* exception not handled */
}

void board_tp_switch(
	unsigned int prev_timepart __unused,
	unsigned int next_timepart __unused,
	unsigned int tpwindow_flags __unused)
{
}

static inline void gpio_pin_cfg(
	volatile struct gpio_regs *gpio_regs,
	unsigned int pin,
	unsigned int af,		/**< alternative function */
	unsigned int mode,		/**< 0:input, 1:output, 2:alternate, 3:analog */
	unsigned int otype,		/**< 0:push-pull, 1:open-drain */
	unsigned int ospeed,	/**< 0:low, 1:medium, 2:fast, 3:high */
	unsigned int pupd)		/**< 0: no, 1:pull-up, 2:pull-down */
{
	gpio_regs->afr[pin / 8] &= ~(0xf << ((pin % 8)*4));
	gpio_regs->afr[pin / 8] |= (af << ((pin % 8)*4));
	/* mode alternate */
	gpio_regs->moder &= ~(0x3 << (pin*2));
	gpio_regs->moder |= (mode << (pin*2));
	/* push-pull */
	gpio_regs->otyper &= ~(0x1 << (pin*1));
	gpio_regs->otyper |= (otype << (pin*1));
	/* fast speed */
	gpio_regs->ospeedr &= ~(0x3 << (pin*2));
	gpio_regs->ospeedr |= (ospeed << (pin*2));
	/* pull-up */
	gpio_regs->pupdr &= ~(0x3 << (pin*2));
	gpio_regs->pupdr |= (pupd << (pin*2));
}

static __init void init_pinmux(void)
{
	/* configure pin C6 as USART6 TX */
	/* alternate function 8 (USART6) */
	gpio_pin_cfg(&GPIO[2], 6, 8, 2, 0, 2, 1);

	/* configure pin C7 as USART6 RX */
	/* alternate function 8 (USART6) */
	gpio_pin_cfg(&GPIO[2], 7, 8, 2, 0, 2, 1);
	
	/* GPIO A pin 0 as GPIO input, connected to EXTI4 (default), IRQ 6 */
    gpio_pin_cfg(&GPIO[0], 0, 0, 0, 0, 1, 2);
 	//(*(volatile uint32_t *)0x4001) |= 0x8001;

    //(*(volatile uint32_t *)0x40013808) = 0x0310;
    //(*(volatile uint32_t *)0x4001380c) = 0x0010; 
	
	
	/*Wheelsensor pins*/
	/* GPIO A pins 3, GPIO B pins 1, 5, GPIO D pins 2 */
	/**************************************************************/
	gpio_pin_cfg(&GPIO[0], 3, 0, 0, 0, 1, 2);
	gpio_pin_cfg(&GPIO[1], 1, 0, 0, 0, 1, 2);	
	gpio_pin_cfg(&GPIO[1], 5, 0, 0, 0, 1, 2);
	gpio_pin_cfg(&GPIO[3], 2, 0, 0, 0, 1, 2);
	/**************************************************************/

	/*Powertrain pins*/
	/* GPIO B pins 3, 14 */
	/**************************************************************/
	gpio_pin_cfg(&GPIO[1], 3, 0x01, 2, 0, 2, 1);
	gpio_pin_cfg(&GPIO[1], 14, 0x09, 2, 0, 2, 1);	

	/**************************************************************/
	
	/*Steering pins*/
	/* GPIO B pins 4, 15 */
	/**************************************************************/
	gpio_pin_cfg(&GPIO[1], 4, 0x02, 2, 0, 2, 1);
	gpio_pin_cfg(&GPIO[1], 15, 0x09, 2, 0, 2, 1);	

	/**************************************************************/	

	/*Ultrasonic pins*/
	/* GPIO A pins 8 */
	/**************************************************************/
	gpio_pin_cfg(&GPIO[0], 8, 0x01, 2, 0, 0, 1);
    (*(volatile uint32_t *)0x40013808) = 0x0310;
    (*(volatile uint32_t *)0x4001380c) = 0x0010; 
	/**************************************************************/		

	/*Distancesensor pins*/
	/* GPIO A pins 15 */
	/**************************************************************/
	gpio_pin_cfg(&GPIO[0], 4, 0, 0, 0, 1, 2);


	/**************************************************************/	
	
	/*Lighting pins*/
	/* GPIO C pins 8, 9, 11, 13*/
	/**************************************************************/
	gpio_pin_cfg(&GPIO[0], 8, 0, 0, 1, 3, 0);
	gpio_pin_cfg(&GPIO[0], 8, 0, 0, 1, 3, 0);
	gpio_pin_cfg(&GPIO[0], 8, 0, 0, 1, 3, 0);
	gpio_pin_cfg(&GPIO[0], 8, 0, 0, 1, 3, 0);
	
	/**************************************************************/	
	
	
	/* Ethernet pins */
	/* GPIO A pins 1, 2, 7: RMII Ref Clk, MDIO, RMOO_CRS_DV */
	gpio_pin_cfg(&GPIO[0], 1, 0xb, 2, 0, 3, 0);
	gpio_pin_cfg(&GPIO[0], 2, 0xb, 2, 0, 3, 0);
	gpio_pin_cfg(&GPIO[0], 7, 0xb, 2, 0, 3, 0);

	/* GPIO B pins 12..15: MII_RX_ER, RMII_TX_EN, RMII_TXD0, RMII_TXD1 */
	gpio_pin_cfg(&GPIO[1], 10, 0xb, 2, 0, 3, 0);
	gpio_pin_cfg(&GPIO[1], 11, 0xb, 2, 0, 3, 0);
	gpio_pin_cfg(&GPIO[1], 12, 0xb, 2, 0, 3, 0);
	gpio_pin_cfg(&GPIO[1], 13, 0xb, 2, 0, 3, 0);

	
	/* GPIO C pins 1, 4, 5: MDC, RMII_RXD0, RMII_RXD1 */
	gpio_pin_cfg(&GPIO[2], 1, 0xb, 2, 0, 3, 0);
	gpio_pin_cfg(&GPIO[2], 4, 0xb, 2, 0, 3, 0);
	gpio_pin_cfg(&GPIO[2], 5, 0xb, 2, 0, 3, 0);
	
	/*GPIO for SPI pins Configuration: */
	gpio_pin_cfg(&GPIO[0], 5, 0x05, 2, 0, 0, 2);
	gpio_pin_cfg(&GPIO[0], 7, 0x05, 2, 0, 0, 2);	
	gpio_pin_cfg(&GPIO[0], 6, 0x05, 2, 0, 0, 2);
	gpio_pin_cfg(&GPIO[4], 3, 0x05, 1, 0, 0, 1);
	gpio_pin_cfg(&GPIO[4], 0, 0x05, 0, 0, 0, 0);	
	gpio_pin_cfg(&GPIO[4], 1, 0x05, 1, 0, 0, 0);
	
	/*GPIO D: GPIO LED output */
	gpio_pin_cfg(&GPIO[4], 2, 0, 1, 0, 3, 0);
	gpio_pin_cfg(&OUTPORT, 12, 0, 1, 0, 3, 0);	
	gpio_pin_cfg(&OUTPORT, 13, 0, 1, 0, 3, 0);
	gpio_pin_cfg(&OUTPORT, 14, 0, 1, 0, 3, 0);
	gpio_pin_cfg(&OUTPORT, 15, 0, 1, 0, 3, 0);	

	/* GPIO E pin 2: PHY reset */
	gpio_pin_cfg(&GPIO[4], 2, 0, 1, 0, 2, 1);

	/* trigger PHY reset */
	/*GPIO[4].br |= (1 << (2*1));
	for (int poll = 0; poll < 20000; poll++) {
		barrier();
	}
	GPIO[4].bs |= (1 << (2*1));*/
}

/** kernel entry function */
void __init board_init(void)
{
	/* set vector table to flash start */
	VTOR = BOARD_ROM_PHYS;

	/* setup caches, clocks, etc */
	init_clocks();
	init_pinmux();
	gpio_init();

	serial_init(115200);
	printf("Starting up ...\n");

	printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
	printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);

	nvic_irq_init();
	nvic_timer_init(100);
	/* enter the kernel */
	/* NOTE: all processors take the same entry point! */
	kernel_main(0);
}
