/*
 * assembler.h
 *
 * Tricore specific assembler definitions
 *
 * azuepke, 2014-10-25: initial
 */

#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#define ALIAS(func, alias)		\
	.global alias				;\
	.equiv alias, func

#define FUNC_PROLOG(name)		\
	.global name				;\
	.type name, %function		;\
	name:						;

#define FUNC_EPILOG(name)		\
	.size name, . - name		;

#define FUNC(name)				\
	.type name, %function		;\
	name:						;

#define DATA(name)				\
	.type name, %object			;\
	name:						;

#endif
