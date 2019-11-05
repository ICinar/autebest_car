#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <assert.h>

//#define NDEBUG

#ifdef NDEBUG
#define DEBUG_PRINTF(...) ;
#undef assert
#define assert(x) if ((x)) {};
#else
#define DEBUG_PRINTF printf("POSIX_DBG: "); printf
#endif

#endif
