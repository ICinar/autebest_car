/*
 * hv_error.h
 *
 * Error codes
 *
 * azuepke, 2013-05-09: initial
 * azuepke, 2013-12-05: use OSEK/AUTOSAR error codes
 */

#ifndef __HV_ERROR_H__
#define __HV_ERROR_H__

#ifndef E_OK
#define E_OK						0
#endif

/** OSEK error codes */
/* "out of range" and "stuff is already in use" errors */
#define E_OS_ACCESS					1
/* call at interrupt level */
#define E_OS_CALLEVEL				2
/* invalid ID */
#define E_OS_ID						3
/* Maximum number of activations reached, no effect */
#define E_OS_LIMIT					4
/* operation has no effect */
#define E_OS_NOFUNC					5
/* still occupies a resource */
#define E_OS_RESOURCE				6
/* target is in wrong state or already in use */
#define E_OS_STATE					7
/* outside configured limits */
#define E_OS_VALUE					8

/** AUTOSAR error codes */
#define E_OS_SERVICEID				9
#define E_OS_ILLEGAL_ADDRESS		10
#define E_OS_MISSINGEND				11
#define E_OS_DISABLEDINT			12
#define E_OS_STACKFAULT				13
#define E_OS_PROTECTION_MEMORY		14
#define E_OS_PROTECTION_TIME		15
#define E_OS_PROTECTION_ARRIVAL		16
#define E_OS_PROTECTION_LOCKED		17
#define E_OS_PROTECTION_EXCEPTION	18
#define E_OS_CORE					19
#define E_OS_SPINLOCK				20
#define E_OS_INTERFERENCE_DEADLOCK	21
#define E_OS_NESTING_DEADLOCK		22
#define E_OS_PARAM_POINTER			23

/** own error codes */
#define E_OS_TIMEOUT				24
#define E_OS_HARDWARE_FAULT			25
#define E_OS_POWER_FAIL				26
#define E_OS_USER_ERROR				27

/* NOTE: do not define error codes beyond 31 in the kernel API,
 *       so they fit in 5 bits, see HM table implementation!
 */

#endif
