/*
 * board.c
 *
 * Board initialization for Cortex-M3/M4
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


/* PLL setup sequence -- it's magic */

/* system control and status */
#define SCS				_REG32(0x400fc1a0)

/* clock source select */
#define CLKSRCSEL		_REG32(0x400fc10c)

/* main PLL */
#define PLL0CON			_REG32(0x400fc080)
#define PLL0CFG			_REG32(0x400fc084)
#define PLL0STAT		_REG32(0x400fc088)
#define PLL0FEED		_REG32(0x400fc08c)

/* USB PLL */
#define PLL1CON			_REG32(0x400fc0a0)
#define PLL1CFG			_REG32(0x400fc0a4)
#define PLL1STAT		_REG32(0x400fc0a8)
#define PLL1FEED		_REG32(0x400fc0ac)

/* clock dividers */
#define CCLKCFG			_REG32(0x400fc104)
#define USBCLKCFG		_REG32(0x400fc108)
#define PCLKSEL0		_REG32(0x400fc1a8)
#define PCLKSEL1		_REG32(0x400fc1ac)

/* power mode control, peripherals */
#define PCON			_REG32(0x400fc0c0)
#define PCONP			_REG32(0x400fc0c4)

/* clock output */
#define CLKOUTCFG		_REG32(0x400fc1c8)

/* flash wait states */
#define FLASHCFG		_REG32(0x400fc000)

static __init void init_clocks(void)
{
	/* booting from 4 MHz internal clock at first */

	/* power down all peripherals first */
	PCONP = 0;

	/* enable main oscillator and wait to stabilize (12 MHz external) */
	SCS = (1 << 5);
	while ((SCS & (1 << 6)) == 0) {
	}
	/* select main oscillator as PLL0 clock source */
	CLKSRCSEL = 0x1;

	/* PLL0 clocks at 288 MHz: div 3 for 96 MHz CPU clock */
	CCLKCFG = 3 - 1;

	/* PLL0 clock 288 MHz: fext*2*12/1 = 288 -> mult 12, div 1 */
	/* setup PLL0: seed, feed, enable, feed, wait until locked */
	PLL0CFG = ((1 - 1) << 16) | (12 - 1);

	PLL0FEED = 0xaa;
	PLL0FEED = 0x55;

	PLL0CON = 0x1;

	PLL0FEED = 0xaa;
	PLL0FEED = 0x55;

	while ((PLL0STAT & (1 << 26)) == 0) {
	}

	/* set 5 clocks flash access time, recommended for <100 MHz */
	FLASHCFG = ((5 - 1) << 12) | 0x03a;

	/* connect PLL0, feed again, and wait to conncet */
	PLL0CON = 0x3;

	PLL0FEED = 0xaa;
	PLL0FEED = 0x55;

	while ((PLL0STAT & (1 << 25)) == 0) {
	}

	/* PLL0 div 6 for 48 MHz USB clock */
	USBCLKCFG = 6 - 1;

	/* set peripheral clock PCLK divider to 1 (96 MHz CPU) */
	PCLKSEL0 = 0x55555555;
	PCLKSEL1 = 0x55555555;

	/* enable peripherals -- see chapter 4.8.9 "Power Control for Peripherals"
	 * on page 64 in UM10360 LPC176x/5x User manual, Rev. 3.1 -- 2 April 2014
	 */
	PCONP = (1 << 3);	/* UART0 */
}


/* pin function select, pin mode select, open drain mode control, I2C pin cfg */
#define PINSEL(x)		((&_REG32(0x4002c000))[x])
#define PINMODE(x)		((&_REG32(0x4002c040))[x])
#define PINMODE_OD(x)	((&_REG32(0x4002c068))[x])
#define I2CPADCFG		_REG32(0x4002c07c)

static __init void init_pinmux(void)
{
	/* PIN MUX: enable UART0 and UART3 */
	PINSEL(0) = 0x0000005a;
}


/** kernel entry function */
void __init board_init(void)
{
	/* set vector table to flash start */
	VTOR = BOARD_ROM_PHYS;

	/* setup caches, clocks, etc */
	init_clocks();
	init_pinmux();

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
