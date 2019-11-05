#include <board_stuff.h>
#include <test_support.h>

void test_main(void)
{
	unsigned long start, end, stack;
	/* hello? */
	stackcheck_init();
	start = gettime();
	print_string("\nhello?\n");
	print_string("is there anybody in there?\n");
	print_string("just nod if you can hear me.\n");
	print_string("is there anyone home?\n\n");
	end = gettime();
	stack = getusedstack();
	print_hwl("start: 0x", start);
	print_hwl("  end: 0x", end);
	print_lwl("ticks: ", end - start);
	print_lwl("\nstack (bytes): ", stack);
}
