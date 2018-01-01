#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include "dirent_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <utime.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include "bcm2835.h" // for clock_gettime
#include "bcmframebuffer.h"

#define DIR DIR_FF // ff.h defines the type DIR, but that is reserved for dirent.h. So define it to be DIR_FF
#include "ff.h"
#undef DIR

#include "initfs.h"

#if 0

static int PiPyOS_bcm_framebuffer_printf(const char *format, ...) {
    char msg[256];
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vsnprintf(msg, sizeof(msg)-1, format, ap);
    va_end(ap);
    PiPyOS_bcm_framebuffer_putstring(msg, -1);
    return ret;
}

#define TRACE_ENTRY(format, function, ...) PiPyOS_bcm_framebuffer_printf(format "\n", function, __VA_ARGS__)
#define TRACE_EXIT(format, function, ...) PiPyOS_bcm_framebuffer_printf(format "\n", function, __VA_ARGS__)

#else

#define TRACE_ENTRY(format, function, ...)
#define TRACE_EXIT(format, function, ...)

#endif


int clock_getres(clockid_t clk_id, struct timespec *res) { 
    res->tv_sec=0;
    res->tv_nsec=1000; // 1us resolution of SYSTIMER
    
    return 0; 

}

int clock_gettime(clockid_t clk_id, struct timespec *tp) { 
    unsigned long low, high, high2;
    unsigned long long microseconds;

    lldiv_t q;

    // Read high and low 32-bit timer registers; ensure no low roll-over in between
    do {
        high = SYSTIMER_CHI;
        low = SYSTIMER_CLO;
        high2 = SYSTIMER_CHI;
    } while (high!=high2);
        
    microseconds = (((unsigned long long) high)<<32) | low;

    q = lldiv(microseconds, 1000000);
    
    tp->tv_sec = q.quot;
    tp->tv_nsec = q.rem * 1000; // convert us to ns

    return 0;
    
}

int _gettimeofday(struct timeval *tv, struct timezone *tz) {
    return 0; //1970
}

/* Only implement thread sleep, no fs's supported */

int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout) {
    unsigned long milliseconds;
 
    if (nfds!=0) {
        errno = ENOSYS;
        return -1;
    }
    
    milliseconds = (timeout->tv_usec / 1000) + (timeout->tv_sec * 1000);
    chThdSleepMilliseconds(milliseconds);
    return 0;
}

clock_t _times(struct tms *buf) {
    errno = ENOSYS;
    return -1;
}


void _exit(int status) { 
    printf("EXIT %d\n", status); 
    for(;;);
}

int _kill(int pid, int sig) { 
    printf("KILL %d, %d\n", pid, sig); 
    return -1;
}


void * _sbrk(int increment) { 
    static char *heap_end=0;
    //printf can not be used, so use chprintf
    //chprintf((BaseSequentialStream *)&SD1, "SBRK\n");
    if (!heap_end) heap_end = (char*)8388608; //We claim all above 8MB. TODO: from linker script

    char *prev_heap_end;

    // TODO: check mem full
    prev_heap_end = heap_end;

    heap_end += increment;
    return prev_heap_end;
}    
    


int _getpid(void) { 
    return 1; // Define PID of our Python process to be 1 (=init)
}

int getppid(void) {
    return 0;
}

//TODO general: set errno upon error

int _fstat(int fd, void *buf) {
    //called before printf can be used, so use chprintf
//    chprintf((BaseSequentialStream *)&SD1, "FSTAT\n");
    errno=ENOSYS;
    return -1;
}


typedef enum {
    HANDLER_NONE=0, // closed file
    HANDLER_FAT,     // most commonly used --> early in the list
    HANDLER_INITFS,
    HANDLER_FRAMEBUFFER,
    HANDLER_SERIAL,
    HANDLER_ROOT,
    HANDLER_CNT    // total number of handers
} handler_t;

char *PiPyOS_filehandler_names[]={NULL, "/sd", "/boot", "/fb", "/serial", NULL};

