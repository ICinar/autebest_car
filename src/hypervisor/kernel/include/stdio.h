/*
 * stdio.h
 *
 * Console output handling functions
 *
 * azuepke, 2013-03-22
 */

#ifndef __STDIO_H__
#define __STDIO_H__

#include <hv_compiler.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/** simple vprintf() */
void vprintf(const char* format, va_list args);

/** simple printf()
 *
 * field width:
 *  0   zero padding (ignored)
 *  nn  decimal field width
 *
 * length modifiers:
 *  l   long
 *  ll  long long
 *  z   size_t or uintptr_t, native register width
 *
 * conversion specifiers:
 *  c    char
 *  s    string
 *  p    pointer (implicit 'z')
 *  x    hex
 *  d    signed decimal
 *  u    unsigned decimal
 *
 * TIPS:
 * - use '%zx' to print register values
 *
 * NOTE:
 * - for hex numbers, a given field width truncates the number
 * - for decimals, a field width aligns to the right
 */
void printf(const char *format, ...) __printflike(1, 2);

#endif
