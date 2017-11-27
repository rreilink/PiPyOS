#include "pyconfig.h"
#include "Python.h"
#include "adaptor.h"
#include <string.h>



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

void PyOS_InitInterrupts(void) { }

void PyOS_FiniInterrupts(void) { }

void PyOS_AfterFork() {} // Fork not supported

// TODO: catch Ctrl+C from console?
int PyOS_InterruptOccurred(void) { return 0; }

void _fini(void) {}
