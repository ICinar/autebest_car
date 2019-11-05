/*
 * tc_io.h
 *
 * TriCore specific I/O access.
 *
 * NOTE: uses a different order than Linux:
 *   outb(port, value);
 *   writel(addr, value);
 *
 * azuepke, 2014-12-31: initial
 */

#ifndef __TC_IO_H__
#define __TC_IO_H__

#include <stdint.h>

static inline uint8_t readb(const volatile void *addr)
{
	uint8_t ret;
	__asm__ volatile("ld.bu %0, %1" : "=d" (ret)
	                 : "m" (*(const volatile uint8_t *)addr) : "memory");
	return ret;
}

static inline uint16_t readw(const volatile void *addr)
{
	uint16_t ret;
	__asm__ volatile("ld.hu %0, %1" : "=d" (ret)
	                 : "m" (*(const volatile uint16_t *)addr) : "memory");
	return ret;
} 

static inline uint32_t readl(const volatile void *addr)
{
	uint32_t ret;
	__asm__ volatile("ld.w %0, %1" : "=d" (ret)
	                 : "m" (*(const volatile uint32_t *)addr) : "memory");
	return ret;
}


static inline void writeb(volatile void *addr, uint8_t val)
{
	__asm__ volatile ("st.b %1, %0" : : "d" (val),
	                  "m" (*(volatile uint8_t *)addr) : "memory");
}

static inline void writew(volatile void *addr, uint16_t val)
{
	__asm__ volatile ("st.h %1, %0" : : "d" (val),
	                  "m" (*(volatile uint16_t *)addr) : "memory");
}

static inline void writel(volatile void *addr, uint32_t val)
{
	__asm__ volatile ("st.w %1, %0" : : "d" (val),
	                  "m" (*(volatile uint32_t *)addr) : "memory");
}

#endif
