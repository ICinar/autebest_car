#include <string.h>

void *memset(void *s, int c, size_t n)
{
	/* optimization: if s is aligned, len is divisible by 4, and c is 0, use 32bit writes */
	if ( (((n | (unsigned)s) & 3) | c) == 0 ) {
		unsigned *d = (unsigned *)s;
		while (n) {
			n -= 4;
			*d++ = 0;
		}
	} else {
		char *d = (char *)s;
		while (n--) {
			*d++ = (char) c;
		}
	}

	return s;
}

void *memcpy(void *dst, const void *src, size_t n)
{
	/* optimization: if everything is aligned and len is divisible by 4, use 32bit access */
	if ( ((n | (unsigned)src | (unsigned)dst) & 3) == 0 ) {
		const unsigned *s = (const unsigned *)src;
		unsigned *d = (unsigned *)dst;

		while (n) {
			n -= 4;
			*d++ = *s++;
		}
	} else {
		const char *s = (const char *)src;
		char *d = (char *)dst;

		while (n--) {
			*d++ = *s++;
		}
	}

	return dst;
}

/* CAUTION: optimized implementation that only returns whether data differs! */
/* return value:         0 - contents of s1 and s2 are the same
                 all other - contents of s1 and s2 differ */
int memcmp(const void * s1, const void * s2, size_t n)
{
	/* optimization: if everything is aligned and len is divisible by 4, use 32bit access */
	if ( ((n | (unsigned)s1 | (unsigned)s2) & 3) == 0 ) {
		const unsigned *a = (const unsigned *)s1;
		const unsigned *b = (const unsigned *)s2;

		while (n && (*a == *b)) {
			n -= 4;
			a++;
			b++;
		}
	} else {
		const char *a = (const char *)s1;
		const char *b = (const char *)s2;

		while (n && (*a == *b)) {
			n--;
			a++;
			b++;
		}
	}

	return n;
}
