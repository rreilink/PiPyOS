#include "pyconfig.h"
#include "Python.h"
#include "adaptor.h"
#include "ch.h"
#include <string.h>

/*
  Provide the readline functionality provided in the Python function
  _readline.readline
  
  This function ignores sys_stdin and sys_stdout, they are assumed to be
  the default values.
*/
static PyObject *readlinecallback = NULL;
char *PyOS_StdioReadline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt);


char *PiPyOS_readline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt) {
    PyObject *module = NULL, *function = NULL, *line = NULL, *linebytes = NULL, *err;
    char *line_cstr = NULL, *ret = NULL;
    Py_ssize_t len;

    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();

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
    if (!line) {
        // Check if it is a KeyboardInterrupt, propagate that and keep the
        // current readline handler as-is
        err = PyErr_Occurred();
        if (err && PyErr_GivenExceptionMatches(err, PyExc_KeyboardInterrupt)) {
            ret = NULL;
            goto out;           
        }
    
        goto error;
    }
    
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

    goto out;

error:
    ret = NULL;
    printf("readline error\n");
    // Fall back to default ReadLine function for subsequent calls
    PyOS_ReadlineFunctionPointer = PyOS_StdioReadline;

out:
    Py_XDECREF(linebytes);
    Py_XDECREF(line);
    Py_XDECREF(function);
    Py_XDECREF(module);
    
    PyGILState_Release(gstate);
    return ret;

}

void PiPyOS_initreadline(void) {
    PyOS_ReadlineFunctionPointer = PiPyOS_readline;
}


extern CondVar PiPyOS_serial_interrupt_cv;
static volatile int interrupt_state = 0;

static WORKING_AREA(waHandleInterruptThread, 4096);

static int
checksignals_witharg(void * arg)
{
    return PyErr_CheckSignals();
}

static msg_t HandleInterruptThread(void *p) {
    Mutex mtx;
    
    chRegSetThreadName("interrupt_catcher");
    
    chMtxInit(&mtx);
    
    for(;;) {
        chMtxLock(&mtx);
        chCondWait (&PiPyOS_serial_interrupt_cv);
        chMtxUnlock();

        interrupt_state = 1;
        Py_AddPendingCall(checksignals_witharg, NULL);
    }
    return RDY_OK;
}

int PyOS_InterruptOccurred(void) { 
    int ret = interrupt_state;
    if (ret) interrupt_state = 0;
    return ret;
}

void PyOS_InitInterrupts(void) {
    /*
     * Create a thread to wait for the PiPyOS_serial_interrupt_cv condition variable
     *
     * It has a priority just above the priority of the Python thread(s) to ensure
     * it is activated when an interrupt occurs
     */
    chThdCreateStatic(waHandleInterruptThread, sizeof(waHandleInterruptThread), NORMALPRIO+1, HandleInterruptThread, NULL);
}

void PyOS_FiniInterrupts(void) { }

void PyErr_SetInterrupt(void) {
    interrupt_state = 1;
}

void PyOS_AfterFork() {} // Fork not supported

void _fini(void) {}


/* _tracemalloc.c cannot be built since it requires native thread-local storage,
 * which is not implemented. We don't need _tracemalloc anyway, but we need to
 * implement some stubs
 */
void _PyTraceMalloc_Init(void) { }
void _PyTraceMalloc_Fini(void) { }
void _PyMem_DumpTraceback(int fd, const void *ptr) { }

