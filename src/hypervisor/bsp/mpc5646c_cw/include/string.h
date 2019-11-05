/*
 * string.h
 *
 * String handling functions.
 *
 * azuepke, 2013-03-22
 */

#ifndef __STRING_H__
#define __STRING_H__

typedef unsigned int size_t;

void *memcpy(void *dst, const void *src, size_t n);

void *memset(void *, int, size_t);

int memcmp(const void *, const void *, size_t);

#endif