typedef struct {
    handler_t handler;
    union {
        initfs_openfile_t initfs;
        FIL ff_fil;
        BaseSequentialStream* serialstream;
    };
} openfile_t;



#define MAX_OPEN_FILES 100
openfile_t openfiles[MAX_OPEN_FILES]={0};


static char *cwd = "/boot";

char *getcwd(char *buf, size_t size) {
    if (!buf) {  
        errno = EINVAL;
        return NULL;
    }
    
    if (size<strlen(cwd)+1) {
        errno = ERANGE;
        return NULL;
    }
    strcpy(buf, cwd);
    return buf;


}

/* Convert FF library FRESULT enum to errno
 * conversion table
 */
 
static int ff_result_errnos[] = {
    0,      /* FR_OK = 0,               (0) Succeeded */
    EIO,    /* FR_DISK_ERR,             (1) A hard error occurred in the low level disk I/O layer */
    EFAULT, /* FR_INT_ERR,              (2) Assertion failed */
    EIO,    /* FR_NOT_READY,            (3) The physical drive cannot work */
    ENOENT, /* FR_NO_FILE,              (4) Could not find the file */
    ENOENT, /* FR_NO_PATH,              (5) Could not find the path */
    ENOENT, /* FR_INVALID_NAME,         (6) The path name format is invalid */
    EACCES, /* FR_DENIED,               (7) Access denied due to prohibited access or directory full */
    EEXIST, /* FR_EXIST,                (8) Access denied due to prohibited access */
    EINVAL, /* FR_INVALID_OBJECT,       (9) The file/directory object is invalid */
    EACCES, /* FR_WRITE_PROTECTED,      (10) The physical drive is write protected */
    ENODEV, /* FR_INVALID_DRIVE,        (11) The logical drive number is invalid */
    EINVAL, /* FR_NOT_ENABLED,          (12) The volume has no work area */
    ENODEV, /* FR_NO_FILESYSTEM,        (13) There is no valid FAT volume */
    EINTR,  /* FR_MKFS_ABORTED,         (14) The f_mkfs() aborted due to any problem */
    EBUSY,  /* FR_TIMEOUT,              (15) Could not get a grant to access the volume within defined period */
    EBUSY,  /* FR_LOCKED,               (16) The operation is rejected according to the file sharing policy */
    ENOMEM, /* FR_NOT_ENOUGH_CORE,      (17) LFN working buffer could not be allocated */
    EMFILE, /* FR_TOO_MANY_OPEN_FILES,  (18) Number of open files > FF_FS_LOCK */
    EINVAL, /* FR_INVALID_PARAMETER     (19) Given parameter is invalid */
};

/* Convert FF library FRESULT enum to errno
 * if fresult==FR_OK (OK), return 0
 * if fresult!=FR_OK (error), set errno appropriately and return -1
 */
static int ff_result_to_errno(int fresult) {
    if (fresult==FR_OK) return 0;
    if (fresult < (int)(sizeof(ff_result_errnos) / sizeof(ff_result_errnos[0]))) {
        errno = ff_result_errnos[fresult];
    } else {
        errno = EINVAL;
    }
    return -1;
}

/* For a given full path (i.e. starting with /), determine which filesystem
 * handler to use to access it. Also, if fspath!=NULL, it is pointed to the
 * start of the path relative to the root of that filesystem, e.g. for
 * pathname = "/sd/file.txt", fspath would be made to point to "/file.txt"
 *
 * Returns the handler id, or HANDLER_NONE (=0) when the pathname does not
 * match any filesystem
 */
static handler_t handler_for_path(const char* pathname, const char** fspath) {
    unsigned int handlerlen, pathlen;
    char *handlername;
    int handler;
    pathlen = strlen(pathname);
    
    for(handler=1; handler < HANDLER_CNT;handler++) {
        handlername = PiPyOS_filehandler_names[handler];
        handlerlen = strlen(handlername);
        if (handlername && strncmp(pathname, handlername, handlerlen)==0) { // path starts with handlername
            if (pathlen == handlerlen) break;     // exact match
            if (pathname[handlerlen]=='/') break; // must have a / after the handlername
        }
    }
    if (handler == HANDLER_CNT) {
        if (strcmp(pathname, "/")==0) { // only for /, not for subpaths of it, to be able to list root directory
            handler = HANDLER_ROOT;
            handlerlen = 1;
        } else {
            return HANDLER_NONE;
        }
    }
    
    if (fspath) {
        *fspath = pathname + handlerlen;
    }
    
    return handler;
}

