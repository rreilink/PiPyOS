#ifndef _OS_H
#define _OS_H

#include <sys/time.h>

int clock_getres(clockid_t clk_id, struct timespec *res);
int clock_gettime(clockid_t clk_id, struct timespec *tp);
void os_init_stdio(void);

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);

#endif