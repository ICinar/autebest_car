/*
 * board.h
 *
 * Board abstraction layer.
 *
 * azuepke, 2013-03-23: initial
 * azuepke, 2013-11-24: rework for MPU kernel
 * azuepke, 2015-06-22: added full API documentation
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdint.h>
#include <hv_compiler.h>
#include <hv_types.h>
#include <arch_regs.h>

/** Idle board
 *
 * This function represents the core of the kernel's idle tasks.
 * It is executed in a task context, but running in kernel space.
 * It loops forever, probably halting the processor core
 * until the next interrupt arrives.
 */
/** idle the board */
void board_idle(void) __noreturn;

/** Reset or halt board
 *
 * A call to this function resets or halts the system according
 * to the halt mode \a mode. The function never returns.
 *
 * \param [in] mode			Halt mode
 */
__tc_fastcall void board_halt(haltmode_t mode) __noreturn;

/** Halt current CPU (internal routine)
 *
 * A call to this function halts the current processor core.
 * The function never returns.
 */
__tc_fastcall void __board_halt(void) __noreturn;

/** Get current system time
 *
 * A call to this function returns the current system time in nanoseconds.
 * Time is assumed to start at 0 when the system boots.
 *
 * \returns Current time in nanoseconds since boot
 */
time_t board_get_time(void);

/** Timer resolution in nanoseconds
 *
 * The board exports the period between two timer interrupts in nanoseconds.
 */
extern unsigned int board_timer_resolution;

/** Print character to system console
 *
 * A successful call to this function prints a character to the system console.
 * The system console is acting in polling mode. If the serial FIFO is full,
 * the function returns an error.
 *
 * \param [in] c			Character to print
 *
 * \retval E_OK				Success
 * \retval E_OS_NOFUNC		Serial FIFO is busy, try again later
 */
unsigned int board_putc(int c);

/** Initialize MPU
 *
 * A call to this function initializes the kernel related part of the MPU.
 * The kernel calls this function on each processor core after initializing
 * exception handling, but before entering the idle task.
 */
void board_mpu_init(void);

#ifdef SMP
/** Start all secondary processor cores
 *
 * A call to this function starts all secondary cores in a system.
 * The kernel calls this function in the context of its idle task.
 * This function should return when all other processor cores have called
 * board_secondary_cpu_up() in the context of their idle tasks.
 *
 * \see board_secondary_cpu_up()
 */
void board_start_secondary_cpus(void);

/** Notification when secondary processor core started
 *
 * A call to this function signals that a secondary processor has successfully
 * booted into its idle task.
 *
 * \see board_start_secondary_cpus()
 * \see board_cpu0_up()
 * \see board_startup_complete()
 */
void board_secondary_cpu_up(unsigned int cpu);

/** Broadcast IPIs to processor cores
 *
 * A call to this function broadcast an IPI to the processors given in the
 * mask \a cpu_mask, which excludes the caller's processor.
 * On reception of an IPI, the board should call kernel_ipi_handle().
 *
 * \param [in] cpu_mask		Mask of processors
 *
 * \see kernel_ipi_handle()
 */
void board_ipi_broadcast(unsigned long cpu_mask);
#endif

/** Notification when first processor core started
 *
 * A call to this function signals that a first processor has successfully
 * booted into its idle task.
 *
 * \see board_start_secondary_cpus()
 * \see board_startup_complete()
 */
void board_cpu0_up(void);

/** Signal successful startup of the system
 *
 * A call to this function signals that the system is now ready for scheduling.
 * The kernel calls it on the first processor core after initialization
 * is complete and all processors were started.
 *
 * \see board_cpu0_up()
 * \see board_secondary_cpu_up()
 */
void board_startup_complete(void);


/** Notification on time partition switch
 *
 * A call to this function signals that the system switches from time partition
 * \a prev_timepart to \a next_timepart on the current processor core.
 * The flags of the next time partition window are given in \a tpwindow_flags.
 *
 * \param [in] prev_timepart	Time partition ID
 * \param [in] next_timepart	Time partition ID
 * \param [in] tpwindow_flags	Time partition window flags
 */

