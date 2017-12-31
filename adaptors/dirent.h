#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/syslimits.h>
#include <sys/types.h>

struct dirent {
    ino_t  d_ino;
    char d_name[NAME_MAX];
};

/* The actual DIR structure contents is defined in dirent_os.h. We define it
 * here as void such that code that uses dirent functions, will not need to
 * depend on all the dependencies that the DIR structure has (filesystem handler
 * specific stuff)
 *
 * All c-source files that actually implement (parts of) the following functions
 * and thus need access to the DIR structure internals, will include dirent_os.h
 * instead.
 *
 * To prevent conflicts, the following section is not included when included
 * from dirent_os.h
 */

#ifndef _DIRENT_OS_H

typedef void DIR;

int            closedir(DIR *);
DIR           *opendir(const char *);
struct dirent *readdir(DIR *);
void           seekdir(DIR *, long);
long           telldir(DIR *);

#endif

#endif