/*
 * Test support functions for getting timestamps and checking stack usage
 */
#include <test_support.h>

/* get current time.
 */
unsigned long gettime(void)
{
	unsigned long tmp;
	__asm__ volatile ("mftbl %0": "=r" (tmp) : : "memory");
	return tmp;
}

static const unsigned long * test_stackend = 0;

#define STACKSIZE_MAX  (2 * 1024ul)

/* prepare for measuring stack size
 */
void stackcheck_init(void)
{
	__asm__ volatile ("mr %0, 1": "=r" (test_stackend) : : "memory");
	__asm__ volatile (
						"	b		2f\n"
						"1:	stw		%2, 0(%0)\n"
						"	addi	%0, %0, 4\n"
						"2:	cmplw	%0, %1\n"
						"	blt+	1b"
						: /* no out */
						: "r" (test_stackend - STACKSIZE_MAX), "r" (test_stackend - 4), "r" (0xA070BE57)
						: "memory"
					  );
}

/* measure stack size */
unsigned long getusedstack(void)
{
	const unsigned long * p;
	unsigned long n = 0;

	if (test_stackend == 0) {
		return 0;
	}

	for (p = (test_stackend - STACKSIZE_MAX); p < test_stackend; p++) {
		if (*p == 0xA070BE57) {
			n += 4;
		} else {
			break;
		}
	}

	if (n == 0) {
		return 0;
	} else {
		return ((STACKSIZE_MAX * 4) - n);
	}
}

/* print helpers. these are actually hardware-independent, but who cares.. */
#define BUF_SIZE 64
static char tmpbuf[BUF_SIZE];

static const char hex[16] = "0123456789abcdef";

void print_long(unsigned long d)
{
	char * a = tmpbuf;
	char * r = a;
	char c;

	do {
		*a = hex[d % 10];
		d /= 10;
		a++;
	} while (d > 0);

	*a = 0;

	a--;
	while (a > r) {
		c = *r;
		*r = *a;
		*a = c;
		r++;
		a--;
	}

	serial_put(tmpbuf);
}

void print_hlong(unsigned long h)
{
	int shift;
	char * a = tmpbuf;

	for (shift = 28; shift >= 0; shift -= 4) {
		*a = hex[(h >> shift) & 0xf];
		a++;
	}
	*a = 0;

	serial_put(tmpbuf);
}

void print_hbuf(const char * s, unsigned long n)
{
	unsigned long i = 0;
	char * a = tmpbuf;

	while (i < n) {
		*a = hex[((unsigned) *s >> 4) & 0xf];
		a++;
		*a = hex[((unsigned) *s) & 0xf];
		a++;
		i++;
		s++;
		if (i > ((BUF_SIZE/2) - 2)) {
			*a = 0;
			serial_put(tmpbuf);
			n -= ((BUF_SIZE/2) - 2);
			i = 0;
			a = tmpbuf;
		}
	}
	*a = 0;
	serial_put(tmpbuf);
}

void print_char(char c)
{
	char a[2];
	a[0] = c;
	a[1] = 0;
	serial_put(a);
}
