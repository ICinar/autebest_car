/*
 * exception.c
 *
 * Architecture specific exception handling
 *
 * azuepke, 2014-10-25: initial
 */

#include <kernel.h>
#include <assert.h>
#include <task.h>
#include <board.h>
#include <tc_layout.h>
#include <tc_private.h>
#include <sched.h>
#include <hm.h>


#ifndef NDEBUG
static __cold void arch_dump_lower_upper(
	unsigned int vector,
	struct arch_ctxt_frame *l,
	struct arch_ctxt_frame *u,
	unsigned int pc)
{
	register unsigned int ___a0 __asm__ ("a0");
	register unsigned int ___a1 __asm__ ("a1");
	register unsigned int ___a8 __asm__ ("a8");
	register unsigned int ___a9 __asm__ ("a9");
	unsigned int class;
	unsigned int tin;
	const char *s;

	assert(u != NULL);

	class = vector >> 16;
	tin = vector & 0xffff;

	s = NULL;
	switch (class) {
	case TRAP_PROT:
		s = "protection error";
		break;
	case TRAP_INST:
		s = "instruction error";
		break;
	case TRAP_CTXT:
		switch (tin) {
		case TIN_FCD:
			s = "FCD: need more contexts!";
			break;
		case TIN_CDO:
			s = "CDO";
			break;
		case TIN_CDU:
			s = "CDU: call depth unterflow";
			break;
		case TIN_FCU:
			s = "FCU: free chain underflow";
			break;
		case TIN_CSU:
			s = "CSU";
			break;
		case TIN_CTYP:
			s = "CTYP: wrong context type";
			break;
		case TIN_NEST:
			s = "NEST";
			break;
		default:
			s = "unknown context error";
			break;
		}
		break;
	case TRAP_BUS:
		s = "bus error";
		break;
	}

	printf("trap class %d, TIN %d, %s\n", class, tin, s != NULL ? s : "");

	if (l != NULL) {
		printf("  d0 =%x  d1 =%x  d2 =%x  d3 =%x\n"
		       "  d4 =%x  d5 =%x  d6 =%x  d7 =%x\n",
		       l->cx.l.d0, l->cx.l.d1, l->cx.l.d2, l->cx.l.d3,
		       l->cx.l.d4, l->cx.l.d5, l->cx.l.d6, l->cx.l.d7);
	}
	printf("  d8 =%x  d9 =%x  d10=%x  d11=%x\n"
	       "  d12=%x  d13=%x  d14=%x  d15=%x\n",
	       u->cx.u.d8, u->cx.u.d9, u->cx.u.d10, u->cx.u.d11,
	       u->cx.u.d12, u->cx.u.d13, u->cx.u.d14, u->cx.u.d15);

	if (l != NULL) {
		printf("  a0 =%x  a1 =%x  a2 =%x  a3 =%x\n"
		       "  a4 =%x  a5 =%x  a6 =%x  a7 =%x\n",
		       ___a0, ___a1, l->cx.l.a2, l->cx.l.a3,
		       l->cx.l.a4, l->cx.l.a5, l->cx.l.a6, l->cx.l.a7);
	}
	printf("  a8 =%x  a9 =%x  a10=%x  a11=%x\n"
	       "  a12=%x  a13=%x  a14=%x  a15=%x\n",
	       ___a8, ___a9, u->cx.u.a10, u->cx.u.a11,
	       u->cx.u.a12, u->cx.u.a13, u->cx.u.a14, u->cx.u.a15);

	printf("  PC =%x  SP =%x  PSW=%x PCXI=%x\n",
	       (l != NULL) ? l->cx.l.pc : pc, u->cx.u.a11, u->cx.u.psw, u->pcxi);
}

__cold void arch_dump_registers(
	struct arch_reg_frame *regs,
	unsigned long vector,
	unsigned long fault,
	unsigned long higher_cx)
{
	struct arch_ctxt_frame *u, *l;

	if (regs != NULL) {
		l = (void*)regs;
		u = CX_TO_PTR(higher_cx);
		arch_dump_lower_upper(vector, l, u, 0);
	} else {
		arch_dump_lower_upper(vector, NULL, NULL, fault);
	}
}
#endif

__tc_fastcall __cold void tc_handler_nmi(void *lower, unsigned long higher_cx)
{
	/* NOTE: no dispatching of the NMI, NMIs are only used for watchdogs */
	hm_exception(lower, 1, HM_ERROR_NMI, (TRAP_NMI<<16)|0, 0, higher_cx);

	board_nmi_dispatch(0);
}

