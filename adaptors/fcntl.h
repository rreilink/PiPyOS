#ifndef _FCNTL_H
#define _FCNTL_H

int fcntl (int fd, int command, ...);

// fcntl commands
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD 5

#endif