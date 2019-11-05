/*
 * error.c
 *
 * ARINC error handling.
 *
 * azuepke, 2014-09-08: initial
 */

#include "apex.h"

void REPORT_APPLICATION_MESSAGE (
/*in */ MESSAGE_ADDR_TYPE MESSAGE_ADDR,
/*in */ MESSAGE_SIZE_TYPE LENGTH,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	// FIXME: IMPLEMENTME
	assert(0);
	(void)MESSAGE_ADDR;
	(void)LENGTH;
	*RETURN_CODE = 42;
}

void CREATE_ERROR_HANDLER (
/*in */ SYSTEM_ADDRESS_TYPE ENTRY_POINT,
/*in */ STACK_SIZE_TYPE STACK_SIZE,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	if (__apex_in_normal_mode()) {
		*RETURN_CODE = INVALID_MODE;
		return;
	}
	if (__apex_error_handler_created) {
		*RETURN_CODE = NO_ACTION;
		return;
	}
	if (STACK_SIZE > __apex_error_handler_stack_size) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}
	if ((uint32_t)ENTRY_POINT != __apex_error_handler_entry_point) {
		__apex_panic("CREATE_ERROR_HANDLER");
	}

	__apex_error_handler_created = 1;
	*RETURN_CODE = NO_ERROR;
}

void GET_ERROR_STATUS (
/*out*/ ERROR_STATUS_TYPE *ERROR_STATUS,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	// FIXME: IMPLEMENTME
	assert(0);
	(void)ERROR_STATUS;
	*RETURN_CODE = 42;
}

void RAISE_APPLICATION_ERROR (
/*in */ ERROR_CODE_TYPE ERROR_CODE,
/*in */ MESSAGE_ADDR_TYPE MESSAGE_ADDR,
/*in */ ERROR_MESSAGE_SIZE_TYPE LENGTH,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	// FIXME: IMPLEMENTME
	assert(0);
	(void)ERROR_CODE;
	(void)MESSAGE_ADDR;
	(void)LENGTH;
	*RETURN_CODE = 42;
}
