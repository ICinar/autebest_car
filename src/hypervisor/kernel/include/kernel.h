/*
 * kernel.h
 *
 * Kernel common functions.
 *
 * azuepke, 2013-11-24: initial MPU version
 * azuepke, 2015-06-23: added full API documentation
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <hv_compiler.h>
#include <stddef.h>
#include <stdarg.h>
#include <arch_regs.h>
#include <arch.h>
#include <hv_types.h>

/* forward declaration */
struct counter_cfg;


/* buildid.c */
/** Kernel build ID
 *
 * This NUL-terminated string represents the build ID of the kernel.
 */
extern const char kernel_buildid[];

/* main.c */
/** Kernel entry point
 *
 * This function represents the kernel entry point for all processor cores.
 * The board layer calls this function at the end of the board initialization
 * while executing on the boot stack.
 *
 * \param [in] hm_restart	Boolean value to indicate a restart by health monitoring
 *
 * \note This function does not return to the caller.
 */
void kernel_main(int hm_restart) __noreturn __tc_fastcall;

/* printf.c */
/** Simple vprintf()
 *
 * This function is a simple vprintf()-implementation printing
 * on the system console. See printf() for the format string.
 *
 * \param [in] format		Format string
 * \param [in] args			Format arguments
 *
 * \see _printf()
 * \see board_putc()
 */
void _vprintf(const char* format, va_list args);

/** Simple printf()
 *
 * This function is a simple printf()-implementation printing
 * on the system console.
 *
 * Field width:
 * - \c 0   zero padding (ignored)
 * - \c nn  decimal field width
 *
 * Length modifiers:
 * - \c l   long
 * - \c ll  long long
 * - \c z   size_t or uintptr_t, native register width
 *
 * Conversion specifiers:
 * - \c c    char
 * - \c s    string
 * - \c p    pointer (implicit 'z')
 * - \c x    hex
 * - \c d    signed decimal
 * - \c u    unsigned decimal
 *
 * Tip:
 * - use \c %zx to print register values
 *
 * Note:
 * - for hex numbers, a given field width truncates the number
 * - for decimals, a field width aligns to the right
 *
 * \param [in] format		Format string
 *
 * \see _vprintf()
 * \see board_putc()
 */
void _printf(const char *format, ...) __printflike(1, 2);

/** Simple vprintf()
 *
 * Wrapper to _vprintf().
 *
 * \param [in] format		Format string
 * \param [in] args			Format arguments
 *
 * \see _vprintf()
 */
#define vprintf(format, args)	_vprintf((format), (args))

/** Simple printf()
 *
 * Wrapper to _printf().
 *
 * \param [in] format		Format string
 *
 * \see _printf()
 */
#define printf(format...)	_printf(format)

/* verbose printing */
#ifdef VERBOSE
#define Vprintf(x...)	printf(x)
#else
#define Vprintf(x...)	do { } while (0)
#endif

/* super verbose printing */
#ifdef VERBOSE2
#define VVprintf(x...)	printf(x)
#else
#define VVprintf(x...)	do { } while (0)
#endif


/* task.c */
/** Notification to wake ISR task
 *
 * This function activates the ISR task identified by \a arg0.
 * The ISR task must be in SUSPENDED state before.
 * The board layer calls this function during interrupt dispatching.
 * This function also calls board_irq_disable() to mask the interrupt source.
 * The interrupt source is unmasked again on task termination.
 *
 * \param [in] arg0			Argument of type struct task
 *
 * \see board_irq_dispatch()
 * \see board_irq_disable()
 * \see board_irq_enable()
 */
void kernel_wake_isr_task(const void *arg0);

/* counter.c */
/** Notification in hardware counter increment
 *
 * This function signals the kernel that a hardware counter was incremented
 * by \a increment tick units since the last counter query, i.e., the kernel
 * called the counter_cfg::query() callback.
 * This function is called from IRQ context by the board layer or the kernel.
 *
 * \param [in] ctr_cfg		Counter configuration
 * \param [in] increment	Increment in counter ticks
 *
 * \see board_irq_dispatch()
 */
void kernel_increment_counter(const struct counter_cfg *ctr_cfg, ctrtick_t increment);

/* sched.c */
/** Notification of timer interrupt
 *
 * This function notifies the kernel on a timer interrupt.
 * The current system time in nanoseconds is passed as argument in \a now.
 * This function is called by the board layer during a timer interrupt.
 *
 * \param [in] now			Current system time
 *
 * \see board_irq_dispatch()
 */
void kernel_timer(time_t now);

#ifdef SMP
/* ipi.c */
/** Notification of IPI
 *
 * A call to this function notifies the kernel of an IPI send by another
 * processor core \a source_cpu to the current processor core \a target_cpu.
 *
 * \param [in] target_cpu	Target processor ID, i.e., the current processor ID
 * \param [in] source_cpu	Source processor ID
 *
 * \see board_ipi_broadcast()
 * \see board_irq_dispatch()
 */
void kernel_ipi_handle(unsigned int target_cpu, unsigned int source_cpu);
#endif

/** Notification of IPI
 *
 * A call to this function checks if the data object specified by
 * its user space address \a user_addr and its size \a size is valid
 * and within the currently active partition's bounds.
 *
 * \param [in] user_addr	Address of a data object in user space
 * \param [in] size			Size of data object
 *
 * \retval E_OK				Success, address valid
 * \retval E_OS_ILLEGAL_ADDRESS	Address invalid
 *
 * \note This function does not check the required alignment.
 */
unsigned int kernel_check_user_addr(
	void *user_addr,
	size_t size);


/** Timeout value for infinity
 *
 * A special timeout expiry value defining infinity or an infinite timeout.
 *
 * \see time_t
 */
#define INFINITY	0xffffffffffffffffULL

/** Indicator for relative timeouts after partition starts
 *
 * A modifier for relative timeouts to indicate a timeout value relative
 * to the next partition release point.
 *
 * \see timeout_t
 */
#define FAR_FUTURE	0x8000000000000000ULL

/** Accessors to the current task's register frame */
#define SET_RET(val)	arch_reg_frame_set_return(arch_get_sched_state()->regs, (val))
#define SET_RET64(val)	arch_reg_frame_set_return64(arch_get_sched_state()->regs, (val))
#define SET_OUT1(val)	arch_reg_frame_set_out1(arch_get_sched_state()->regs, (val))
#define SET_OUT2(val)	arch_reg_frame_set_out2(arch_get_sched_state()->regs, (val))
#define SET_OUT3(val)	arch_reg_frame_set_out3(arch_get_sched_state()->regs, (val))

#define SAVE_ARG0(val)	arch_reg_frame_save_arg0(arch_get_sched_state()->regs, (val))
#define SAVE_ARG1(val)	arch_reg_frame_save_arg1(arch_get_sched_state()->regs, (val))

#define SET_ARG0(val)	arch_reg_frame_set_arg0(arch_get_sched_state()->regs, (val))
#define SET_ARG1(val)	arch_reg_frame_set_arg1(arch_get_sched_state()->regs, (val))
#define SET_ARG2(val)	arch_reg_frame_set_arg2(arch_get_sched_state()->regs, (val))

#endif
