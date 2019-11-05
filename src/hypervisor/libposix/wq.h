#ifndef _WQ_H_
#define _WQ_H_

#include "pthread.h"

int alloc_free_wq(void);
void free_wq(int id);

#endif
