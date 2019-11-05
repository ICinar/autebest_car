/*
 * hv_compiler.h
 *
 * Compiler specific macros.
 *
 * azuepke, 2013-03-22
 */

#ifndef __HV_COMPILER_H__
#define __HV_COMPILER_H__


#ifdef CODEWARRIOR
#include "hv_compiler-cw.h"
#else
#include "hv_compiler-gcc.h"
#endif

#endif
