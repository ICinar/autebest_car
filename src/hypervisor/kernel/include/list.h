/*
 * list.h
 *
 * Double linked lists.
 *
 * azuepke, 2013-03-27
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <hv_compiler.h>

/** internal list type */
struct __list {
	struct __list *next;
	struct __list *prev;
};

/** list head or node */
typedef struct __list list_t;


/** assert that a list is NULL */
#define __list_assert_is_null(list)	\
	do {	\
		assert((list)->next == NULL);	\
		assert((list)->prev == NULL);	\
	} while (0)

/** assert that a list is not NULL */
#define __list_assert_is_not_null(list)	\
	do {	\
		assert((list)->next != NULL);	\
		assert((list)->prev != NULL);	\
	} while (0)

/** add element n between a and b to list */
#define __list_add(a, n, b)	\
	do {	\
		(b)->prev = (n);	\
		(n)->next = (b);	\
		(n)->prev = (a);	\
		(a)->next = (n);	\
	} while (0)

/** delete element n between a and b from list */
#define __list_del(a, n, b)	\
	do {	\
		(a)->next = (b);	\
		(b)->prev = (a);	\
	} while (0)


/** static list head initializer */
#define LIST_HEAD_INIT(head) { &(head), &(head) }

/** init list head */
static inline __alwaysinline void list_head_init(list_t *head) __nonnull(1);
static inline __alwaysinline void list_head_init(list_t *head)
{
	head->next = head;
	head->prev = head;
}


/** init list node */
static inline __alwaysinline void list_node_init(list_t *node) __nonnull(1);
static inline __alwaysinline void list_node_init(list_t *node)
{
#ifdef NDEBUG
	/* dummy -- prevent compiler warning */
	(void)node;
#else
	node->next = NULL;
	node->prev = NULL;
#endif
}

/** static list node initializer */
#define LIST_NODE_INIT(node) { NULL, NULL }


/** check if a list head is empty */
static inline __alwaysinline int list_is_empty(const list_t *head) __nonnull(1);
static inline __alwaysinline int list_is_empty(const list_t *head)
{
	__list_assert_is_not_null(head);
	return head->next == head;
}


/** get the first element of from a non-empty list */
static inline __alwaysinline list_t *__list_first(const list_t *head) __nonnull(1);
static inline __alwaysinline list_t *__list_first(const list_t *head)
{
	__list_assert_is_not_null(head);
	return head->next;
}

/** get the last element of from a non-empty list */
static inline __alwaysinline list_t *__list_last(const list_t *head) __nonnull(1);
static inline __alwaysinline list_t *__list_last(const list_t *head)
{
	__list_assert_is_not_null(head);
	return head->prev;
}

/** get the first element of from a list or NULL if the list is empty */
static inline __alwaysinline list_t *list_first(const list_t *head) __nonnull(1);
static inline __alwaysinline list_t *list_first(const list_t *head)
{
	__list_assert_is_not_null(head);
	return (head->next != head) ? head->next : NULL;
}

/** get the last element of from a list or NULL if the list is empty */
static inline __alwaysinline list_t *list_last(const list_t *head) __nonnull(1);
static inline __alwaysinline list_t *list_last(const list_t *head)
{
	__list_assert_is_not_null(head);
	return (head->prev != head) ? head->prev : NULL;
}


/** get the next element after a node in a list */
static inline __alwaysinline list_t *list_next(const list_t *head, const list_t *node) __nonnull(1, 2);
static inline __alwaysinline list_t *list_next(const list_t *head, const list_t *node)
{
	__list_assert_is_not_null(head);
	__list_assert_is_not_null(node);
	return (node->next != head) ? node->next : NULL;
}

/** get the previous element before a node in a list */
static inline __alwaysinline list_t *list_prev(const list_t *head, const list_t *node) __nonnull(1, 2);
static inline __alwaysinline list_t *list_prev(const list_t *head, const list_t *node)
{
	__list_assert_is_not_null(head);
	__list_assert_is_not_null(node);
	return (node->prev != head) ? node->prev : NULL;
}


