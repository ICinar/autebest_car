/*
 * start.S
 *
 * Assembler startup code for MPC5646C
 *
 * tjordan, 2014-07-15: initial version for MPC5646C
 */

#include <board_stuff.h>
#include <ppc_asm.h>
#include <ppc_spr.h>
#include <ppc_tlb.h>


	.global __start
	.global ppc_set_tlb
	.global bam_rchw
	.global bam_resetvector
	.extern board_init
	.extern board_init_clocks
	.extern __rom_data_start
	.extern __data_start
	.extern __data_end
	.extern __bss_end
	/*.extern _SDA2_BASE_
	.extern _SDA_BASE_ */

	.data
	.section .text.rchw, 16, 1, 2

	/* special boot word RCHW for BAM */
bam_rchw:
	.long 0x015A0000
	/* entry function */
bam_resetvector:
	.long __start

	.text_vle
	.section .text.start, 16, 1, 6
	/* entry point from boot loader */
__start:
	/* keep load address in r30 */
	bl		@la1
@la1:	mflr	r31
	addi	r31, r31, -4
	b		_real_start

_real_start:
	/* disable all interrupts */
	li		r0, 0
	mtmsr	r0

_tlb_setup:
	/* After booting, we've got one TLB entry set up, giving us access to the
	 * 4K page including the start address.
	 * We now want to set up TLB entries for all of ROM, RAM and peripherals.
	 * However, TLB entries must not overlap: the MMU will fail silently and
	 * return corrupt addresses if more than one entry matches.
	 * So, to get a working entry for all ROM while running out of ROM, we do
	 * the following:
	 * - create a valid temporary entry in address space 1 for all ROM
	 * - switch MSR to address space 1 (the boot entry is in address space 0)
	 * - now, we can update the boot entry to cover all ROM
	 * - and then we'll switch back to address space 0.
	 */

	/* set up temporary entry for ROM mapping: we'll use entry number 1 */
	lwi		r3, MAS0_ESEL(1) | MAS0_TLBSEL(1)
	lwi		r4, MAS1_TSIZE(BOARD_ROM_TLBSIZE) | MAS1_TS | MAS1_V /* TS is the translation address space */
	lwi		r5, BOARD_ROM_PHYS | MAS2_VLE
	lwi		r6, BOARD_ROM_PHYS | MAS3_SR | MAS3_SX	/* phys + r-x */

	bl		ppc_set_tlb

	/* switch data and instruction address space to 1 (DS=IS=1 in MSR) */
	li		r3, 0x30
	isync
	mtmsr	r3
	isync

	/* set up entry for ROM mapping */
	lwi		r3, MAS0_ESEL(BOARD_PRIVATE_TLBS) | MAS0_TLBSEL(1)
	lwi		r4, MAS1_TSIZE(BOARD_ROM_TLBSIZE) | MAS1_V | MAS1_IPROT
	lwi		r5, BOARD_ROM_PHYS | MAS2_VLE
	lwi		r6, BOARD_ROM_PHYS | MAS3_SR | MAS3_SX	/* phys + r-x */

	bl		ppc_set_tlb

	/* set up entry for RAM mapping */
	lwi		r3, MAS0_ESEL(BOARD_PRIVATE_TLBS + 1) | MAS0_TLBSEL(1)
	lwi		r4, MAS1_TSIZE(BOARD_RAM_TLBSIZE) | MAS1_V
	lwi		r5, BOARD_RAM_PHYS
	lwi		r6, BOARD_RAM_PHYS | MAS3_SR | MAS3_SW	/* phys + rw- */

	bl		ppc_set_tlb

	/* peripheral bridge 1 */
	/* G bit in MAS2: peripherals must be guarded as accesses may have side effects */
	lwi		r3, MAS0_ESEL(BOARD_PRIVATE_TLBS + 2) | MAS0_TLBSEL(1)
	lwi		r4, MAS1_TSIZE(BOARD_PER1_TLBSIZE) | MAS1_V
	lwi		r5, BOARD_PER1_PHYS | MAS2_G
	lwi		r6, BOARD_PER1_PHYS | MAS3_SR | MAS3_SW	/* phys + rw- */

	bl		ppc_set_tlb

	/* peripheral bridge 0 */
	/* G bit in MAS2: peripherals must be guarded as accesses may have side effects */
	lwi		r3, MAS0_ESEL(BOARD_PRIVATE_TLBS + 3) | MAS0_TLBSEL(1)
	lwi		r4, MAS1_TSIZE(BOARD_PER0_TLBSIZE) | MAS1_V
	lwi		r5, BOARD_PER0_PHYS | MAS2_G
	lwi		r6, BOARD_PER0_PHYS | MAS3_SR | MAS3_SW	/* phys + rw- */

	bl		ppc_set_tlb

_tlb_cleanup:
	/* before we can switch back to address space 0 again, we need to
	 * invalidate the boot TLB entry */
	lwi		r3, MAS0_ESEL(0) | MAS0_TLBSEL(1)
	li		r0, 0
	mtspr	SPR_MAS0, r3
	mtspr	SPR_MAS1, r0
	mtspr	SPR_MAS2, r0
	mtspr	SPR_MAS3, r0

	isync
	tlbwe
	isync

	/* switch data and instruction address space back to 0 */
	mtmsr	r0
	isync

	/* now, clear remains of TLB1 */
	/* unfortunately, we cannot use flash invalidation in MMUCSR0,
	 * as entries marked with IPROT are not cleared. */
	/* entry 0 has already been invalidated above, now we're invalidating
	 * starting from entry 1 up to (BOARD_PRIVATE_TLBS - 1) */
	lwi		r3, (BOARD_PRIVATE_TLBS - 1)
	mtctr	r3

	/* skip first entry in TLB1 */
	lwi		r3, MAS0_ESEL(1) | MAS0_TLBSEL(1)
