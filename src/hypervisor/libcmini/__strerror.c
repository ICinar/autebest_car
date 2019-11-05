/*
 * __strerror.c
 *
 * Internal strerror().
 *
 * azuepke, 2013-09-10: initial
 * azuepke, 2013-12-05: use OSEK/AUTOSAR error codes
 */

#include <string.h>
#include <hv_error.h>

/** strerror() */
const char *__strerror(unsigned int err)
{
	switch (err) {
	case E_OK:							return "E_OK";

	/* OSEK error codes */
	case E_OS_ACCESS:					return "E_OS_ACCESS";
	case E_OS_CALLEVEL:					return "E_OS_CALLEVEL";
	case E_OS_ID:						return "E_OS_ID";
	case E_OS_LIMIT:					return "E_OS_LIMIT";
	case E_OS_NOFUNC:					return "E_OS_NOFUNC";
	case E_OS_RESOURCE:					return "E_OS_RESOURCE";
	case E_OS_STATE:					return "E_OS_STATE";
	case E_OS_VALUE:					return "E_OS_VALUE";

	/* AUTOSAR error codes */
	case E_OS_SERVICEID:				return "E_OS_SERVICEID";
	case E_OS_ILLEGAL_ADDRESS:			return "E_OS_ILLEGAL_ADDRESS";
	case E_OS_MISSINGEND:				return "E_OS_MISSINGEND";
	case E_OS_DISABLEDINT:				return "E_OS_DISABLEDINT";
	case E_OS_STACKFAULT:				return "E_OS_STACKFAULT";
	case E_OS_PROTECTION_MEMORY:		return "E_OS_PROTECTION_MEMORY";
	case E_OS_PROTECTION_TIME:			return "E_OS_PROTECTION_TIME";
	case E_OS_PROTECTION_ARRIVAL:		return "E_OS_PROTECTION_ARRIVAL";
	case E_OS_PROTECTION_LOCKED:		return "E_OS_PROTECTION_LOCKED";
	case E_OS_PROTECTION_EXCEPTION:		return "E_OS_PROTECTION_EXCEPTION";
	case E_OS_CORE:						return "E_OS_CORE";
	case E_OS_SPINLOCK:					return "E_OS_SPINLOCK";
	case E_OS_INTERFERENCE_DEADLOCK:	return "E_OS_INTERFERENCE_DEADLOCK";
	case E_OS_NESTING_DEADLOCK:			return "E_OS_NESTING_DEADLOCK";
	case E_OS_PARAM_POINTER:			return "E_OS_PARAM_POINTER";

	/* own error codes */
	case E_OS_TIMEOUT:					return "E_OS_TIMEOUT";
	case E_OS_HARDWARE_FAULT:			return "E_OS_HARDWARE_FAULT";
	case E_OS_POWER_FAIL:				return "E_OS_POWER_FAIL";
	case E_OS_USER_ERROR:				return "E_OS_USER_ERROR";

	default:							return "E_???";
	}
}
