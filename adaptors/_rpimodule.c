
#include "Python.h"

// TODO: this somewhere else
#define dmb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")

/*
  Expose the RPi peripherals map to Python
*/


static PyObject *
_rpi_peek(PyObject *self, PyObject *args)
{
    long address;
    long value;

    if (!PyArg_ParseTuple(args, "k", &address))
        return NULL;
    
    if ((address & 3) !=0) {
        PyErr_SetString(PyExc_ValueError, "address must be word-aligned");
        return NULL;
    }
    
    value = *((long *)address);
    
    return PyLong_FromLong(value);
}

static PyObject *
_rpi_poke(PyObject *self, PyObject *args)
{
    long address;
    long value;

    if (!PyArg_ParseTuple(args, "kk", &address, &value))
        return NULL;
    
    if ((address & 3) !=0) {
        PyErr_SetString(PyExc_ValueError, "address must be word-aligned");
        return NULL;
    }
    
    *((long *)address) = value;
    
    Py_RETURN_NONE;
}


static PyMethodDef _RpiMethods[] = {
    {"peek",  _rpi_peek, METH_VARARGS, "Read a memory address."},
    {"poke",  _rpi_poke, METH_VARARGS, "Write a memory address."},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef _rpimodule = {
    PyModuleDef_HEAD_INIT,
    "_rpi",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    _RpiMethods
};


PyMODINIT_FUNC
PyInit__rpi(void) {

    PyObject *module = NULL;
    
    module = PyModule_Create(&_rpimodule);
    if (!module) goto error;

    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}
