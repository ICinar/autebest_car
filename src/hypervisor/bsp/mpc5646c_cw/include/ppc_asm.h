/*
 * ppc_asm.h
 *
 * PPC assembler helper.
 *
 * azuepke, 2013-11-21: initial PPC port
 */

#ifndef __PPC_ASM_H__
#define __PPC_ASM_H__

/** GPRs */
#define r0	0
#define r1	1
#define r2	2
#define r3	3
#define r4	4
#define r5	5
#define r6	6
#define r7	7
#define r8	8
#define r9	9
#define r10	10
#define r11	11
#define r12	12
#define r13	13
#define r14	14
#define r15	15
#define r16	16
#define r17	17
#define r18	18
#define r19	19
#define r20	20
#define r21	21
#define r22	22
#define r23	23
#define r24	24
#define r25	25
#define r26	26
#define r27	27
#define r28	28
#define r29	29
#define r30	30
#define r31	31

/** FPRs */
#define f0	0
#define f1	1
#define f2	2
#define f3	3
#define f4	4
#define f5	5
#define f6	6
#define f7	7
#define f8	8
#define f9	9
#define f10	10
#define f11	11
#define f12	12
#define f13	13
#define f14	14
#define f15	15
#define f16	16
#define f17	17
#define f18	18
#define f19	19
#define f20	20
#define f21	21
#define f22	22
#define f23	23
#define f24	24
#define f25	25
#define f26	26
#define f27	27
#define f28	28
#define f29	29
#define f30	30
#define f31	31

/** SPE vectors */
#define evr0	0
#define evr1	1
#define evr2	2
#define evr3	3
#define evr4	4
#define evr5	5
#define evr6	6
#define evr7	7
#define evr8	8
#define evr9	9
#define evr10	10
#define evr11	11
#define evr12	12
#define evr13	13
#define evr14	14
#define evr15	15
#define evr16	16
#define evr17	17
#define evr18	18
#define evr19	19
#define evr20	20
#define evr21	21
#define evr22	22
#define evr23	23
#define evr24	24
#define evr25	25
#define evr26	26
#define evr27	27
#define evr28	28
#define evr29	29
#define evr30	30
#define evr31	31

/** CR Fields */
#define cr0	0
#define cr1	1
#define cr2	2
#define cr3	3
#define cr4	4
#define cr5	5
#define cr6	6
#define cr7	7

#define addi e_add16i
#define stw e_stw
#define li e_li
#define lis e_lis
#define lwz e_lwz
#define blr se_blr
#define b e_b
#define bl e_bl
#define isync se_isync
#define sync msync
#define rfi se_rfi
#define stmw e_stmw
#define bdnz e_bdnz
#define blt e_blt

/** load 32-bit immediates */
.macro lwi reg imm32
	lis		\reg, \imm32@h
	e_or2i		\reg, \imm32@l
.endm

#endif
