/*
 * printf.c
 *
 * Console output handling functions, kernel version.
 *
 * azuepke, 2013-03-22: initial
 * azuepke, 2013-05-17: added user mode build
 * azuepke, 2013-11-24: rework for MPU kernel
 */

#include <kernel.h>
#include <board.h>
#include <hv_error.h>

static inline __alwaysinline void putc(int c)
{
	unsigned int err;

	if (c == '\n') {
		do {
			err = board_putc('\r');
		} while (err != E_OK);
	}

	do {
		err = board_putc(c);
	} while (err != E_OK);
}

static inline __alwaysinline void puts(const char *s)
{
	unsigned int err;

	while (*s != '\0') {
		do {
			err = board_putc(*s);
		} while (err != E_OK);
		s++;
	}
}

static const char hex[16] = "0123456789abcdef";

static inline __alwaysinline void putx(uint64_t h, int width)
{
	unsigned int err;
	int shift;
	int c;

	for (shift = width * 4 - 4; shift >= 0; shift -= 4) {
		c = hex[(h >> shift) & 0xf];

		do {
			err = board_putc(c);
		} while (err != E_OK);
	}
}

static inline __alwaysinline uint64_t divide_by_10(uint64_t n, unsigned int *rem)
{
	/* divide by 10 algorithm from Hacker's delight */
	uint64_t q, r;
	uint32_t t;

	q = (n >> 1) + (n >> 2);
	q += q >> 4;
	q += q >> 8;
	q += q >> 16;
	q += q >> 32;
	q >>= 3;
	r = n - (q << 3) - (q << 1);	/* n - (q * 10) */
	t = (r + 6) >> 4;
	q += t;							/* q + (r > 9) */

	*rem = (uint32_t)r - (t << 3) - (t << 1);
	return q;
}

static inline __alwaysinline void putd(uint64_t num, int width)
{
	unsigned int rem;
	char tmp[21];		/* 2^64-1 = "18446744073709551615\0", 21 chars */
	int pos;

	pos = sizeof(tmp) - 1;
	if (width > pos)
		width = pos;

	tmp[pos] = '\0';
	do {
		num = divide_by_10(num, &rem);
		pos--;
		tmp[pos] = '0' + rem;
	} while (num > 0);

	while ((int)sizeof(tmp) - 1 - pos < width) {
		pos--;
		tmp[pos] = ' ';
	}

	puts(&tmp[pos]);
}

void _vprintf(const char* format, va_list args)
{
	uint64_t num;
	int l, z;	/* length modifiers: long, long long, size_t */
	int fmode;
	int sign;
	int base;
	int width;
	char c;

	goto reset_statemachine;
	while ((c = *format++) != '\0') {
		if (!fmode) {
			/* normal text mode */
			if (c == '%') {
				fmode = 1;
			} else {
				putc(c);
			}
			continue;
		}

		/* field witdth */
		if (c >= '0' && c <= '9') {
			width = width * 10 + (c - '0');
			continue;
		}

		/* length modifiers */
		switch (c) {
		case 'l':
			l++;
			continue;
		case 'z':
			z = 1;
			continue;
		}

		/* conversion specifiers */
		switch (c) {
		case '%':
			putc(c);
			goto reset_statemachine;

		case 'c':
			putc(va_arg(args, int));
			goto reset_statemachine;

		case 's':
			puts(va_arg(args, char *));
			goto reset_statemachine;

		case 'p':	/* pointer */
			z = 1;
			/* fall-through */

		case 'x':	/* hex */
			base = 16;
			break;

		case 'd':	/* decimal */
			sign = 1;
			/* fall-through */
		case 'u':
			base = 10;
			break;
		}

		/* "z" overrides l or ll and any given field width */
		if (z) {
			l = 1;
			width = 0;
		}

		if (l == 0) {
			num = va_arg(args, int);
		}
#if __WORDSIZE < 64
		else if (l >= 2) {
			num = va_arg(args, long long);
		}
#endif
		else {
			num = va_arg(args, long);
			/* indicate in l that a long is just 32-bit */
#if __WORDSIZE < 64
			l = 0;
#endif
		}

		/* hex */
		if (base == 16) {
			/* a given width overrides natural width */
			putx(num, width ? width : (l ? 16 : 8));
			goto reset_statemachine;
		}

		/* decimal */
		if (sign && ((int64_t)num < 0)) {
			num = -num;
			putc('-');
		}
		putd(num, width);

reset_statemachine:
		fmode = 0;
		l = 0;
		z = 0;
		sign = 0;
		base = 0;
		width = 0;
	}
}

void _printf(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	_vprintf(format, args);
	va_end(args);
}
