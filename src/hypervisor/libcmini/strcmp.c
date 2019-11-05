/*
 * strcmp.c
 *
 * simple byte-wise strcmp
 *
 * azuepke, 2013-04-06
 */

#include <string.h>

int strcmp(const char *s1, const char *s2)
{
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;

	while ((*us1 != '\0') && (*us1 == *us2)) {
		us1++;
		us2++;
	}

	return (*us1 < *us2) ? -1 : (*us1 > *us2);
}
