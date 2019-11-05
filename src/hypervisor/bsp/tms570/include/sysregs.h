/*
 * sysregs.h
 *
 * TMS570 Primary System Control Registers (SYS)
 * See TMS570 manual chapter 2.5
 *
 * azuepke, 2014-01-29: initial
 */

#ifndef __TMS570_SYSREGS_H__
#define __TMS570_SYSREGS_H__

#include <stdint.h>

/** System Register Frame 1 located at 0xffffff00 to 0xffffffff
 *  See TMS570 manual chapter 2.5.1
 */
typedef volatile struct {
	uint32_t SYSPC1;				/* 0x0000 */
	uint32_t SYSPC2;				/* 0x0004 */
	uint32_t SYSPC3;				/* 0x0008 */
	uint32_t SYSPC4;				/* 0x000C */
	uint32_t SYSPC5;				/* 0x0010 */
	uint32_t SYSPC6;				/* 0x0014 */
	uint32_t SYSPC7;				/* 0x0018 */
	uint32_t SYSPC8;				/* 0x001C */
	uint32_t SYSPC9;				/* 0x0020 */
	uint32_t SSWPLL1;				/* 0x0024 */
	uint32_t SSWPLL2;				/* 0x0028 */
	uint32_t SSWPLL3;				/* 0x002C */
	uint32_t CSDIS; 				/* 0x0030 */
	uint32_t CSDISSET;				/* 0x0034 */
	uint32_t CSDISCLR;				/* 0x0038 */
	uint32_t CDDIS; 				/* 0x003C */
	uint32_t CDDISSET;				/* 0x0040 */
	uint32_t CDDISCLR;				/* 0x0044 */
	uint32_t GHVSRC;				/* 0x0048 */
	uint32_t VCLKASRC;				/* 0x004C */
	uint32_t RCLKSRC;				/* 0x0050 */
	uint32_t CSVSTAT;				/* 0x0054 */
	uint32_t MSTGCR;				/* 0x0058 */
	uint32_t MINITGCR;				/* 0x005C */
	uint32_t MSINENA;				/* 0x0060 */
	uint32_t MSTFAIL;				/* 0x0064 */
	uint32_t MSTCGSTAT;				/* 0x0068 */
	uint32_t MINISTAT;				/* 0x006C */
	uint32_t PLLCTL1;				/* 0x0070 */
	uint32_t PLLCTL2;				/* 0x0074 */
	uint32_t UERFLAG;				/* 0x0078 */
	uint32_t DIEIDL;				/* 0x007C */
	uint32_t DIEIDH;				/* 0x0080 */
	uint32_t VRCTL; 				/* 0x0084 */
	uint32_t LPOMONCTL;				/* 0x0088 */
	uint32_t CLKTEST;				/* 0x008C */
	uint32_t DFTCTRLREG1;			/* 0x0090 */
	uint32_t DFTCTRLREG2;			/* 0x0094 */
	uint32_t   reserved1;			/* 0x0098 */
	uint32_t   reserved2;			/* 0x009C */
	uint32_t GPREG1;				/* 0x00A0 */
	uint32_t BTRMSEL;				/* 0x00A4 */
	uint32_t IMPFASTS;				/* 0x00A8 */
	uint32_t IMPFTADD;				/* 0x00AC */
	uint32_t SSISR1;				/* 0x00B0 */
	uint32_t SSISR2;				/* 0x00B4 */
	uint32_t SSISR3;				/* 0x00B8 */
	uint32_t SSISR4;				/* 0x00BC */
	uint32_t RAMGCR;				/* 0x00C0 */
	uint32_t BMMCR1;				/* 0x00C4 */
	uint32_t BMMCR2;				/* 0x00C8 */
	uint32_t MMUGCR;				/* 0x00CC */
	uint32_t CLKCNTL;				/* 0x00D0 */
	uint32_t ECPCNTL;				/* 0x00D4 */
	uint32_t DSPGCR;				/* 0x00D8 */
	uint32_t DEVCR1;				/* 0x00DC */
	uint32_t SYSECR;				/* 0x00E0 */
	uint32_t SYSESR;				/* 0x00E4 */
	uint32_t SYSTASR;				/* 0x00E8 */
	uint32_t GBLSTAT;				/* 0x00EC */
	uint32_t DEV;   				/* 0x00F0 */
	uint32_t SSIVEC;				/* 0x00F4 */
	uint32_t SSIF;  				/* 0x00F8 */
} sysreg_frame1_t;

