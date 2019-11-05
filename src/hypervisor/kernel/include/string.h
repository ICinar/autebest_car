/*
 * string.h
 *
 * String handling functions.
 *
 * azuepke, 2013-03-22
 */

#ifndef __STRING_H__
#define __STRING_H__

#include <stdint.h>

/* memcpy.c */
/** simple byte-wise memcpy() */
void *memcpy(void *dst, const void *src, size_t n);

/* memset.c */
/** simple byte-wise memset() */
void *memset(void *s, int c, size_t n);

/* memcmp.c */
/** simple byte-wise memcp() */
int memcmp(const void *s1, const void *s2, size_t n);

/* strlen.c */
/** simple strlen() */
size_t strlen(const char *s);

/* strcmp.c */
/** simple strcmp() */
int strcmp(const char *s1, const char *s2);

/* strcpy.c */
/** simple strcpy() */
char *strcpy(char *dst, const char *src);

/* __strerror.c */
/** simple strerror() */
const char *__strerror(unsigned int err);

#endif
