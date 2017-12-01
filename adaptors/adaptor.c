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


/*
  Provide the readline functionality provided in the Python function
  _readline.readline
  
  This function ignores sys_stdin and sys_stdout, they are assumed to be
  the default values.
*/
static PyObject *readlinecallback = NULL;
char *PyOS_StdioReadline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt);


char *PiPyOS_readline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt) {
    PyObject *module = NULL, *function = NULL, *line = NULL, *linebytes = NULL;
    char *line_cstr = NULL, *ret = NULL;
    Py_ssize_t len;

    if (!readlinecallback) {
    
        PyObject *module = PyImport_ImportModule("_readline");
        if (!module) goto error;
        
        PyObject *function = PyObject_GetAttrString(module, "readline");
        if (!function) goto error;
        
        if (!PyCallable_Check(function)) {
            PyErr_SetString(PyExc_TypeError, "_readline.readline must be callable");
            goto error;
        }
        Py_INCREF(function);
        readlinecallback = function;
    }
    
    line = PyObject_CallFunction(readlinecallback, "s", prompt);
    if (!line) goto error;
    
    linebytes = PyUnicode_AsEncodedString(line, "latin-1", "ignore");
    if (!linebytes) goto error;
    
    line_cstr = PyBytes_AsString(linebytes);
    if (!line_cstr) goto error;

    len = PyBytes_Size(linebytes);
    ret = PyMem_Malloc(len+1);
    if (!ret) {
        PyErr_NoMemory();
        goto error;
    }
    memcpy(ret, line_cstr, len+1);

    Py_XDECREF(linebytes);
    Py_XDECREF(line);
    Py_XDECREF(function);
    Py_XDECREF(module);


    return ret;

error:
    Py_XDECREF(linebytes);
    Py_XDECREF(line);
    Py_XDECREF(function);
    Py_XDECREF(module);
    printf("readline error\n");
    // Fall back to default ReadLine function for subsequent calls
    PyOS_ReadlineFunctionPointer = PyOS_StdioReadline;

    return NULL;

}

void PiPyOS_initreadline(void) {
    PyOS_ReadlineFunctionPointer = PiPyOS_readline;
}



void PyOS_InitInterrupts(void) { }

void PyOS_FiniInterrupts(void) { }

void PyOS_AfterFork() {} // Fork not supported

// TODO: catch Ctrl+C from console?
int PyOS_InterruptOccurred(void) { return 0; }

void _fini(void) {}
