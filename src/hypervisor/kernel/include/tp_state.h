/*
 * tp_state.h
 *
 * Kernel time partition windows and schedules.
 *
 * azuepke, 2015-04-22: initial
 */

#ifndef __TP_STATE_H__
#define __TP_STATE_H__

#include <stdint.h>

/** upper limit of time partitions in the system (so we can use 8-bit indices) */
#define MAX_TIMEPARTS	256

/** upper limit of time partition schedules in the system (... 8-bit indices) */
#define MAX_TPSCHEDULES	256

/** upper limit of windows in the system (so we can use 16-bit indices) */
#define MAX_TPWINDOWS	65536

/* Time partition window flags */
#define TPWINDOW_FLAG_FIRST		0x01	/**< first window in schedule */
#define TPWINDOW_FLAG_LAST		0x02	/**< last window in schedule */
#define TPWINDOW_FLAG_RELEASE	0x04	/**< partition release point */

/** Time partition window configuration */
struct tpwindow_cfg {
	/** associated time partition ID */
	uint8_t timepart;
	/** time partition window flags */
	uint8_t flags;
	uint16_t padding;

	/** window duration in nanoseconds (limits us to 4.2 seconds) */
	uint32_t duration;
};

/** Time partition schedule table configuration */
struct tpschedule_cfg {
	/** start */
	const struct tpwindow_cfg *start;
};

#endif
