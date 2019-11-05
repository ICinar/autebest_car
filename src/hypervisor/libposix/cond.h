#ifndef _COND_H_
#define _COND_H_

#define __COND_INITIALIZED(c)		(c->wq_id != 0)

int __init_cond(struct pthread_cond_str *c);
void __cond_wait(struct pthread_cond_str *cond, int prio);
void __do_cond_wait(struct pthread_cond_str *cond, int prio);
int __cond_timedwait(struct pthread_cond_str *cond, timeout_t timeout,
		     int prio);
int __do_cond_timedwait(struct pthread_cond_str *cond, timeout_t timeout,
			int prio);
void __cond_signal(struct pthread_cond_str *cond);
void __cond_broadcast(struct pthread_cond_str *cond);

#endif
