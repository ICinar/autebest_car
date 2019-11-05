/*
 * ProtectionHook.c
 *
 * default ProtectionHook routine
 *
 * azuepke, 2015-03-12: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

/* internal ProtectionHook routine if the user doesn't supply one */
__weak ProtectionReturnType ProtectionHook(StatusType Fatalerror __unused)
{
	return PRO_SHUTDOWN;
}
