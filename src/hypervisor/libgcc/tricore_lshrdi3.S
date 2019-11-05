/*
 * tricore_lshrdi3.S
 *
 * TriCore compiler support library (snippets from libgcc)
 *
 * azuepke, 2015-02-02: initial
 * azuepke, 2015-03-27: split into dedicated files
 */

#include <assembler.h>

	.text

FUNC_PROLOG(__lshrdi3)
	jnz.t	%d6, 5, 2f

	rsub	%d7, %d6, 32
	rsub	%d6
	dextr	%d2, %d5, %d4, %d7
	sh		%d3, %d5, %d6
	sel		%d2, %d6, %d2, %d4
	ret

2:	add		%d6, %d6, 32
	rsub	%d6
	sh		%d2, %d5, %d6
	mov		%d3, 0
	ret
FUNC_EPILOG(__lshrdi3)
