#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/syslimits.h>

struct dirent {
    ino_t  d_ino;
    char d_name[NAME_MAX];
};

typedef struct{
    int idx;
    int idx_cur;
    struct dirent dirent;
} DIR;



int            closedir(DIR *);
DIR           *opendir(const char *);
struct dirent *readdir(DIR *);
void           seekdir(DIR *, long);
long           telldir(DIR *);

#endif