#define SYSREG1	((sysreg_frame1_t *)0xffffff00U)

/** System Register Frame 2 located at 0xffffe100 to 0xffffe1ff
 *  See TMS570 manual chapter 2.5.2
 */
typedef volatile struct {
	uint32_t PLLCTL3;				/* 0x0000 */
	uint32_t   reserved1;			/* 0x0004 */
	uint32_t STCCLKDIV;				/* 0x0008 */
	uint32_t   reserved2[6];		/* 0x000C */
	uint32_t ECPCNTRL0;				/* 0x0024 */
	uint32_t   reserved3[5];		/* 0x0028 */
	uint32_t CLK2CNTL;				/* 0x003C */
	uint32_t VCLKACON1;				/* 0x0040 */
	uint32_t   reserved4[11];		/* 0x0044 */
	uint32_t CLKSLIP;				/* 0x0070 */
	uint32_t   reserved5[30];   	/* 0x0074 */
	uint32_t EFC_CTLEN;				/* 0x00EC */
	uint32_t DIEIDL_REG0;			/* 0x00F0 */
	uint32_t DIEIDH_REG1;			/* 0x00F4 */
	uint32_t DIEIDL_REG2;			/* 0x00F8 */
	uint32_t DIEIDH_REG3;			/* 0x00FC */
} sysreg_frame2_t;

#define SYSREG2	((sysreg_frame2_t *)0xffffe100U)


/** Peripheral Central Resource (PCR) Control Registers located at 0xffffe000
 *  See TMS570 manual chapter 2.5.2
 */
typedef volatile struct {
	uint32_t PMPROTSET0;			/* 0x0000 */
	uint32_t PMPROTSET1;			/* 0x0004 */
	uint32_t   reserved1[2];		/* 0x0008 */
	uint32_t PMPROTCLR0;			/* 0x0010 */
	uint32_t PMPROTCLR1;			/* 0x0014 */
	uint32_t   reserved2[2];		/* 0x0018 */
	uint32_t PPROTSET0; 			/* 0x0020 */
	uint32_t PPROTSET1; 			/* 0x0024 */
	uint32_t PPROTSET2; 			/* 0x0028 */
	uint32_t PPROTSET3; 			/* 0x002C */
	uint32_t   reserved3[4];		/* 0x0030 */
	uint32_t PPROTCLR0; 			/* 0x0040 */
	uint32_t PPROTCLR1; 			/* 0x0044 */
	uint32_t PPROTCLR2; 			/* 0x0048 */
	uint32_t PPROTCLR3; 			/* 0x004C */
	uint32_t   reserved4[4];		/* 0x0050 */
	uint32_t PCSPWRDWNSET0;			/* 0x0060 */
	uint32_t PCSPWRDWNSET1;			/* 0x0064 */
	uint32_t   reserved5[2];		/* 0x0068 */
	uint32_t PCSPWRDWNCLR0;			/* 0x0070 */
	uint32_t PCSPWRDWNCLR1;			/* 0x0074 */
	uint32_t   reserved6[2];		/* 0x0078 */
	uint32_t PSPWRDWNSET0;			/* 0x0080 */
	uint32_t PSPWRDWNSET1;			/* 0x0084 */
	uint32_t PSPWRDWNSET2;			/* 0x0088 */
	uint32_t PSPWRDWNSET3;			/* 0x008C */
	uint32_t   reserved7[4];		/* 0x0090 */
	uint32_t PSPWRDWNCLR0;			/* 0x00A0 */
	uint32_t PSPWRDWNCLR1;			/* 0x00A4 */
	uint32_t PSPWRDWNCLR2;			/* 0x00A8 */
	uint32_t PSPWRDWNCLR3;			/* 0x00AC */
} pcrreg_t;

#define PCRREG	((pcrreg_t *)0xffffe000U)

#endif