void os_init_stdio(void) {
    int i;
    for(i=0;i<3;i++) {
        openfiles[i].handler = HANDLER_SERIAL;
        openfiles[i].serialstream = (BaseSequentialStream *)&SD1;
    }
}



/* File open switch logic
 *
 * Determines the filesystem handler for the given pathname, and calls the
 * appropriate xxx_open function which does the actual opening of the file for
 * that filesystem.
 *
 * Each xxx_open should return 0 on success. On failure, it should set errno and
 * return -1.
 *
 */

int _open(const char *pathname, int flags ) {
    int fd = -1;
    int error;
    int i;
    const char *fspathname;
    handler_t handler;
    
    TRACE_ENTRY("%s(%s, %d)", "open", pathname, flags);
    
    handler = handler_for_path(pathname, &fspathname);
    
    
    //TODO: check flags
 
    if (!handler) {
        errno = ENOENT;
        return -1;
    }
    
    // find file id to return
    for(i=0;i<MAX_OPEN_FILES;i++) {
        if (!openfiles[i].handler) {
            fd = i;
            break;
        }
    }

    if (fd==-1) {
        errno = EMFILE; // too many open files
        return -1;
    }

    switch (handler) {
        case HANDLER_FAT:
            error = ff_result_to_errno(f_open(&openfiles[fd].ff_fil, fspathname, FA_READ));
            break;
        case HANDLER_INITFS:
            error = PiPyOS_initfs_open(&openfiles[fd].initfs, fspathname, flags);
            break;
            
        case HANDLER_FRAMEBUFFER:
            error = 0;
            break;
        case HANDLER_SERIAL:
            if (strcmp(fspathname, "/0")==0) {
                error = 0;
                openfiles[fd].serialstream = (BaseSequentialStream *)&SD1;
            } else {
                error = -1;
                errno = ENOENT;
            }
            break;
        case HANDLER_ROOT:
            errno = EISDIR;
            error = -1;
            break;
        default:
            errno = ENOENT;
            error = -1;
        
    }
    
    if (!error) {
        openfiles[fd].handler = handler;
    } else fd = -1;

    TRACE_EXIT("%s returned %d", "open", fd);

    return fd;
}




int _close(int fd) {
    int error;
    TRACE_ENTRY("%s(%s)", "close", fd);
    
    if (fd<=2) { // Do not allow to close stdio
        error = EPERM;
        return -1;
    } else {    
        switch(openfiles[fd].handler) {
            case HANDLER_FAT:
                error = ff_result_to_errno(f_close(&openfiles[fd].ff_fil));
                break;
            default:
                error = 0;
        }
        openfiles[fd].handler=0;//Mark as closed even if there were errors
    }
    
    TRACE_EXIT("%s returned %d", "close", fd);

    return error; 
}


ssize_t _write(int fd, const void *buf, size_t count) {
    ssize_t ret;

    TRACE_ENTRY("%s(%d,<buffer>,%d)", "write", fd, count);

    switch(openfiles[fd].handler) {
        case HANDLER_FRAMEBUFFER:
            PiPyOS_bcm_framebuffer_putstring((const char *)buf, count);
            ret = count;
            break;
        case HANDLER_SERIAL:
            chSequentialStreamWrite(openfiles[fd].serialstream, buf, count);
            ret = count;
            break;
        default:
            errno = EBADF; // no write to all other file handlers including HANDLER_NONE (closed file)
            ret = -1;
    }
    
    TRACE_EXIT("%s returned %d", "write", ret);
    return ret;
}


