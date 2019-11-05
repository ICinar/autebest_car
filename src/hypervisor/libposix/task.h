#ifndef _TASK_H_
#define _TASK_H_

#include "pthread.h"

int alloc_free_task(void);
void free_task(int id);

#endif
