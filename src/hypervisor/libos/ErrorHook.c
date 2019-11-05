/*
 * ErrorHook.c
 *
 * default ErrorHook routine
 *
 * azuepke, 2015-03-12: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

/* internal ErrorHook routine if the user doesn't supply one */
__weak void ErrorHook(StatusType error_code __unused)
{
}
