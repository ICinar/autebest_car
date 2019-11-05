/*
 * memcpy.c
 *
 * simple byte-wise memcpy
 *
 * azuepke, 2013-03-22
 */

#include <string.h>

void *memcpy(void *dst, const void *src, size_t n)
{
	const char *s = (const char *)src;
	char *d = (char *)dst;

	while (n--) {
		*d++ = *s++;
	}

	return dst;
}
