/*
 * stm_timer.c
 *
 * Tricore STM timer.
 *
 * azuepke, 2014-12-31: initial
 */

/*
 * This driver covers two versions of the STM system timer:
 * the old 56-bit counter used in TC1796, TC1797, and TC1798, and
 * the new 64-bit counter used in TC27x and newer.
 *
 * As TSIM implements the old version, we implement both variants.
 * The preprocessor define AURIX_TIMER switches modes.
 *
 * Bits only available for the old timer are tagged (old), and
 * flags for the new one are tagged with (new).
 *
 * For the old timer, the chip also includes the interrupt request registers
 * SRC0 and SRC1 in 16-bit format.
 *
 * For the 64-bit counter, the chip exposes a second view on the timer value
 * and offers reset and fine granular access control.
 *
 * We use the lower 32-bit of the ever increasing 56/64-bit timer
 * with a comparator value of also 32-bit.
 * Further, we use only comparator #0 and interrupt #0 of the timer.
 */

#include <kernel.h>
#include <assert.h>
#include <tc_io.h>
#include <board.h>
#include <board_stuff.h>


/** Register offsets in STM. */
#define STM_CLC				0x00	/* clock control */
#define STM_ID				0x08	/* identification (read only) */

#define STM_TIM0			0x10	/* timer 0 */
#define STM_TIM1			0x14	/* timer 1 */
#define STM_TIM2			0x18	/* timer 2 */
#define STM_TIM3			0x1c	/* timer 3 */
#define STM_TIM4			0x20	/* timer 4 */
#define STM_TIM5			0x24	/* timer 5 */
#define STM_TIM6			0x28	/* timer 6 */
#define STM_CAP				0x2c	/* timer capture */

#define STM_CMP0			0x30	/* compare 0 */
#define STM_CMP1			0x34	/* compare 1 */
#define STM_CMCON			0x38	/* compare match control */
									/* bits  4.. 0: MSIZE0   */
									/* bits 12.. 8: MSTART0  */
									/* bits 20..16: MSIZE1   */
									/* bits 28..24: MSTART1  */
#define STM_ICR				0x3c	/* interrupt control register */
#define STM_ISCR			0x40	/* interrupt set/clear register */

/* TC1796, TC1797, and TC1798 interrupt sources */
#define STM_SRC1			0xf8	/* interrupt 1 (16-bit format) */
#define STM_SRC0			0xfc	/* interrupt 0 (16-bit format) */

/* TC27x and newer: second view, debug and reset control */
#define STM_TIM0SV			0x50	/* timer 0 second view (new only) */
#define STM_CAPSV			0x54	/* timer capture second view (new only) */
#define STM_OCS				0xe8	/* OCDS control and status (new only) */
#define STM_KRSTCLR			0xec	/* reset status clear */
#define STM_KRST1			0xf0	/* reset control register 1 */
#define STM_KRST0			0xf4	/* reset control register 0 */
#define STM_ACCEN1			0xf8	/* access enable register 1 */
#define STM_ACCEN0			0xfc	/* access enable register 0 */


/* Register accessors (accessing STM0..2, depending on the core) */
static inline uint32_t stm_read(unsigned int core_id, unsigned int reg)
{
	unsigned long cpu_offset = core_id * STM_TIMER_COREOFFSET;
	return readl((volatile void *)(STM_TIMER_BASE + cpu_offset + reg));
}

static inline void stm_write(unsigned int core_id, unsigned int reg, uint32_t val)
{
	unsigned long cpu_offset = core_id * STM_TIMER_COREOFFSET;
	writel((volatile void *)(STM_TIMER_BASE + cpu_offset + reg), val);
}

/* Register accessors to current core */
static inline uint32_t stm_read_current_core(unsigned int reg)
{
	return stm_read(arch_cpu_id(), reg);
}

static inline void stm_write_current_core(unsigned int reg, uint32_t val)
{
	stm_write(arch_cpu_id(), reg, val);
}

/** Bits in CLC. */
#define CLC_DISR			0x01	/* module disable request */
#define CLC_DISS			0x02	/* module disable status */
#define CLC_SPEN			0x04	/* OCDS enable suspend mode (old) */
#define CLC_EDIS			0x08	/* sleep mode enable */
#define CLC_SBWE			0x10	/* OCDS bits write enable (old) */
#define CLC_FSOE			0x20	/* OCDS fast switch off enable (old) */
#define CLC_RMC(x)			((x)<<8)	/* clock divider in run mode, divide clock by 1..7 (old) */

