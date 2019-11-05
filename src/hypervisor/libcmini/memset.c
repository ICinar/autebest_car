/*
 * memset.c
 *
 * simple byte-wise memset
 *
 * azuepke, 2013-03-22
 */

#include <string.h>

void *memset(void *s, int c, size_t n)
{
	char *d = (char *)s;

	while (n--) {
		*d++ = (char) c;
	}

	return s;
}
