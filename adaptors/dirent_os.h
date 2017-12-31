#ifndef _DIRENT_OS_H
#define _DIRENT_OS_H


/* See dirent.h on how dirent.h and dirent_os.h work together
 */
#ifdef _DIRENT_H
#error Include dirent_os.h before dirent.h (or any file that includes dirent.h)
#endif

#include "dirent.h"

#define DIR DIR_FF // ff.h defines the type DIR, but that is reserved for dirent.h. So define it to be DIR_FF
#include "ff.h"
#undef DIR


typedef struct{
    int handler;
    struct dirent dirent;
    union { // A union of structs for all possible filesystem handlers
        struct {
            int idx;
            int idx_cur;
        } initfs;
        DIR_FF ff_dir;
    };
} DIR;



int            closedir(DIR *);
DIR           *opendir(const char *);
struct dirent *readdir(DIR *);
void           seekdir(DIR *, long);
long           telldir(DIR *);

#endif