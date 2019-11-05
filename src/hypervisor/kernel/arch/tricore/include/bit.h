/*
 * bit.h
 *
 * Tricore architecture specific operations on bits.
 *
 * azuepke, 2014-12-16: initial
 */

#ifndef __BIT_H__
#define __BIT_H__

#include <stdint.h>
#include <hv_compiler.h>

/** find first bit set */
/** NOTE: unlike POSIX ffs(), the lowest bit is 0 */
/** NOTE: not robust if val is zero! */
static inline __pure unsigned int __bit_ffs(unsigned long val)
{
	unsigned long ret;
	__asm__ ("clz	%0, %1" : "=d" (ret) : "d" (val));
	return 31 - ret;
}

/** find last bit set */
/** NOTE: unlike POSIX ffs(), the lowest bit is 0 */
/** NOTE: not robust if val is zero! */
static inline __pure unsigned int __bit_fls(unsigned long val)
{
	return __bit_ffs(val & -val);
}

/** set bit in bit array */
static inline void __bit_set(uint32_t *addr, unsigned int bit)
{
	uint32_t *word;
	uint64_t mask;

	//addr[bit >> 5] |= 1u << (bit & 31);
	__asm__ ("addsc.at %0, %1, %2" : "=&a" (word) : "a" (addr), "d" (bit));
	__asm__ ("imask %A0, 1, %1, 1" : "=&d" (mask) : "d" (bit));
	__asm__ volatile ("ldmst %0, %A1" : "+m" (*word) : "d" (mask) : "memory");
}

/** clear bit in bit array */
static inline void __bit_clear(uint32_t *addr, unsigned int bit)
{
	uint32_t *word;
	uint64_t mask;

	//addr[bit >> 5] &= ~(1u << (bit & 31));
	__asm__ ("addsc.at %0, %1, %2" : "=&a" (word) : "a" (addr), "d" (bit));
	__asm__ ("imask %A0, 0, %1, 1" : "=&d" (mask) : "d" (bit));
	__asm__ volatile ("ldmst %0, %A1" : "+m" (*word) : "d" (mask) : "memory");
}

/** test bit in bit array */
static inline int __bit_test(uint32_t *addr, unsigned int bit)
{
	uint32_t tmp1, tmp2;
	uint32_t *word;

	//return addr[bit >> 5] & 1u << (bit & 31);
	__asm__ ("addsc.at %0, %1, %2" : "=&a" (word) : "a" (addr), "d" (bit));
	__asm__ ("ld.w %0, %1" : "=&d" (tmp1) : "m" (*word));
	__asm__ ("extr.u %0, %1, %2, 1" : "=&d" (tmp2) : "d" (tmp1), "d" (bit));

	return tmp2;
}

#endif
