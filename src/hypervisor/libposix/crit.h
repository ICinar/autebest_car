#define __CRIT_PRIO_MAX 255

unsigned int __crit_enter(void);
void __crit_leave(unsigned int oldprio);
