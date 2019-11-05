/*
 * ShutdownHook.c
 *
 * default ShutdownHook routine
 *
 * azuepke, 2015-04-17: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

/* internal ShutdownHook routine if the user doesn't supply one */
__weak void ShutdownHook(StatusType error_code __unused)
{
}
