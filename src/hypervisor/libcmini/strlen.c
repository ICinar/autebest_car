/*
 * strlen.c
 *
 * simple byte-wise strlen
 *
 * azuepke, 2013-03-22
 */

#include <string.h>

size_t strlen(const char *s)
{
	size_t i;

	i = 0;
	while (*s++ != '\0')
		i++;

	return i;
}
