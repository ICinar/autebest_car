/*
 * memcmp.c
 *
 * simple byte-wise memcmp()
 *
 * azuepke, 2015-10-13
 */

#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;

	while ((n > 0) && (*us1 == *us2)) {
		us1++;
		us2++;
		n--;
	}

	return (*us1 < *us2) ? -1 : (*us1 > *us2);
}
