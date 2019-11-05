/*
 * start.S
 *
 * Assembler startup code for PPC e200 cores
 *
 * NOTE: check this code again when going to *real* hardware
 *
 * azuepke, 2013-11-22: initial PPC port
 * azuepke, 2014-05-13: added magic TLB dance to enable virtual mappings
 * azuepke, 2014-06-03: adapted to MPU
 */

#include <board_stuff.h>
#include <ppc_asm.h>
#include <ppc_spr.h>
#include <ppc_tlb.h>


	.global _start
	.global ppc_set_tlb

	.text
	.section .text.start, "ax"

	/* entry point from boot loader */
_start:
	/* keep load address in r30 */
	bl		1f
1:	mflr	r31
	addi	r31, r31, -4
	b		_real_start

_real_start:
	/* disable all interrupts */
	li		r0, 0
	mtmsr	r0


	/*
	 * copy .data
	 */
_copy_data:
	lwi		r3, __rom_data_start
	lwi		r4, __data_start
	lwi		r5, __data_end

1:	lwz		r0, 0(r3)
	addi	r3, r3, 4
	stw		r0, 0(r4)
	addi	r4, r4, 4
	cmplw	r4, r5
	blt-	1b


	/*
	 * clear .bss
	 */
_clear_bss:
	li		r0, 0
	lwi		r5, __bss_end

1:	stw		r0, 0(r4)
	addi	r4, r4, 4
	cmplw	r4, r5
	blt-	1b


_common_start:
	/* HID setup */

	/* MMU setup */
	li		r0, 0
	mtspr	SPR_PID0, r0
	isync


	/* clear complete TLB1 */
	/* unfortunately, we cannot use flash invalidation in MMUCSR0,
	 * as entries marked with IPROT are not cleared.
	 *
	 * Also, we assume that we're running from the first entry in TLB1,
	 * so we're careful not pulling the carpet under our feet.
	 */
	/* extract number of entries in TLB1 */
	mfspr	r3, SPR_TLB1CFG
	andi.	r3, r3, 0xfff
	addi	r3, r3, -1
	mtctr	r3
	li		r0, 0

	/* skip first entry in TLB1 */
	lwi		r3, MAS0_ESEL(1) | MAS0_TLBSEL(1)
1:	mtspr	SPR_MAS0, r3

	/* just clear valid bit in MAS1 */
	mtspr	SPR_MAS1, r0

	isync
	tlbwe
	isync

	addis	r3, r3, 1	/* increment ESEL (bit 16) */
	bdnz	1b


	/* setup an 1M entry in TLB1 for ROM mapping */
	lwi		r3, MAS0_ESEL(BOARD_PRIVATE_TLBS + 0) | MAS0_TLBSEL(1)
	mtspr	SPR_MAS0, r3

	lwi		r3, MAS1_TSIZE(TLB_SIZE_1M) | MAS1_V
	mtspr	SPR_MAS1, r3

	lwi		r3, BOARD_ROM_PHYS	/* virt */
	mtspr	SPR_MAS2, r3

	lwi		r3, BOARD_ROM_PHYS | MAS3_SR | MAS3_SX	/* phys + r-x */
	mtspr	SPR_MAS3, r3

	isync
	tlbwe
	isync


	/* setup an 1M entry in TLB1 for RAM mapping */
	lwi		r3, MAS0_ESEL(BOARD_PRIVATE_TLBS + 1) | MAS0_TLBSEL(1)
	mtspr	SPR_MAS0, r3

	lwi		r3, MAS1_TSIZE(TLB_SIZE_1M) | MAS1_V
	mtspr	SPR_MAS1, r3

	lwi		r3, BOARD_RAM_PHYS | MAS2_M	/* virt + coherent */
	mtspr	SPR_MAS2, r3

	lwi		r3, BOARD_RAM_PHYS | MAS3_SR | MAS3_SW	/* phys + rw- */
	mtspr	SPR_MAS3, r3

	isync
	tlbwe
	isync


	/* go virtual */
	lwi		r3, _virtual
	mtsrr0	r3
	li		r3, 0
	mtsrr1	r3
	rfi

_virtual:
	/* clear previous first entry in TLB1 */
	lwi		r3, MAS0_ESEL(0) | MAS0_TLBSEL(1)
	mtspr	SPR_MAS0, r3

	/* just clear valid bit */
	li		r0, 0
	mtspr	SPR_MAS1, r0

	isync
	tlbwe
	isync


	/* init FPU */
	li		r0, 0
	mtspr	SPR_SPEFSCR, r0
	isync

	/* enable branch prediction and flush internal branch cache */
	li		r3, 0x201
	mtspr	SPR_BUCSR, r3
	sync

	/* enable data and instruction cache (with parity) */
	lwi		r3, 0x00010103
	msync
	isync
	mtspr	SPR_L1CSR0, r3
	isync
	mtspr	SPR_L1CSR1, r3
	isync

	/* ... invoke C level, put invalid return address on stack */
	lwi		r1, BOOT_STACK-8
	/* also invalidate thread pointer and link register */
	li		r2, 0
	mtlr	r2
	/* terminate stack chain */
	stw		r2, 0(r1)

	b		board_init
	/* does not return */


/* setup a TLB entry in TLB1 for the kernel mapping
 * void ppc_set_tlb(uint32_t mas0, uint32_t mas1, uint32_t mas2, uint32_t mas3);
 */
ppc_set_tlb:
	mtspr	SPR_MAS0, r3
	mtspr	SPR_MAS1, r4
	mtspr	SPR_MAS2, r5
	mtspr	SPR_MAS3, r6

	isync
	tlbwe
	isync

	blr
