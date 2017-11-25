#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <sys/types.h>

void _exit(int status) { 
    chprintf((BaseSequentialStream *)&SD1, "EXIT\r\n"); 
    for(;;);
}

int _kill(int pid, int sig) { 
    chprintf((BaseSequentialStream *)&SD1, "KILL\r\n"); 
    return -1;
}


void * _sbrk(int increment) { 
    static char *heap_end=0;
    
    chprintf((BaseSequentialStream *)&SD1, "SBRK\r\n"); 
    extern char* _end;
    if (!heap_end) heap_end = (char*)8388608;

    char *prev_heap_end;

    // TODO: check mem full
    prev_heap_end = heap_end;
    chprintf((BaseSequentialStream *)&SD1, "HEAP END = %x\r\n", (int) heap_end); 

        
    heap_end += increment;
    return prev_heap_end;
}    
    


int _getpid(void) { return 123; }

int _fstat(int fd, void *buf) {
    chprintf((BaseSequentialStream *)&SD1, "SBRK\r\n"); 
    return -1;
}

int _open(const char *pathname, int flags ) {
    chprintf((BaseSequentialStream *)&SD1, "OPEN %s\r\n", pathname); 
    return -1;
}
int _close(int fd) { return 0; }
int _isatty(int fd) { return fd<=2; }
int _lseek(int fd, int offset, int whence) {
    chprintf((BaseSequentialStream *)&SD1, "SEEK\r\n"); 
    return -1; 
}
ssize_t _write(int fd, const void *buf, size_t count) {
    chSequentialStreamWrite((BaseSequentialStream *)&SD1, buf, count);
    return count; 

}
ssize_t _read(int fd, const void *buf, size_t count) { 
    chprintf((BaseSequentialStream *)&SD1, "READ\r\n");
    return -1;
}

int dup(int fd) { 
    // TODO: implement real functionality if required
    // Core Python uses it only to check if fd is valid (stdin closed etc)
    return fd;
    
}


void sig_ign(int code) {
    chprintf((BaseSequentialStream *)&SD1, "SIGIGN\r\n");
}

void sig_err(int code) {
    chprintf((BaseSequentialStream *)&SD1, "SIGERR\r\n");

}
