
#include "Python.h"
#include "pi/bcmmailbox.h"

/*
  Expose the RPi peripherals map to Python
*/


static PyObject *
_rpi_peek(PyObject *self, PyObject *args)
{
    unsigned long address;
    unsigned long value;

    if (!PyArg_ParseTuple(args, "k", &address))
        return NULL;
    
    if ((address & 3) !=0) {
        PyErr_SetString(PyExc_ValueError, "address must be word-aligned");
        return NULL;
    }
    
    DataMemBarrier();
    value = *((unsigned long *)address);
    DataMemBarrier();
    
    return PyLong_FromLong(value);
}

static PyObject *
_rpi_poke(PyObject *self, PyObject *args)
{
    unsigned long address;
    unsigned long value;

    if (!PyArg_ParseTuple(args, "kk", &address, &value))
        return NULL;
    
    if ((address & 3) !=0) {
        PyErr_SetString(PyExc_ValueError, "address must be word-aligned");
        return NULL;
    }
    
    DataMemBarrier();
    *((long *)address) = value;
    DataMemBarrier();
    
    Py_RETURN_NONE;
}



static PyObject *
_rpi_get_property(PyObject *self, PyObject *args)
{
    PyObject *reply = NULL;
    int tagid, requestsize, responsesize;
    char *buffer;
    
    if (!PyArg_ParseTuple(args, "ii", &tagid, &requestsize)) {
        return NULL;
    }
    
    buffer = malloc(requestsize);
    if (!buffer) return PyErr_NoMemory();
    
    responsesize = PiPyOS_bcm_get_property_tag(tagid, buffer, requestsize);
    
    if (responsesize<0) {
        free(buffer);
        PyErr_SetString(PyExc_IOError, "failed to get property from VC");
        return NULL;
    }
    
    reply = PyBytes_FromStringAndSize(buffer, responsesize);
    
    // The following works in both cases: error or no error
    Py_XINCREF(reply);
    free(buffer);
    return reply;    
}

static PyObject *
_rpi_writereadmailbox(PyObject *self, PyObject *args)
{
    PyObject *reply = NULL;
    int channel;
    const char *data;
    char *buffer;
    char *buffer_aligned;
    int size;
    if (!PyArg_ParseTuple(args, "iy#", &channel, &data, &size)) {
        return NULL;
    }
    
    if (size & 0x3) {
        PyErr_SetString(PyExc_ValueError, "Data should be multiple of 4 bytes");
        return NULL;
    }
    
    if (channel<0 || channel>15) {
        PyErr_SetString(PyExc_ValueError, "Channel should be 0<=channel<15");
        return NULL;
    }
    
    // get 16-byte aligned buffer
    buffer = malloc(size + 15);
    if (!buffer) return PyErr_NoMemory();
    
    buffer_aligned = (void*)((((unsigned int)buffer)+15) & ~0xf);
    memcpy(buffer_aligned, data, size);
    
    PiPyOS_bcm_mailbox_write_read(channel, buffer_aligned);
        
    reply = PyBytes_FromStringAndSize(buffer_aligned, size);
    
    // The following works in both cases: error or no error
    Py_XINCREF(reply);
    free(buffer);
    return reply;
    

}

extern void PiPyOS_bcm_framebuffer_putstring(char*, int);

static PyObject *
_rpi_fbwrite(PyObject *self, PyObject *args) {
    char *data;
    int size;
    if (!PyArg_ParseTuple(args, "y#",  &data, &size)) {
        return NULL;
    }    
    PiPyOS_bcm_framebuffer_putstring(data, size);

    Py_RETURN_NONE;
}



static PyMethodDef _RpiMethods[] = {
    {"peek",  _rpi_peek, METH_VARARGS, "Read a memory address."},
    {"poke",  _rpi_poke, METH_VARARGS, "Write a memory address."},
    {"writereadmailbox", _rpi_writereadmailbox, METH_VARARGS, "Write to mailbox and read reply."},
    {"getproperty", _rpi_get_property, METH_VARARGS, "Get VideoCore property."},
    {"fb_write", _rpi_fbwrite, METH_VARARGS, "Write bytes to the framebuffer"},

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
