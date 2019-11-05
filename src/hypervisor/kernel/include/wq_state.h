/*
 * wq_state.h
 *
 * ARINC compatible abtract wait queue state.
 *
 * azuepke, 2014-09-05: initial
 */

#ifndef __WQ_STATE_H__
#define __WQ_STATE_H__

#include <stdint.h>
#include <list.h>

/*
 * Wait queues are represented by two arrays, one for configuration data
 * and one for dynamic runtime data. Entries on both array relate to each
 * other. Partitions can access assigned ranges of wait queues.
 *
 * For wait queues to become usable, they need the link attribute stored
 * in the configuration. The link either points to it own wait queue (itself)
 * for a normal wait queue used for semaphores, or to another wait queue
 * forming producer-consumer-relationships used for queuing ports or buffers.
 */


/** upper limit of wait queues (so we can use 16-bit indices) */
#define MAX_WQS	65536

/** max number of waiting tasks on a wait queue (for 8-bit indices) */
#define NUM_WAITERS	255

/** Wait queue state */
#define WQ_STATE_CLOSED			0
#define WQ_STATE_READY			1

/* forward declaration */
struct wq;

/** static wait queue configuration:
 */
struct wq_cfg {
	/** pointer to other wait queue (if wait queue is connected by a channel) */
	struct wq *link;
};

/** runtime wait queue data:
 */
struct wq {
	/** queue of waiting tasks (double-linked list) */
	list_t waitq;

	/** wait queue associated user state */
	uint32_t *user_state;

	/** queuing discipline */
	uint8_t discipline;
	/** wait queue state, non-zero if initialized by the user */
	uint8_t state;
	/** associated processor */
	uint8_t cpu_id;
	uint8_t padding;
};

#endif
