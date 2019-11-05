/*
 * stdarg.h
 *
 * GCC specific variable argument handling.
 *
 * azuepke, 2013-03-22
 */

#ifndef __STDARG_H__
#define __STDARG_H__


#ifdef CODEWARRIOR
#include "stdarg-cw.h"
#else
#include "stdarg-gcc.h"
#endif

#endif
