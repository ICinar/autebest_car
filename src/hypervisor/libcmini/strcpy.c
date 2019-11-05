/*
 * strcpy.c
 *
 * simple byte-wise strcpy
 *
 * azuepke, 2013-05-03
 */

#include <string.h>

char *strcpy(char *dst, const char *src)
{
	char *d = dst;

	while (*src) {
		*d++ = *src++;
	}
	*d = '\0';

	return dst;
}
