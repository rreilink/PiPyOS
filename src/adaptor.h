#ifndef ADAPTOR_H
#define ADAPTOR_H

#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int lstat(const char *path, struct stat *buf);

extern int fileno(FILE *f); // TODO: should come from stdio.h

//TODO

#define VERSION "3.6"
#define PYTHONPATH "/boot"
#define PREFIX "/boot"
#define EXEC_PREFIX "/boot"
#define VPATH "/boot"




extern void sig_ign(int);
extern void sig_err(int);


#define SIG_IGN sig_ign
#define SIG_ERR sig_err

#define CLOCK_MONOTONIC 0


#define PYTHREAD_NAME "dummy"

#endif