@tlbl:
	mtspr	SPR_MAS0, r3

	/* just clear valid bit in MAS1 */
	mtspr	SPR_MAS1, r0

	isync
	tlbwe
	isync

#if 0
	addis	r3, r3, 1	/* increment ESEL (bit 16) */
#else
	e_add2is	r3, 1	/* increment ESEL (bit 16) */
#endif
	bdnz	@tlbl

_init_hw_stackpointer:
	/* initialize 128 bytes of stack using stmw */
	lwi		r3, (BOARD_RAM_PHYS+BOARD_RAM_SIZE)-128
	stmw	r0, 0(r3)
	/* set up an initial stack pointer */
	lwi		r1, (BOARD_RAM_PHYS+BOARD_RAM_SIZE)-16

_init_hw_sdastuff:
	/*lwi		r2, _SDA2_BASE_
	lwi		r13, _SDA_BASE_*/

_init_hw_before_data:
	/* before starting to copy data from RAM to ROM, be sure to set up the PLL
	 * so that we don't waste time. as clock setup is rather tedious, we'll do it
	 * using C code. however, we don't have a complete C execution environment yet! */
	bl		board_init_clocks

_init_ram:
	/* ECC RAM needs initialisation before it can be used */

	lwi		r3, BOARD_RAM_PHYS
	/* will use stmw to write 32 registers - 128 bytes - per iteration. no need
	 * write the last 128 byte block, as this was initialized above for stack */
	lwi		r4, ((BOARD_RAM_SIZE / 128) - 1)
	mtctr	r4
	/* FIXME: could optimize this: don't initialize stuff that gets overwritten
	 * by .data or .bss below anyway - however, this would need special knowledge
	 * about the position and the alignment of these sections. */

#ifndef USE_RANDOM_VALUES_FOR_RAM_INIT
	/* add some markers */
	lwi		r0, 0xDEADBEEF /* traditional for uninitialized memory */
	mr		r1, r0
	mr		r2, r0
	/* r3 holds the address */
	mr		r4, r0
	mr		r5, r0
	mr		r6, r0
	mr		r7, r0
	mr		r8, r0
	mr		r9, r0
	mr		r10, r0
	mr		r11, r0
	mr		r12, r0
	mr		r13, r0
	mr		r14, r0
	mr		r15, r0
	mr		r16, r0
	mr		r17, r0
	mr		r18, r0
	mr		r19, r0
	mr		r20, r0
	mr		r21, r0
	mr		r22, r0
	mr		r23, r0
	mr		r24, r0
	mr		r25, r0
	mr		r26, r0
	mr		r27, r0
	mr		r28, r0
	mr		r29, r0
	mr		r30, r0
	mr		r31, r0
#endif /* USE_MARKERS */

@ramloop:
	stmw	r0, 0(r3)
	addi	r3, r3, 128
	bdnz	@ramloop

	/*
	 * copy .data
	 */
_copy_data:
	lwi		r3, __rom_data_start
	lwi		r4, __data_start
	lwi		r5, __data_end

@dataloop:
	lwz		r0, 0(r3)
	addi	r3, r3, 4
	stw		r0, 0(r4)
	addi	r4, r4, 4
	cmplw	r4, r5
	blt	@dataloop

	/*
	 * clear .bss
	 */
_clear_bss:
	li		r0, 0
	lwi		r5, __bss_end

@bssloop:
	stw		r0, 0(r4)
	addi	r4, r4, 4
	cmplw	r4, r5
	blt	@bssloop


_common_start:
	li		r0, 0
	mtspr	SPR_PID0, r0
	isync

	/* go virtual - FIXME is this needed? */
	lwi		r3, _virtual
	mtsrr0	r3
	li		r3, 0
	mtsrr1	r3
	rfi

_virtual:

	/* init FPU */
	mtspr	SPR_SPEFSCR, r0

	/* enable branch prediction and flush internal branch cache */
	li		r3, 0x200 /* flush */
	mtspr	SPR_BUCSR, r3
	sync
#ifndef DISABLE_HWOPT
	li		r3, 1 /* enable */
	mtspr	SPR_BUCSR, r3
	sync
#endif

#define HID0_DOZE	0x00800000 /* enable doze power management mode */
#define HID0_ICR	0x00020000 /* interrupt inputs clear reservation */
#define HID0_TBEN	0x00004000 /* TimeBase enable */

	/* init HID0: enable DOZE mode, enable time base counter */
	lwi		r3, HID0_DOZE | HID0_ICR | HID0_TBEN
	mtspr	SPR_HID0, r3
	isync

_init_stackpointer:
	/* set up the stack pointer */
	lwi		r1, BOOT_STACK-16

	/* invalidate link register: set an end marker that will result in an exception when used */
	lwi		r2, 0xAFFEBAFF
	mtlr	r2
	/* terminate stack chain */
	stw		r2, 0(r1)

_init_sdastuff:
	/*lwi		r2, _SDA2_BASE_
	lwi		r13, _SDA_BASE_*/

	bl		board_init
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