/** remove a linked node from its list */
static inline __alwaysinline void list_del(list_t *node) __nonnull(1);
static inline __alwaysinline void list_del(list_t *node)
{
	__list_assert_is_not_null(node);

	__list_del(node->prev, node, node->next);

#ifndef NDEBUG
	/* debugging */
	node->next = NULL;
	node->prev = NULL;
#endif
}

/** remove and return the first element from a non-empty list */
static inline __alwaysinline list_t *__list_remove_first(list_t *head) __nonnull(1);
static inline __alwaysinline list_t *__list_remove_first(list_t *head)
{
	list_t *node;

	node = __list_first(head);
	list_del(node);

	return node;
}

/** remove and return the last element from a non-empty list */
static inline __alwaysinline list_t *__list_remove_last(list_t *head) __nonnull(1);
static inline __alwaysinline list_t *__list_remove_last(list_t *head)
{
	list_t *node;

	node = __list_last(head);
	list_del(node);

	return node;
}

/** remove and return the first element from a list or NULL if the list is empty */
static inline __alwaysinline list_t *list_remove_first(list_t *head) __nonnull(1);
static inline __alwaysinline list_t *list_remove_first(list_t *head)
{
	list_t *node;

	node = list_first(head);
	if (node != NULL) {
		list_del(node);
	}

	return node;
}

/** remove and return the last element from a list or NULL if the list is empty */
static inline __alwaysinline list_t *list_remove_last(list_t *head) __nonnull(1);
static inline __alwaysinline list_t *list_remove_last(list_t *head)
{
	list_t *node;

	node = list_last(head);
	if (node != NULL) {
		list_del(node);
	}

	return node;
}


/** insert node at beginning of list */
static inline __alwaysinline void list_add_first(list_t *head, list_t *node) __nonnull(1, 2);
static inline __alwaysinline void list_add_first(list_t *head, list_t *node)
{
	list_t *next;

	__list_assert_is_not_null(head);
	__list_assert_is_null(node);

	next = head->next;
	__list_add(head, node, next);
}

/** insert node at tail of list */
static inline __alwaysinline void list_add_last(list_t *head, list_t *node) __nonnull(1, 2);
static inline __alwaysinline void list_add_last(list_t *head, list_t *node)
{
	list_t *prev;

	__list_assert_is_not_null(head);
	__list_assert_is_null(node);

	prev = head->prev;
	__list_add(prev, node, head);
}


/** concatenate non-empty list head2 at the end of list head1 */
static inline __alwaysinline void list_concat(list_t *head1, list_t *head2) __nonnull(1, 2);
static inline __alwaysinline void list_concat(list_t *head1, list_t *head2)
{
	list_t *prev1, *prev2;

	__list_assert_is_not_null(head1);
	__list_assert_is_not_null(head2);

	prev1 = head1->prev;
	prev2 = head2->prev;

	head1->prev = prev2;
	prev1->next = head2;
	head2->prev = prev1;
	prev2->next = head1;
}


/** insert an entry into a sorted list where criteria becomes true */
/* NOTE: __ITER__ is the internal iterator */
#define list_add_sorted(head, node, criteria)	\
	do {	\
		list_t *_head = (head);	\
		list_t *_node = (node);	\
		list_t *__ITER__, *_prev;	\
		__list_assert_is_not_null(_head);	\
		__list_assert_is_null(_node);	\
		__ITER__ = _head->next;	\
		while (__ITER__ != _head) {	\
			if (criteria)	\
				break;	\
			__ITER__ = __ITER__->next;	\
		}	\
		_prev = __ITER__->prev;	\
		__list_add(_prev, _node, __ITER__);	\
	} while (0)


/** magic cast to get surrounding data structure where list is embedded in */
#define list_entry(list, type, member)	\
	container_of(list, type, member)

#endif
