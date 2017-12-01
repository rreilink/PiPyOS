#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent.h"
#include <stdio.h>
#include <stdlib.h>
#include <utime.h>
#include <string.h>
#include <fcntl.h>


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
    return -1;
}

typedef struct {
    unsigned int name_offs;
    unsigned int data_offs;
    unsigned int size;

} file_rec_t;

typedef struct {
    char *data;
    int pos;
    int size;

} openfile_t;

#define MAX_OPEN_FILES 20
openfile_t openfiles[MAX_OPEN_FILES]={0};


extern char _binary_initfs_bin_start;

int _stat(const char *path, struct stat *buf) {
    file_rec_t *r = (file_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;
    int i=0;

//    printf("STAT %s %d\n", path,(int)(&_binary_initfs_bin_start) );

    
    while(r[i].data_offs) {
        if (strcmp(&s[r[i].name_offs], path)==0) {
            if (r[i].size==0xffffffff) {
                buf->st_size = 0;
                buf->st_blocks = 0;
                buf->st_mode = S_IFDIR | S_IRUSR | S_IXUSR;
//                printf("Found dir\n");
                goto found;
            } else {
                buf->st_size = r[i].size;
                buf->st_blocks = (r[i].size+511)/512;
                buf->st_mode = S_IFREG | S_IRUSR;
//                printf("Found file\n");
                goto found;
            }
        
        }
        i++;
    }

    return -1;
found:

    buf->st_dev = 0;
    buf->st_ino = 0;

    buf->st_nlink = 1;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_rdev = 0;
    buf->st_atime = 0;
    buf->st_mtime = 0;
    buf->st_ctime = 0;
    buf->st_blksize = 512;


    return 0;
}

int lstat(const char *path, struct stat *buf) {
    return stat(path, buf); // Links are not supported; lstat = stat
}

int fcntl(int fd, int cmd, ...) {
//    printf("FCNTL\n");
    return 0; // TODO
}


int _open(const char *pathname, int flags ) {
    file_rec_t *r = (file_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;

    int i=0;

    int fd = -1;
 
//    printf("OPEN %s %d\n", pathname, (int)(&_binary_initfs_bin_start)); 
   
    // find file id to return (start with 3: 0-2 reserved for stdin/out/err)
    for(i=3;i<MAX_OPEN_FILES;i++) {
        if (!openfiles[i].data) {
            fd = i;
            break;
        }
    }

    if (fd==-1) {
        printf("TOO MANY OPEN FILES\n");
        return -1;
    }

    i = 0;
    while(r[i].data_offs) {
        if (strcmp(&s[r[i].name_offs], pathname)==0) {
            if (r[i].size==0xffffffff) {
//                printf("Open found dir\n");
                return -1;
            } else {
                openfiles[fd].data = &s[r[i].data_offs];
                openfiles[fd].pos = 0;
                openfiles[fd].size = r[i].size;
//                printf("open found file %d\n", fd);
                return fd;
            }
        
        }
        i++;
    }

    return -1;

}




int _close(int fd) { 
//    printf("CLOSE %d\n", fd);  
    openfiles[fd].data=0;
    return 0; 
}

int _isatty(int fd) { 
    //called before printf can be used, so don't use printf here!
    return fd<=2; 

}

int _lseek(int fd, int offset, int whence) {

//    printf("SEEK\n"); 
    return -1; 
}
ssize_t _write(int fd, const void *buf, size_t count) {
    chSequentialStreamWrite((BaseSequentialStream *)&SD1, buf, count);
    return count; 

}
ssize_t _read(int fd, void *buf, size_t count) { 
//    printf("READ\n");
    if (fd==0) {
        if (count ==0) return 0;
        while(chSequentialStreamRead((BaseSequentialStream *)&SD1, buf, 1)==0);
        return 1;
    
    }


    if (!openfiles[fd].data) {
        printf("READ from closed file %d\n", fd);
        return -1 ;
    }
    int n = openfiles[fd].size - openfiles[fd].pos;
    if (n>((int)count)) n = count;
    memcpy(buf, openfiles[fd].data + openfiles[fd].pos, n);
    openfiles[fd].pos += n;

    return n;
}

int _unlink(const char *pathname) { printf("UNLINK\n"); return -1; }
int _execve(const char *filename, char *const argv[], char *const envp[]) { printf("EXECVE\n"); return -1; }
int execv(const char *path, char *const argv[]) { printf("EXECV\n"); return -1; }
pid_t _fork(void) { printf("FORK\n"); return -1; }
pid_t _wait(int *status) { printf("WAIT\n"); return -1; }
int _link(const char *oldpath, const char *newpath) { printf("LINK\n"); return -1;}

int pipe(int pipefd[2]) { printf("PIPE\n"); return -1;}

int utime(const char *filename, const struct utimbuf *times) { printf("UTIME\n"); return -1;}
mode_t umask(mode_t mask) { printf("UMASK\n"); return -1;}
int rmdir(const char *pathname) { printf("RMDIR\n"); return -1;}
int mkdir(const char *pathname, mode_t mode) { printf("MKDIR\n"); return -1;}
int chdir(const char *path) { printf("CHDIR\n"); return -1;}
int chmod(const char *path, mode_t mode) { printf("CHMOD\n"); return -1;}

// Group / user ids: only one user: 0
uid_t getuid(void) { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void) { return 0; }
gid_t getegid(void) { return 0; }

char *ttyname(int fd) {  return NULL; }


//TODO: able to list root directory

DIR *opendir(const char *name) {
    file_rec_t *r = (file_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;

    int i=0;
    // TODO: strip trailing /
    while(r[i].data_offs) {
        if (strcmp(&s[r[i].name_offs], name)==0) {
            if (r[i].size==0xffffffff) {
                
                DIR *ret = malloc(sizeof(DIR));
                ret->idx = i;
                ret->idx_cur = i;
            
                return ret;              
              
                
            }
        }
        i++;
    }


    return NULL;
 

}

struct dirent *readdir(DIR *dirp) {
    file_rec_t *r = (file_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;
    char *parent_name = &s[r[dirp->idx].name_offs];

    // todo: readdir does not work for /
    
    int l = strlen(parent_name);

    // Loop over all remaining directory entries
    while (r[dirp->idx_cur].data_offs) {
        dirp->idx_cur++;
    
        char *current_name = &s[r[dirp->idx_cur].name_offs];
    
        // if this dir or file is not a subdir of the parent, we are done
        if (strncmp(current_name, parent_name, l)!=0) {
            break;
        }
        
        char *subpath = current_name + l; // the part below the parent
        if (*subpath=='/') { // should always be true, but check to be sure
            subpath++; // strip leading /
            if (!strchr(subpath, '/')) {
                // No / in subpath, so it is a direct child (no file in a sub-dir)
                            
                l = strlen(subpath);
                if (l >= NAME_MAX) {
                    return NULL; //filename too long
                }
                
                // Copy info of the current listing into dirp->dirent
                memcpy(dirp->dirent.d_name, subpath, l+1);
                dirp->dirent.d_ino = dirp->idx_cur;
                    
                return &dirp->dirent;                
                
            }
        } 
    }
//    printf("READDIR end\n");
    return NULL;
    
}


int closedir(DIR *dirp) {
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
