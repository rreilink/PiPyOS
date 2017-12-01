#ifndef _OS_H
#define _OS_H


struct timeval {
    long	tv_sec;		/* seconds */
    long	tv_usec;	/* and microseconds */
};

struct timezone {
    int tz_minuteswest;     /* minutes west of Greenwich */
    int tz_dsttime;         /* type of DST correction */
};


int gettimeofday(struct timeval *tv, struct timezone *tz);

int clock_getres(clockid_t clk_id, struct timespec *res);
int clock_gettime(clockid_t clk_id, struct timespec *tp);


typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);

#endif