/*
 * ppc_tlb.h
 *
 * PowerPC e500-style TLBs.
 *
 * azuepke, 2013-11-23: initial PPC port
 */

#ifndef __PPC_TLB_H__
#define __PPC_TLB_H__

#include <ppc_spr.h>

/* memory assist registers */

/* MAS0: entry select / replacement control */
#define MAS0_NV(x)		((x) << 0)	/* next victim */
#define MAS0_ESEL(x)	((x) << 16)	/* entry select */
#define MAS0_TLBSEL(x)	((x) << 28)	/* TLB select */

/* MAS1: TLB valid, size and associated PID */
#define MAS1_TSIZE(x)	((x) << 7)	/* translation size */
#define MAS1_TS			0x00001000	/* translation space, matches MSR.IS and DS */
#define MAS1_TID(x)		((x) << 16)	/* translation identity, compared to PID */
#define MAS1_IPROT		0x40000000	/* protect entry from invalidation */
#define MAS1_V			0x80000000	/* valid */

/* MAS2: TLB virtual address and WIMGE bits */
#define MAS2_E			0x00000001
#define MAS2_G			0x00000002
#define MAS2_M			0x00000004
#define MAS2_I			0x00000008
#define MAS2_W			0x00000010
#define MAS2_VLE		0x00000020	/* e200z4 */

/* MAS3: TLB physical address and access permission */
#define MAS3_SR			0x00000001
#define MAS3_UR			0x00000002
#define MAS3_SW			0x00000004
#define MAS3_UW			0x00000008
#define MAS3_SX			0x00000010
#define MAS3_UX			0x00000020
#define MAS3_U3			0x00000040
#define MAS3_U2			0x00000080
#define MAS3_U1			0x00000100
#define MAS3_U0			0x00000200

/* MAS4: default WIMGE and size settings preloaded on MMU exceptions */
#define MAS4_TLBSELD(x)	((x) << 28)
#define MAS4_TIDSELD(x)	((x) << 16)
#define MAS4_TSIZED(x)	((x) << 7)
#define MAS4_ED			0x00000001
#define MAS4_GD			0x00000002
#define MAS4_MD			0x00000004
#define MAS4_ID			0x00000008
#define MAS4_WD			0x00000010
#define MAS4_VLED		0x00000020

/* MAS5: hardware virtualization */

/* MAS6: PID and address space for TLB searches */
#define MAS6_SAS		0x00000001
#define MAS6_SPID(x)	((x) << 16)

/* MAS7: TLB physical address upper bits */

/* MAS9: hardware virtualization */

#define TLB_SIZE_1K		0x00	/* e200z4 only */
#define TLB_SIZE_2K		0x01	/* e200z4 only */
#define TLB_SIZE_4K		0x02
#define TLB_SIZE_8K		0x03	/* e200z4 only */
#define TLB_SIZE_16K	0x04
#define TLB_SIZE_32K	0x05	/* e200z4 only */
#define TLB_SIZE_64K	0x06
#define TLB_SIZE_128K	0x07	/* e200z4 only */
#define TLB_SIZE_256K	0x08
#define TLB_SIZE_512K	0x09	/* e200z4 only */
#define TLB_SIZE_1M		0x0A
#define TLB_SIZE_2M		0x0B	/* e200z4 only */
#define TLB_SIZE_4M		0x0C
#define TLB_SIZE_8M		0x0D	/* e200z4 only */
#define TLB_SIZE_16M	0x0E
#define TLB_SIZE_32M	0x0F	/* e200z4 only */
#define TLB_SIZE_64M	0x10
#define TLB_SIZE_128M	0x11	/* e200z4 only */
#define TLB_SIZE_256M	0x12
#define TLB_SIZE_512M	0x13	/* e200z4 only */
#define TLB_SIZE_1G		0x14	/* e200z4 only */
#define TLB_SIZE_2G		0x15	/* e200z4 only */
#define TLB_SIZE_4G		0x16	/* e200z4 only */

#endif