ssize_t _read(int fd, void *buf, size_t count) { 
    int error;
    int ret;
    unsigned int bytesread;
    
    TRACE_ENTRY("%s(%d,<buffer>,%d)", "read", fd, count);

    switch(openfiles[fd].handler) {
        case HANDLER_NONE:
            errno = EBADF;
            ret = -1;
            break;
        case HANDLER_FAT:
            error = f_read(&openfiles[fd].ff_fil, buf, count, &bytesread);
            if (error) {
                ret = ff_result_to_errno(error);
            } else {
                ret = bytesread;
            }
            break;
        case HANDLER_INITFS:
            ret = PiPyOS_initfs_read(&openfiles[fd].initfs, buf, count);
            break;
        case HANDLER_SERIAL:
            if (count ==0) return 0;
            while(chSequentialStreamRead(openfiles[fd].serialstream, buf, 1)==0);
            ret = 1;
            break;
        default:
            errno = EBADF; // no read from all other file handlers including HANDLER_NONE (closed file)
            ret = -1;
    }
    
    TRACE_EXIT("%s returned %d", "read", ret);

    return ret;

}

DIR *opendir(const char *pathname) {
    DIR *ret = NULL;
    handler_t handler;
    int error;
    const char *fspathname;    
    handler = handler_for_path(pathname, &fspathname);
    
    //printf("OPENDIR %s handler = %d = %s\n", pathname, handler, fspathname);
    
    if (handler == HANDLER_NONE) {
        errno = ENOENT;    
        return NULL;
    }
    
    ret = malloc(sizeof(DIR));
    
    switch(handler) {
        case HANDLER_FAT:
            error = f_opendir(&ret->ff_dir, fspathname);
            if (error) {
                ff_result_to_errno(error);
            }
            break;
        case HANDLER_INITFS:
            error = PiPyOS_initfs_opendir(fspathname, ret);
            break;
        case HANDLER_ROOT:
            error = 0;
            ret->rootfs_idx = 1;
            break;
        default: // Other handlers are char devices
            errno = ENOTDIR;
            error = -1;
    }
    
    if (error) {
        free(ret);
        ret = NULL;
    } else {
        // Store handler so readdir can call the appropriate xxx_readdir
        ret->handler = handler;
    }    
    
    return ret;
}


/* Return the next item in a directory listing
 *
 * On end-of-dir, NULL is returned and errno is unchanged.
 * On error, NULL is returned and errno is set.
 *
 */
struct dirent *readdir(DIR *dirp) {

    FILINFO info;
    int error;
    struct dirent *ret; // Do NOT set value here, to make sure all code paths are covered

    switch(dirp->handler) {
        case HANDLER_FAT:
            error = f_readdir(&dirp->ff_dir, &info);
            if (error) {
                // Error
                ff_result_to_errno(error);
                ret = NULL;
            } else {
                if (info.fname[0] == 0) {
                    // End of directory
                    ret = NULL;
                } else {
                    strcpy(dirp->dirent.d_name, info.fname);
                    dirp->dirent.d_ino = 0; // No access to inode
                    ret = &dirp->dirent;
                }
            }
            break;
        case HANDLER_INITFS:
            // PiPyOS_initfs_readdir returns 0 on success, 1 on eof
            // On failure, -1 is returned and errno is set
            error = PiPyOS_initfs_readdir(dirp);
            if (error) {
                ret = NULL;
            } else {
                ret = &dirp->dirent;
            }
            break;
        case HANDLER_ROOT:
            if ((dirp->rootfs_idx < HANDLER_CNT) && (PiPyOS_filehandler_names[dirp->rootfs_idx])) {
                strcpy(dirp->dirent.d_name, PiPyOS_filehandler_names[dirp->rootfs_idx]);
                dirp->dirent.d_ino = dirp->rootfs_idx;
                ret = &dirp->dirent;
                dirp->rootfs_idx++;
            } else {
                ret = NULL;
            }
            break;
        default: // Should not happen
            errno = ENOTDIR;
            ret = NULL;
    }
    
