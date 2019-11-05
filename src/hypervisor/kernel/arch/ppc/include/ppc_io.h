/*
 * ppc_io.h
 *
 * PPC specific I/O access.
 *
 * NOTE: uses a different order than Linux:
 *   outb(port, value);
 *   writel(addr, value);
 *
 * azuepke, 2013-11-21: initial PPC port
 */

#ifndef __PPC_IO_H__
#define __PPC_IO_H__

#include <stdint.h>

static inline uint8_t readb(const volatile void *addr)
{
	uint8_t ret;
	__asm__ volatile("lbz %0, %1" : "=r" (ret)
	                 : "m" (*(const volatile uint8_t *)addr) : "memory");
	return ret;
}

static inline uint16_t readw(const volatile void *addr)
{
	uint16_t ret;
	__asm__ volatile("lhz %0, %1" : "=r" (ret)
	                 : "m" (*(const volatile uint16_t *)addr) : "memory");
	return ret;
}

static inline uint32_t readl(const volatile void *addr)
{
	uint32_t ret;
	__asm__ volatile("lwz %0, %1" : "=r" (ret)
	                 : "m" (*(const volatile uint32_t *)addr) : "memory");
	return ret;
}


static inline void writeb(volatile void *addr, uint8_t val)
{
	__asm__ volatile ("stb %0, %1" : : "r" (val),
	                  "m" (*(volatile uint8_t *)addr) : "memory");
}

static inline void writew(volatile void *addr, uint16_t val)
{
	__asm__ volatile ("sth %0, %1" : : "r" (val),
	                  "m" (*(volatile uint16_t *)addr) : "memory");
}

static inline void writel(volatile void *addr, uint32_t val)
{
	__asm__ volatile ("stw %0, %1" : : "r" (val),
	                  "m" (*(volatile uint32_t *)addr) : "memory");
}

#endif
