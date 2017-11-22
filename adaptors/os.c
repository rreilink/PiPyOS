//
// main.c
//
#include <uspienv.h>
#include <uspi.h>
#include <uspios.h>
#include <uspienv/util.h>
#include <uspienv/macros.h>
#include <uspienv/types.h>


static const char FromSample[] = "sample";




void _exit(int status) { LogWrite (FromSample, LOG_ERROR, "EXIT"); for(;;); }
int _kill(int pid, int sig) { LogWrite (FromSample, LOG_ERROR, "KILL"); for(;;); }


void * _sbrk(int increment) { 
    static char *heap_end=0;
    
    LogWrite (FromSample, LOG_ERROR, "SBRK %d", increment); 
    extern char* _end;
    if (!heap_end) heap_end = _end;
    // TODO: define max for heap

    char *prev_heap_end;

    // TODO: check mem full
    prev_heap_end = heap_end;
    LogWrite (FromSample, LOG_ERROR, "HEAP END= %x", (int)heap_end); 
        
    heap_end += increment;
    return prev_heap_end;
}    
    


int _getpid(void) { return 123; }
int _fstat(int fd, void *buf) { LogWrite (FromSample, LOG_ERROR, "FSTAT"); return -1; }
int _open(const char *pathname, int flags ) { LogWrite (FromSample, LOG_ERROR, "OPEN"); return -1; }
int _close(int fd) { return 0; }
int _isatty(int fd) { return fd<=2; }
int _lseek(int fd, int offset, int whence) { LogWrite (FromSample, LOG_ERROR, "SEEK"); return -1; }
ssize_t _write(int fd, const void *buf, size_t count) {
    ScreenDeviceWrite(USPiEnvGetScreen(), buf, count);
    return count; 

}
ssize_t _read(int fd, const void *buf, size_t count) { LogWrite (FromSample, LOG_ERROR, "READ"); return -1; }

int dup(int fd) { 
    // TODO: implement real functionality if required
    // Core Python uses it only to check if fd is valid (stdin closed etc)
    return fd;
    
}

void initmsg(void) {
    LogWrite (FromSample, LOG_ERROR, "Hello, world");
}

void sig_ign(int code) {
    LogWrite (FromSample, LOG_ERROR, "SIGIGN");
}

void sig_err(int code) {
    LogWrite (FromSample, LOG_ERROR, "SIGERR");

}
