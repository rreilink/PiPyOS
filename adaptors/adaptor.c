#include "pyconfig.h"
#include "Python.h"
#include "adaptor.h"
#include <string.h>

int clock_getres(clockid_t clk_id, struct timespec *res) { return -1; }
int clock_gettime(clockid_t clk_id, struct timespec *tp) { return -1; }
int clock_settime(clockid_t clk_id, const struct timespec *tp) { return -1;}

int fileno(FILE *stream) { return -1;}
int isatty(int fd) { return -1;}
int fstat(int fd, struct stat *buf) {return -1;}
int stat(const char *path, struct stat *buf) {return -1;}


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



int open(const char *pathname, int flags) {return -1;}

void PyOS_InitInterrupts(void) { }

void PyOS_FiniInterrupts(void) { }

// TODO: catch Ctrl+C from console?
int PyOS_InterruptOccurred(void) { return 0; }

// From posixmodule.c
/*
    Return the file system path representation of the object.

    If the object is str or bytes, then allow it to pass through with
    an incremented refcount. If the object defines __fspath__(), then
    return the result of that method. All other types raise a TypeError.
*/
PyObject *
PyOS_FSPath(PyObject *path)
{
    /* For error message reasons, this function is manually inlined in
       path_converter(). */
    _Py_IDENTIFIER(__fspath__);
    PyObject *func = NULL;
    PyObject *path_repr = NULL;

    if (PyUnicode_Check(path) || PyBytes_Check(path)) {
        Py_INCREF(path);
        return path;
    }

    func = _PyObject_LookupSpecial(path, &PyId___fspath__);
    if (NULL == func) {
        return PyErr_Format(PyExc_TypeError,
                            "expected str, bytes or os.PathLike object, "
                            "not %.200s",
                            Py_TYPE(path)->tp_name);
    }

    path_repr = PyObject_CallFunctionObjArgs(func, NULL);
    Py_DECREF(func);
    if (NULL == path_repr) {
        return NULL;
    }

    if (!(PyUnicode_Check(path_repr) || PyBytes_Check(path_repr))) {
        PyErr_Format(PyExc_TypeError,
                     "expected %.200s.__fspath__() to return str or bytes, "
                     "not %.200s", Py_TYPE(path)->tp_name,
                     Py_TYPE(path_repr)->tp_name);
        Py_DECREF(path_repr);
        return NULL;
    }

    return path_repr;
}

void _fini(void) {}
