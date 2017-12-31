#ifndef _INITFS_H
#define _INITFS_H

#include "dirent_os.h"

typedef struct {
    char *data;
    int pos;
    int size;

} initfs_openfile_t;


int PiPyOS_initfs_open(initfs_openfile_t *file, const char *pathname, int flags);
int PiPyOS_initfs_read(initfs_openfile_t *file, void *buf, size_t count);
int PiPyOS_initfs_opendir(const char *pathname, DIR *dir);
int PiPyOS_initfs_readdir(DIR *dirp);
int PiPyOS_initfs_stat(const char *pathname, struct stat *buf);

#endif