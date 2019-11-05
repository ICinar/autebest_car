/*
 * Test support functions for getting timestamps and checking stack usage
 */

unsigned long gettime(void);

void stackcheck_init(void);

/* measure stack size */
unsigned long getusedstack(void);

/* print long */
void print_long(unsigned long);
void print_hlong(unsigned long);
void print_hbuf(const char *, unsigned long);
void print_char(char);
#define print_string(s)  serial_put(s)

#define print_lwl(a, b) \
  do { print_string(a); print_long(b); print_char('\n'); } while (0)

#define print_hwl(a, b) \
  do { print_string(a); print_hlong(b); print_char('\n'); } while (0)

#define print_bwl(a, b, c) \
  do { print_string(a); print_hbuf(b, c); print_char('\n'); } while (0)

/* external functions */
void serial_put(const char *);

void test_main(void);

void update_LEDs(unsigned char);
