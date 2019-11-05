/*
 * apex.c
 *
 * APEX internal lib code and data
 *
 * azuepke, 2014-09-08: initial
 */

#include <assert.h>
#include <hv_error.h>
#include "apex.h"

/** internal state of the APEX library */
uint8_t __apex_operating_mode;
uint8_t __apex_start_condition;
uint8_t __apex_lock_level;
uint8_t __apex_error_handler_created;
uint16_t __apex_previous_process;

/** user scheduling state (imported from libsys) */
extern user_sched_state_t __sys_sched_state;

/** APEX initialization */
void __apex_init(void)
{
	unsigned int operating_mode;

	/* get operating mode */
	operating_mode = sys_part_get_operating_mode();
	assert((operating_mode == COLD_START) ||
	       (operating_mode == WARM_START));
	__apex_operating_mode = operating_mode;

	/* FIXME: a second const upper limit to check the hook IDs would be nice */
	assert(__apex_init_hook_id >= __apex_num_procs);
	assert(__apex_error_handler_hook_id >= __apex_num_procs);

	/* lock priorities of the hooks */
	__apex_proc_prio[__apex_init_hook_id] = 255;
	__apex_proc_prio[__apex_error_handler_hook_id] = 255;

	__apex_lock_level = 1;

	/* FIXME: no way to retrieve the start condition from the kernel API */
	__apex_start_condition = NORMAL_START;

	main();
	/* __apex_startup() aborts if main() returns */
}

/** invocation stub of APEX error handler */
void __apex_error_handler_invoke(void (*func)(void))
{
	if (__apex_error_handler_created) {
		__apex_previous_process = INVALID_ID;	/* FIXME! get process ID */

		func();
	}
	sys_abort();
}

/** translate a kernel error code into an APEX error code */
unsigned int __apex_error_translate(unsigned int err)
{
	assert(E_OK == NO_ERROR);

	switch (err) {
	case E_OK:
		return NO_ERROR;

	case E_OS_NOFUNC:
		return NO_ACTION;

	case E_OS_STATE:
		return NOT_AVAILABLE;

	case E_OS_ID:
		return INVALID_PARAM;

	case E_OS_VALUE:
		return INVALID_CONFIG;

	case E_OS_RESOURCE:
		return INVALID_MODE;

	case E_OS_TIMEOUT:
		return TIMED_OUT;

	/* FIXME: hacky */
	default:
		return 100 + err;
	}
}

/** ARINC configuration mismatch */
void __apex_panic(const char *msg)
{
	unsigned int err;
	const char *s;

	s = "PANIC: ARINC CONFIG MISMATCH: ";
	while (*s != '\0') {
		do {
			err = sys_putchar(*s);
		} while (err != E_OK);
		s++;
	}

	s = msg;
	while (*s != '\0') {
		do {
			err = sys_putchar(*s);
		} while (err != E_OK);
		s++;
	}

	/* FIXME: inject INVALID_CONFIG error into HM! */
	sys_abort();
}

/** Compare first 30 characters of APEX names,
 * returnes != 1 if the names are considered equal
 */
/* NOTE: same as: !strncmp(s1, s2, MAX_NAME_LENGTH) */
int __apex_name_eq(const char *s1, const char *s2)
{
	size_t i;

	for (i = 0; i < MAX_NAME_LENGTH; i++) {
		if (s1[i] != s2[i]) {
			return 0;
		}
		if (s1[i] == '\0') {
			break;
		}
	}

	return 1;
}

/** Copy up to 30 characters of APEX names, pad with NUL-characters */
/* NOTE: same as: !strncmp(s1, s2, MAX_NAME_LENGTH) */
void __apex_name_cpy(char *d, const char *s)
{
	size_t i;

	for (i = 0; i < MAX_NAME_LENGTH; i++) {
		d[i] = s[i];
		if (s[i] == '\0') {
			break;
		}
	}
	for (; i < MAX_NAME_LENGTH; i++) {
		d[i] = '\0';
	}
}

/** Find ID of APEX process by name, returns INVALID_ID if not found */
unsigned int __apex_proc_find(const char *name)
{
	const struct apex_proc_cfg *cfg;
	unsigned int id;

	cfg = __apex_proc_cfg;
	for (id = 0; id < __apex_num_procs; id++, cfg++) {
		if (__apex_name_eq(name, cfg->name)) {
			return id;
		}
	}

	return INVALID_ID;
}

/** Find ID of APEX semaphore by name, returns INVALID_ID if not found */
unsigned int __apex_sem_find(const char *name)
{
	const struct apex_sem_cfg *cfg;
	unsigned int id;

	cfg = __apex_sem_cfg;
	for (id = 0; id < __apex_num_sems; id++, cfg++) {
		if (__apex_name_eq(name, cfg->name)) {
			return id;
		}
	}

	return INVALID_ID;
}
