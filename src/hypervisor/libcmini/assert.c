/*
 * assert.c
 *
 * Assert.
 *
 * azuepke, 2014-03-04: initial
 */

#include <hv.h>
#include <stdio.h>
#include <assert.h>

/* the assert function always exists, regardless of the macro defition */
void __assert(const char *file, int line, const char *func, const char *cond) __tc_fastcall;

void __assert(const char *file, int line, const char *func, const char *cond)
{
	printf("%s:%d: %s: assertion '%s' failed.\n", file, line, func, cond);
	sys_abort();
}
