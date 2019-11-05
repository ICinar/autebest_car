/*
 * sciregs.h
 *
 * TMS570 Serial Communication Interface (SCI)
 * See TMS570 manual chapter 27
 *
 * azuepke, 2014-01-27: initial
 */

#ifndef __TMS570_SCIREGS_H__
#define __TMS570_SCIREGS_H__

#include <stdint.h>

/** Serial Communication Interface located at 0xfff7e400 to 0xfff7e45f
 *  See TMS570 manual chapter 27.7
 */
typedef volatile struct {
	uint32_t GCR0;					/* 0x0000 Global Control Register 0 */
	uint32_t GCR1;					/* 0x0004 Global Control Register 1 */
	uint32_t rsdv1;					/* 0x0008 Global Control Register 2 */
	uint32_t SETINT;				/* 0x000C Set Interrupt Enable Register */
	uint32_t CLEARINT;				/* 0x0010 Clear Interrupt Enable Register */
	uint32_t SETINTLVL;				/* 0x0014 Set Interrupt Level Register */
	uint32_t CLEARINTLVL;			/* 0x0018 Set Interrupt Level Register */
	uint32_t FLR; 					/* 0x001C Interrupt Flag Register */
	uint32_t INTVECT0;				/* 0x0020 Interrupt Vector Offset 0 */
	uint32_t INTVECT1;				/* 0x0024 Interrupt Vector Offset 1 */
	uint32_t FORMAT;				/* 0x0028 Format Control Register */
	uint32_t BRS; 					/* 0x002C Baud Rate Selection Register */
	uint32_t ED; 					/* 0x0030 Emulation Register */
	uint32_t RD; 					/* 0x0034 Receive Data Buffer */
	uint32_t TD; 					/* 0x0038 Transmit Data Buffer */
	uint32_t PIO0;					/* 0x003C Pin Function Register */
	uint32_t PIO1;					/* 0x0040 Pin Direction Register */
	uint32_t PIO2;					/* 0x0044 Pin Data In Register */
	uint32_t PIO3;					/* 0x0048 Pin Data Out Register */
	uint32_t PIO4;					/* 0x004C Pin Data Set Register */
	uint32_t PIO5;					/* 0x0050 Pin Data Clr Register */
	uint32_t PIO6;					/* 0x0054: Pin Open Drain Output Enable Register */
	uint32_t PIO7;					/* 0x0058: Pin Pullup/Pulldown Disable Register */
	uint32_t PIO8;					/* 0x005C: Pin Pullup/Pulldown Selection Register */
	uint32_t rsdv2[12U];			/* 0x0060: Reserved                */
	uint32_t IODFTCTRL;				/* 0x0090: I/O Error Enable Register */
} scireg_t;

#define SCIREG1 ((scireg_t *)0xfff7e400U)
#define SCIREG2 ((scireg_t *)0xfff7e500U)


/** SCI interrupt flags */
#define SCI_INT_FE		0x04000000	/* framming error */
#define SCI_INT_OE		0x02000000	/* overrun error */
#define SCI_INT_PE		0x01000000	/* parity error */
#define SCI_INT_RX		0x00000200	/* receive buffer ready */
#define SCI_INT_TX		0x00000100	/* transmit buffer ready */
#define SCI_INT_WAKE	0x00000002	/* wakeup */
#define SCI_INT_BREAK	0x00000001	/* break */

#endif
