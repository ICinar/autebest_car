/*
 * stdarg.h
 *
 * Codewarrior specific variable argument handling.
 *
 * azuepke, 2013-03-22
 */

#ifndef __STDARG_CW_H__
#define __STDARG_CW_H__

/* HACK */
#define va_start(v, l)
#define va_arg(v, l) (0)
#define va_end(v)
#define va_copy(d, s)
typedef int	va_list;

#endif
