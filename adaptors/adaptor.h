#ifndef ADAPTOR_H
#define ADAPTOR_H

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "fcntl.h"
#include <errno.h>

#include "adaptordefs.h"


//TODO

#define VERSION "3.6"
#define PYTHONPATH "/boot"
#define PREFIX "/boot"
#define EXEC_PREFIX "/boot"
#define VPATH "/boot"

int clock_getres(clockid_t clk_id, struct timespec *res);
int clock_gettime(clockid_t clk_id, struct timespec *tp);
int clock_settime(clockid_t clk_id, const struct timespec *tp);



// O_
#define O_NONBLOCK 1
#define O_RDONLY 2



// fcntl mask
#define FD_CLOEXEC 1


#define S_IFDIR 1 // used in S_ISDIR in pyport.h
#define S_IFREG 2 // used in S_ISREG in pyport.h

extern void sig_ign(int);
extern void sig_err(int);


#define SIG_IGN sig_ign
#define SIG_ERR sig_err

#define CLOCK_MONOTONIC 0


#define PYTHREAD_NAME "dummy"

#endif