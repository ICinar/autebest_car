/*
 * assert.h
 *
 * Assert.
 *
 * azuepke, 2013-05-03: split from kernel.h
 */

#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <hv_compiler.h>

/** runtime assert */
#ifdef NDEBUG
#define assert(cond) do { } while (0)
#else
/** assert internal function */
void __assert(const char *file, int line, const char *func, const char *cond) __noreturn __cold __tc_fastcall;

#define assert(cond) \
	do { \
		if (cond) { \
			/* EMPTY */ \
		} else { \
			__assert(__FILE__, __LINE__, __FUNCTION__, __stringify(cond)); \
		} \
	} while (0)

#endif

/** static assert, verified at compile time:
 * use the _underscore variant in global scope.
 */
#define _static_assert(cond) \
	typedef int __concatenate(__STATIC_ASSERT_FAILED, __COUNTER__)[(cond) ? 1 : -1]

#define static_assert(cond) \
	do { \
		_static_assert(cond); \
	} while (0)

#endif
