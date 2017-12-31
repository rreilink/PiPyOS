#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <utime.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define DIR DIR_FF // ff.h defines the type DIR, but that is reserved for dirent.h. So define it to be DIR_FF
#include "ff.h"
#undef DIR

#include "initfs.h"

int clock_getres(clockid_t clk_id, struct timespec *res) { 
    res->tv_sec=0;
    res->tv_nsec=1000000; // 1ms; TODO: get from ChibiOS timer config
    
    return 0; 

}
int clock_gettime(clockid_t clk_id, struct timespec *tp) { 
    systime_t ticks; 
    ldiv_t q;

    // Get ChibiOS ticks counter, split to whole and fractional seconds
    
    ticks = chTimeNow(); // TODO: handle overflow
    q = ldiv(ticks, 1000);
    
    tp->tv_sec = q.quot;
    tp->tv_nsec = q.rem * 1000000;

    return 0;
    
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
    HANDLER_CNT    // total number of handers
} handler_t;

char *PiPyOS_filehandler_names[]={NULL, "/sd", "/boot", "/dev/fb", "/dev/serial0"};

typedef struct {
    handler_t handler;
    union {
        initfs_openfile_t initfs;
        FIL ff_fil;
    };
} openfile_t;



#define MAX_OPEN_FILES 100
openfile_t openfiles[MAX_OPEN_FILES]={0};

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
        if (strncmp(pathname, handlername, handlerlen)==0) { // path starts with handlername
            if (pathlen == handlerlen) break;     // exact match
            if (pathname[handlerlen]=='/') break; // must have a / after the handlername
        }
    }
    if (handler == HANDLER_CNT) {
        return HANDLER_NONE;
    }
    
    if (fspath) {
        *fspath = pathname + handlerlen;
    }
    
    return handler;
}


int _stat(const char *pathname, struct stat *buf) {

    handler_t handler;
    
    handler = handler_for_path(pathname, NULL);
    
    printf("STAT %s handler = %d\n", pathname, handler);
    if (!handler) {
        errno = ENOENT;
        return -1;
    }
    
    switch (handler) {
        case HANDLER_INITFS:
            return PiPyOS_initfs_stat(pathname, buf);
        default:
            errno = ENOENT;
            return -1;
    }

}

int lstat(const char *path, struct stat *buf) {
    return stat(path, buf); // Links are not supported; lstat = stat
}

int fcntl(int fd, int cmd, ...) {
    return 0; // TODO
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
    char *real;
    handler_t handler;
    
    handler = handler_for_path(pathname, &fspathname);
    
    //printf("OPEN %s handler = %d = %s\n", pathname, handler, fspathname);

    if (!handler) {
        errno = ENOENT;
        return -1;
    }
    
    // find file id to return (start with 3: 0-2 reserved for stdin/out/err)
    for(i=3;i<MAX_OPEN_FILES;i++) {
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
            error = PiPyOS_initfs_open(&openfiles[fd].initfs, pathname, flags);
            break;
        default:
            errno = ENOENT;
            error = -1;
        
    }
    
    if (!error) {
        openfiles[fd].handler = handler;
        printf("OPEN succes %d\n", fd);
        return fd;

    }

    return -1;
}




int _close(int fd) {
    switch(openfiles[fd].handler) {
        case HANDLER_FAT:
            break;
        default:
            ;
    }
    openfiles[fd].handler=0;
    return 0; 
}

int _isatty(int fd) { 
    //called before printf can be used, so don't use printf here!
    return fd<=2; 

}

int _lseek(int fd, int offset, int whence) {
    errno = ENOSYS;
    return -1; 
}


// TODO: this definition somewhere else
extern void PiPyOS_bcm_framebuffer_putstring(const char*, int);

ssize_t _write(int fd, const void *buf, size_t count) {
    PiPyOS_bcm_framebuffer_putstring((const char *)buf, count);
    chSequentialStreamWrite((BaseSequentialStream *)&SD1, buf, count);
    return count; 

}


// todo: catch Ctrl+C from console, even when _read is not called
int PyOS_InterruptOccurred(void) { 
    return 0;
}


ssize_t _read(int fd, void *buf, size_t count) { 
    int error;
    unsigned int bytesread;
    
    if (fd==0) {
        if (count ==0) return 0;
        while(chSequentialStreamRead((BaseSequentialStream *)&SD1, buf, 1)==0);
        return 1;
    
    }

    switch(openfiles[fd].handler) {
        case HANDLER_NONE:
            return -1;
        case HANDLER_FAT:
            error = f_read(&openfiles[fd].ff_fil, buf, count, &bytesread);
            if (error) {
                return ff_result_to_errno(error);
            } else {
                return bytesread;
            }
        case HANDLER_INITFS:
            return PiPyOS_initfs_read(&openfiles[fd].initfs, buf, count);
        default:
            errno = EACCES;
            return -1 ;
    }

}

int _unlink(const char *pathname) { printf("UNLINK\n"); errno=ENOSYS; return -1; }
int _execve(const char *filename, char *const argv[], char *const envp[]) { printf("EXECVE\n"); errno=ENOSYS; return -1; }
int execv(const char *path, char *const argv[]) { printf("EXECV\n"); errno=ENOSYS; return -1; }
pid_t _fork(void) { printf("FORK\n"); errno=ENOSYS; return -1; }
pid_t _wait(int *status) { printf("WAIT\n"); errno=ENOSYS; return -1; }
int _link(const char *oldpath, const char *newpath) { printf("LINK\n"); errno=ENOSYS; return -1;}

int pipe(int pipefd[2]) { printf("PIPE\n"); errno=ENOSYS; return -1;}

int utime(const char *filename, const struct utimbuf *times) { printf("UTIME\n"); errno=ENOSYS; return -1;}
mode_t umask(mode_t mask) { printf("UMASK\n"); errno=ENOSYS; return -1;}
int rmdir(const char *pathname) { printf("RMDIR\n"); errno=ENOSYS; return -1;}
int mkdir(const char *pathname, mode_t mode) { printf("MKDIR\n"); errno=ENOSYS; return -1;}
int chdir(const char *path) { printf("CHDIR\n"); errno=ENOSYS; return -1;}
int chmod(const char *path, mode_t mode) { printf("CHMOD\n"); errno=ENOSYS; return -1;}


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


// Group / user ids: only one user: 0
uid_t getuid(void) { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void) { return 0; }
gid_t getegid(void) { return 0; }

char *ttyname(int fd) {  return NULL; }



DIR *opendir(const char *pathname) {
    DIR *ret = NULL;
    handler_t handler;
    int error=-1;
    const char *fspathname;    
    handler = handler_for_path(pathname, &fspathname);
    
    printf("OPENDIR %s handler = %d = %s\n", pathname, handler, fspathname);
    
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
            error = PiPyOS_initfs_opendir(pathname, ret);
            break;
        default: // Other handlers are char devices
            errno = ENOTDIR;
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
        default:
            errno = ENOTDIR;
            ret = NULL;
    }
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



int dup(int fd) { 
//    printf("DUP\n");
    // TODO: implement real functionality if required
    // Core Python uses it only to check if fd is valid (stdin closed etc)
    return fd;
    
}


void sig_ign(int code) {
    (void) code;
    chprintf((BaseSequentialStream *)&SD1, "SIGIGN\r\n");
}

void sig_err(int code) {
    (void) code;
    chprintf((BaseSequentialStream *)&SD1, "SIGERR\r\n");

}