/** Bits in ICR. */
#define ICR_CMP0EN			0x01	/* CMP0 interrupt enable */
#define ICR_CMP0IR			0x02	/* CMP0 interrupt request */
#define ICR_CMP0OS			0x04	/* CMP0 output selection: STMIR0 or 1 */
#define ICR_CMP1EN			0x10	/* CMP1 interrupt enable */
#define ICR_CMP1IR			0x20	/* CMP1 interrupt request */
#define ICR_CMP1OS			0x40	/* CMP1 output selection: STMIR0 or 1 */

/** Bits in ISCR. */
#define ISCR_CMP0IRR		0x01	/* reset CMP0 interrupt flag */
#define ISCR_CMP0IRS		0x02	/* set   CMP0 interrupt flag */
#define ISCR_CMP1IRR		0x04	/* reset CMP1 interrupt flag */
#define ISCR_CMP1IRS		0x08	/* set   CMP1 interrupt flag */

/** Bits in OCS (TC27x and newer). */
#define OCS_SUS_RUN			(0<<24)	/* Debugging: continue running */
#define OCS_SUS_STOP		(2<<24)	/* Debugging: stop counter */
#define OCS_SUS_P			(1<<28)	/* Debugging suspend write protection */
#define OCS_SUSSTA			(1<<29)	/* Status: module is suspended */



/** time in nanoseconds on last interrupt */
static time_t time_last_tick_ns;
/** ticker time in nanoseconds */
static unsigned int clock_ns;
/** timer resolution in nanoseconds */
unsigned int board_timer_resolution;
/** next expiry and reload delta */
uint32_t next_expiry, reload;

/** get current time in nanoseconds */
time_t board_get_time(void)
{
	/* FIXME: implement sub-nanosecond timer resolution!
	 *   lo = STM_TIM0
	 *   hi = STM_CAP
	 *   return hi << 32 | lo
	 */
	return time_last_tick_ns;
}

/** interrupt handler */
void stm_timer_handler(unsigned int irq __unused)
{
	/* clear interrupt flag */
	stm_write_current_core(STM_ISCR, ISCR_CMP0IRR);

	/* set next expiry */
#if 1
	// FIXME: ALEX: program timer relative to now, or we miss interrupts on TSIM!
	next_expiry = stm_read_current_core(STM_TIM0);
#endif
	next_expiry += reload;
	stm_write_current_core(STM_CMP0, next_expiry);

	time_last_tick_ns += clock_ns;

	/* notify kernel on timer interrupt */
	kernel_timer(time_last_tick_ns);
}

/** initialize timer, per-core setup */
__init void stm_timer_init_core(unsigned int core_id)
{
	uint32_t val;

	/* let debugger halt the timer */
#ifdef AURIX_TIMER
	val = 0;
	stm_write(core_id,  STM_CLC, val);
	val = OCS_SUS_P | OCS_SUS_STOP;
	stm_write(core_id,  STM_OCS, val);
#else
	val = CLC_RMC(1) | CLC_SBWE | CLC_SPEN | CLC_FSOE;
	stm_write(core_id,  STM_CLC, val);
#endif

	/* setup comparator #0 */
	val = 31;	/* MSIZE0 = 31, MSTART = 0 */
	stm_write(core_id,  STM_CMCON, val);

	/* set initial expiry */
	next_expiry = stm_read(core_id, STM_TIM0);
	next_expiry += reload;
	stm_write(core_id, STM_CMP0, next_expiry);

	/* reset CMP0 interrupt flag */
	stm_write(core_id, STM_ISCR, ISCR_CMP0IRR);

	/* enable CMP0 interrupt as STMIR0 */
	stm_write(core_id, STM_ICR, ICR_CMP0EN);
}

/** initialize timer, common setup */
__init void stm_timer_init(unsigned int freq)
{
	reload = STM_TIMER_CLOCK / freq;
	clock_ns = 1000000000 / freq;
	board_timer_resolution = clock_ns;

	/* FIXME: this is specific to core #0! */
	stm_timer_init_core(0);

	/* unmask timer interrupt */
	board_irq_enable(STM_TIMER_IRQ);
}
