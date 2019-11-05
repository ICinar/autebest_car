#include <hv.h>
#include "pthread.h"

extern int main(void);

void __posix_init(void);
void __posix_init(void)
{
	pthread_init();
	sys_task_terminate();
}
