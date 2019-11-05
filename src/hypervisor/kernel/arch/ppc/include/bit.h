/*
 * bit.h
 *
 * PPC architecture specific operations on bits.
 *
 * azuepke, 2013-11-21: initial PPC port
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
	__asm__ ("cntlzw	%0, %1" : "=r" (ret) : "r" (val));
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
	addr[bit >> 5] |= 1u << (bit & 31);
}

/** clear bit in bit array */
static inline void __bit_clear(uint32_t *addr, unsigned int bit)
{
	addr[bit >> 5] &= ~(1u << (bit & 31));
}

/** test bit in bit array */
static inline int __bit_test(uint32_t *addr, unsigned int bit)
{
	return addr[bit >> 5] & 1u << (bit & 31);
}

#endif