static unsigned int tc_hm_error_id(unsigned long class, unsigned long tin)
{
	switch (class) {
	case TRAP_PROT:
		switch (tin) {
		case TIN_PRIV:
			return HM_ERROR_PRIVILEGED_INSTRUCTION;

		case TIN_MPR:
			return HM_ERROR_MPU_ERROR_READ;

		case TIN_MPW:
			return HM_ERROR_MPU_ERROR_WRITE;

		case TIN_MPX:
			return HM_ERROR_MPU_ERROR_CODE;

		case TIN_MPP:
			return HM_ERROR_MPU_ERROR_DEVICE;

		case TIN_MPN:	/* NULL pointer access. NOTE: does not fit better */
			return HM_ERROR_MPU_ERROR_WRITE;

		default:
			assert(tin == TIN_GRWP);
			return HM_ERROR_PRIVILEGED_INSTRUCTION;
		}
		break;

	case TRAP_INST:
		switch (tin) {
		case TIN_IOPC:
		case TIN_UOPC:
		case TIN_OPD:
			return HM_ERROR_ILLEGAL_INSTRUCTION;

		default:
			assert(tin == TIN_ALN || tin == TIN_MEM);
			return HM_ERROR_UNALIGNED_DATA;
		}
		break;

	case TRAP_CTXT:
		switch (tin) {
		case TIN_FCD:
		case TIN_CDO:
			return HM_ERROR_CONTEXT_OVERFLOW;

		case TIN_CDU:
		case TIN_FCU:
		case TIN_CSU:
			return HM_ERROR_CONTEXT_UNDERFLOW;

		default:
			return HM_ERROR_CONTEXT_ERROR;
		}
		break;

	case TRAP_BUS:
		switch (tin) {
		case TIN_PSE:
			return HM_ERROR_CODE_MEMORY_ERROR;

		case TIN_DSE:
			return HM_ERROR_DATA_MEMORY_ERROR;

		case TIN_DAE:
			return HM_ERROR_ASYNC_BUS_ERROR;

		case TIN_PIE:
			return HM_ERROR_CODE_MEMORY_ERROR;

		case TIN_DIE:
			return HM_ERROR_DATA_MEMORY_ERROR;

		default:
			return HM_ERROR_HARDWARE_ERROR;
		}
		break;

	default:
		assert(class == TRAP_ASSERT);	/* TRAP_SYS and TRAP_NMI not raised */
		return HM_ERROR_ARITHMETIC_OVERFLOW;
	}
}


__tc_fastcall __cold void tc_handler_trap_kern(void *lower, unsigned long higher_cx, unsigned long class, unsigned long tin)
{
	unsigned int hm_error_id;

	hm_error_id = tc_hm_error_id(class, tin);
	hm_exception(lower, 1, hm_error_id, (class<<16)|tin, 0, higher_cx);
}

__tc_fastcall void tc_handler_trap_user(void *lower, unsigned long higher_cx, unsigned long class, unsigned long tin)
{
	unsigned int hm_error_id;
	int fatal;

	hm_error_id = tc_hm_error_id(class, tin);
	fatal = 0;

	/* NOTE: since the architecture lacks a fault-address register, we cannot
	 * detect HM_ERROR_STACK_OVERFLOW properly.
	 */

	if (class == TRAP_CTXT) {
		if ((tin == TIN_CTYP) || (tin == TIN_NEST)) {
			fatal = 1;
		}
	} else if (class == TRAP_BUS) {
		if ((tin == TIN_DAE) || (tin == TIN_CAE) ||
		    (tin == TIN_PIE) || (tin == TIN_DIE)) {
			fatal = 1;
		}
	}

	hm_exception(lower, fatal, hm_error_id, (class<<16)|tin, 0, higher_cx);
}

__tc_fastcall __cold void tc_handler_fcu(unsigned long higher_cx, unsigned long pc)
{
	hm_exception(NULL, 1, HM_ERROR_CONTEXT_ERROR, (TRAP_CTXT<<16)|TIN_FCU, pc, higher_cx);
}

__init void arch_init_exceptions(void)
{
	unsigned int syscon;
	uint64_t *s, *d;
	unsigned int i;

	/* copy exception vectors to CRAM */
	/* NOTE: on multicore, CRAM refers to the core program RAM at core local
	 *       addresses, so that each core sets up its own exception handlers.
	 */
	assert((addr_t)tc_trap_table_start + CRAM_VECTOR_SIZE
	    == (addr_t)tc_trap_table_end);
	s = (uint64_t *)tc_trap_table_start;
	d = (uint64_t *)CRAM_VECTOR_BASE;
	for (i = 0; i < CRAM_VECTOR_SIZE/sizeof(*s); i++) {
		*d++ = *s++;
	}

	MTCR(CSFR_BIV, CRAM_VECTOR_BASE | CRAM_VECTOR_IRQ_OFFSET | BIV_VSS);
	MTCR(CSFR_BTV, CRAM_VECTOR_BASE);
	MTCR(CSFR_ISP, 0);
	ISYNC();
#ifndef NDEBUG
	/* debug check to test if BIV/BTV were really updated */
	{
		unsigned int biv = MFCR(CSFR_BIV);
		assert(biv == (CRAM_VECTOR_BASE | CRAM_VECTOR_IRQ_OFFSET | BIV_VSS));
		unsigned int btv = MFCR(CSFR_BTV);
		assert(btv == CRAM_VECTOR_BASE);
	}
#endif

	syscon = MFCR(CSFR_SYSCON);
	syscon |= SYSCON_U1_IOS|SYSCON_U1_IED;
	syscon |= SYSCON_TS|SYSCON_IS;
	//syscon |= SYSCON_TPROTEN;
	MTCR_ISYNC(CSFR_SYSCON, syscon);

	/* NOTE: the MPU is enabled in board_mpu_init() in the board layer */

	/* enable CPU cycle counter (multi counters 1..3 not set) */
	MTCR_ISYNC(CSFR_CCTRL, 0x02);

	/* ENDINIT protection is enabled by board_cpu0_up() or
	 * board_startup_complete() later
	 */
}

/** switch over to the kernel stack and invoke next() */
__init void arch_switch_to_kernel_stack(void (*next)(void *))
{
	struct sched_state *sched = current_sched_state();
	unsigned long pcxi, fcx, lcx, psw;

	/* we call next(), and later return to a to-be prepared UPPER in entry #1 */
	pcxi = 0;	/* later PTR_TO_CX(&regs->csa[1]); */

	fcx = sched->arch.kern_fcx;
	lcx = sched->arch.kern_lcx;

	/* also reset call depth counter in PSW */
	psw = PSW_IDLE_BITS;

	tc_switch_to_kernel_stack(sched->arch.kern_stack, next, pcxi, fcx, lcx, psw);
	/* DOES NOT RETURN */
}
