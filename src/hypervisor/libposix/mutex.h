#ifndef _MUTEX_H_
#define _MUTEX_H_

/* internal mutex stuff */
#define __MUTEX_STATE_UNINITIALIZED 0x00
#define __MUTEX_STATE_UNLOCKED		0x10
#define __MUTEX_STATE_LOCKED		0x20
#define __MUTEX_STATE_HASWAITERS	0x40

#define __MUTEX_INITIALIZED(m)		(m->wq_id != 0)

int __init_mutex(struct pthread_mutex_str *m, int type, int protocol);
int __lock_mutex(struct pthread_mutex_str *m, int protocol);
int __trylock__mutex(struct pthread_mutex_str *m);
int __lock_mutex_timeout(struct pthread_mutex_str *m, timeout_t timeout);
int __unlock_mutex(struct pthread_mutex_str *m, int protocol);
unsigned int __do_unlock_mutex(struct pthread_mutex_str *m, int protocol,
			       unsigned int crit);

#endif