void board_tp_switch(unsigned int prev_timepart, unsigned int next_timepart, unsigned int tpwindow_flags);

/** Notification on exceptions
 *
 * The kernel forwards all exceptions that happen in the system to the board
 * layer. The current register frame is passed in \a regs. A non-zero value
 * for \a fatal encodes an exception in kernel context or a critical exception
 * in user context. \a hm_error_id encodes the HM error code, and \a vector,
 * \a fault_addr, and \a aux encode further information on the exception,
 * e.g. the exception vector, a fault address, or a fault status register.
 *
 * \param [in] regs			Register frame
 * \param [in] fatal		Fatal exception indicator
 * \param [in] hm_error_id	HM error code
 * \param [in] vector		Architecture specific exception vector
 * \param [in] fault_addr	Fault address
 * \param [in] aux			Architecture specific additional information
 *
 * \return A non-zero value indicates that the board layer handled the
 * exception. A zero value indicates that the exception is not handled.
 *
 * \note If the board layer is able to handle and recover an exception,
 * for example an otherwise fatal exception raised for hardware testing
 * purposes, the function returns a non-zero value to indicate success.
 * This skips further exception handling and the kernel continues execution.
 * If the board layer is not able to handle the exception, the function returns
 * zero, and normal exception handling continues in the kernel.
 */
int board_hm_exception(
	struct arch_reg_frame *regs,
	int fatal,
	unsigned int hm_error_id,
	unsigned long vector,
	unsigned long fault_addr,
	unsigned long aux);


/** Dispatch interrupt
 *
 * A call to this function requests interrupt dispatch for interrupt vector
 * \a vector on the current processor core. The kernel calls this function
 * from its interrupt handler. The board layer acknowledges the interrupt
 * and calls the registered ISR-handler.
 *
 * \note The kernel handler to activate ISR tasks kernel_wake_isr_task()
 * masks the interrupt source.
 *
 * \param [in] vector		Architecture specific interrupt vector
 *
 * \see board_nmi_dispatch()
 * \see kernel_wake_isr_task()
 * \see kernel_increment_counter()
 * \see kernel_timer()
 * \see kernel_ipi_handle()
 */
void board_irq_dispatch(unsigned int vector);

/** Dispatch NMI
 *
 * A call to this function requests dispatching of a non-maskable interrupt
 * for interrupt vector \a vector on the current processor core.
 * The kernel calls this function from its interrupt handler.
 * The board layer acknowledges and handles the NMI.
 *
 * \param [in] vector		Architecture specific interrupt vector
 *
 * \see board_irq_dispatch()
 */
void board_nmi_dispatch(unsigned int vector);

/** Disable interrupt source
 *
 * A call to this function signals that the ISR task associated with
 * interrupt \a irq_id was activated by the kernel.
 * The board layer masks the interrupt source until the ISR task terminates.
 *
 * \param [in] irq_id		Interrupt ID
 *
 * \see board_irq_enable()
 * \see kernel_wake_isr_task()
 */
void board_irq_disable(unsigned int irq_id);

/** Enable interrupt source
 *
 * A call to this function signals that the ISR task associated with
 * interrupt \a irq_id terminated.
 * The board layer unmasks the interrupt source again.
 * For interrupts handled in supervisor mode, the board layer calls
 * this function internally to enable an interrupt source initially.
 *
 * \param [in] irq_id		Interrupt ID
 *
 * \see board_irq_disable()
 */
void board_irq_enable(unsigned int irq_id);

/** Default handler for unhandled interrupts
 *
 * The board layer provides this function as default interrupt handler
 * for all unregistered interrupt sources.
 *
 * \param [in] irq_id		Interrupt ID
 *
 * \see board_irq_dispatch()
 */
void board_unhandled_irq_handler(unsigned int irq_id);

#endif
