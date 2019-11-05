#ifndef _TIME_H_
#define _TIME_H_

#include <hv_types.h>

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

#endif
