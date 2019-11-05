/*
 * GetActiveApplicationMode.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

AppModeType GetActiveApplicationMode(void)
{
	return __OsApplicationMode;
}
