/*
 * stdint.h
 *
 * Standard integer types.
 *
 * azuepke, 2013-03-22
 */

#ifndef __STDINT_H__
#define __STDINT_H__

#if defined __x86_64__
#define __WORDSIZE 64
#endif

#if defined __i386__
#define __WORDSIZE 32
#endif

#if defined __arm__
#define __WORDSIZE 32
#endif

#if (defined __powerpc__) || (defined __PPC__)
#define __WORDSIZE 32
#endif

#if defined __tricore__
#define __WORDSIZE 32
#endif

#ifndef __WORDSIZE
#error Adapt this file to your compiler!
#endif

/* unsigned base types */
typedef unsigned char uint8_t;
typedef volatile unsigned char vuint8_t;
typedef unsigned short int uint16_t;
typedef volatile unsigned short vuint16_t;
typedef unsigned int uint32_t;
typedef volatile unsigned int vuint32_t;
#if __WORDSIZE == 64
typedef unsigned long int uint64_t;
#else
typedef unsigned long long int uint64_t;
#endif

/* signed base types */
typedef signed char int8_t;
typedef volatile signed char vint8_t;
typedef signed short int int16_t;
typedef volatile signed short vint16_t;
typedef signed int int32_t;
typedef volatile signed int vint32_t;
#if __WORDSIZE == 64
typedef signed long int int64_t;
#else
typedef signed long long int int64_t;
#endif

#define INT8_MAX	0x7f
#define INT16_MAX	0x7fff
#define INT32_MAX	0x7fffffff
#if __WORDSIZE == 64
#define INT64_MAX	0x7ffffffffffffffful
#else
#define INT64_MAX	0x7fffffffffffffffull
#endif

#define INT8_MIN	(-INT8_MAX-1)
#define INT16_MIN	(-INT16_MAX-1)
#define INT32_MIN	(-INT32_MAX-1)
#define INT64_MIN	(-INT64_MAX-1)

#define UINT8_MAX	0xffu
#define UINT16_MAX	0xffffu
#define UINT32_MAX	0xffffffffu
#if __WORDSIZE == 64
#define UINT64_MAX	0xfffffffffffffffful
#else
#define UINT64_MAX	0xffffffffffffffffull
#endif

/* pointer types */
#if __WORDSIZE == 64
typedef signed long int intptr_t;
typedef unsigned long int uintptr_t;
#else
typedef signed int intptr_t;
typedef unsigned int uintptr_t;
#endif

/* addresses and sizes */
typedef uintptr_t addr_t;
typedef intptr_t ssize_t;
typedef uintptr_t size_t;

#endif