    TRACE_EXIT("%s returned %d", "stat", ret);
    
    return ret;
    
}


int closedir(DIR *dirp) {
    switch(dirp->handler) {
        case HANDLER_FAT:
            f_closedir(&dirp->ff_dir);
            break;
    }
    free(dirp); 
    return 0;
}


int _isatty(int fd) { 
    int handler = openfiles[fd].handler;
    return (handler == HANDLER_SERIAL) || (handler == HANDLER_FRAMEBUFFER);
}

int _stat(const char *pathname, struct stat *buf) {

    handler_t handler;
    const char *fspathname;
    int error;
    FILINFO info;    

    TRACE_ENTRY("%s(%s, <buffer>)", "stat", pathname);

    handler = handler_for_path(pathname, &fspathname);

    if (!handler) {
        errno = ENOENT;
        return -1;
    }
    
    switch (handler) {
        case HANDLER_FAT:
            error = f_stat(fspathname, &info);
            if (error) {
                error = ff_result_to_errno(error);
            } else {
                memset(buf, 0, sizeof(*buf));
                
                if (info.fattrib & AM_DIR) {
                    buf->st_mode = S_IFDIR | S_IRUSR | S_IXUSR;
                } else {
                    buf->st_size = info.fsize;
                    buf->st_blocks = (info.fsize+511)/512;
                    buf->st_mode = S_IFREG | S_IRUSR;
                }
                buf->st_ino = 0; // no access to inode from FatFs lib
                buf->st_nlink = 1;
                buf->st_blksize = 512;
            
                error = 0;
            }
            break;
        case HANDLER_INITFS:
            error = PiPyOS_initfs_stat(fspathname, buf);
            break;
        default:
            errno = ENOENT;
            error = -1;
    }
    
    if (!error) buf->st_dev = handler;
    return error;

}

int lstat(const char *path, struct stat *buf) {
    return stat(path, buf); // Links are not supported; lstat = stat
}


int fcntl(int fd, int cmd, ...) {
    return 0; // fcntl is used by several functions in Python fileutils.c
}

/* Core Python uses dup to check if an fd is valid.
 */
int dup(int fd) {
    if (!openfiles[fd].handler) {
        errno = EBADF;
        return -1;
    }

    for(int i=0;i<MAX_OPEN_FILES;i++) {
        if (!openfiles[i].handler) {
            memcpy(&openfiles[i], &openfiles[fd], sizeof(openfiles[0]));
            return i;
        }
    }
    
    errno = EMFILE;
    return -1;

}


/*  
 *  Stubs; functionality could be implemented once required
 */

// Group / user ids: only one user: 0
uid_t getuid(void) { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void) { return 0; }
gid_t getegid(void) { return 0; }

char *ttyname(int fd) {  return NULL; }
void sig_ign(int code) { }
void sig_err(int code) { }
int _unlink(const char *pathname)                   { errno=ENOSYS; return -1; }
int _execve(const char *filename, 
    char *const argv[], char *const envp[])         { errno=ENOSYS; return -1; }
int execv(const char *path, char *const argv[])     { errno=ENOSYS; return -1; }
pid_t _fork(void)                                   { errno=ENOSYS; return -1; }
pid_t _wait(int *status)                            { errno=ENOSYS; return -1; }
int _link(const char *oldpath, const char *newpath) { errno=ENOSYS; return -1;}
int pipe(int pipefd[2])                             { errno=ENOSYS; return -1;}
int utime(const char *filename, 
    const struct utimbuf *times)                    { errno=ENOSYS; return -1;}
mode_t umask(mode_t mask)                           { errno=ENOSYS; return -1;}
int rmdir(const char *pathname)                     { errno=ENOSYS; return -1;}
int mkdir(const char *pathname, mode_t mode)        { errno=ENOSYS; return -1;}
int chdir(const char *path)                         { errno=ENOSYS; return -1;}
int chmod(const char *path, mode_t mode)            { errno=ENOSYS; return -1;}
int _lseek(int fd, int offset, int whence)          { errno=ENOSYS; return -1;}

