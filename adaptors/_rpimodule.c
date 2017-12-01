
#include "Python.h"

// TODO: this somewhere else
#define dmb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")

/*
  Expose the RPi peripherals map to Python
*/
static inline unsigned int _read(unsigned int address) {
    unsigned int ret;
    dmb();
    ret = *((unsigned long *)address);
    dmb();
    return ret;
}

static inline void _write(unsigned int address, unsigned int value) {
    dmb();
    *((unsigned long *)address) = value;
    dmb();
}

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
    
    dmb();
    value = *((unsigned long *)address);
    dmb();
    
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
    
    dmb();
    *((long *)address) = value;
    dmb();
    
    Py_RETURN_NONE;
}

#define MAILBOX_BASE		(0x20000000 + 0xB880)

#define MAILBOX0_READ  		   (MAILBOX_BASE + 0x00)
#define MAILBOX0_STATUS 	       (MAILBOX_BASE + 0x18)
#define MAILBOX_STATUS_EMPTY	   0x40000000
#define MAILBOX1_WRITE         (MAILBOX_BASE + 0x20)
#define MAILBOX1_STATUS 	       (MAILBOX_BASE + 0x38)
#define MAILBOX_STATUS_FULL	   0x80000000


//
// Cache control
//
#define InvalidateInstructionCache()	\
				__asm volatile ("mcr p15, 0, %0, c7, c5,  0" : : "r" (0) : "memory")
#define FlushPrefetchBuffer()	__asm volatile ("mcr p15, 0, %0, c7, c5,  4" : : "r" (0) : "memory")
#define FlushBranchTargetCache()	\
				__asm volatile ("mcr p15, 0, %0, c7, c5,  6" : : "r" (0) : "memory")
#define InvalidateDataCache()	__asm volatile ("mcr p15, 0, %0, c7, c6,  0" : : "r" (0) : "memory")
#define CleanDataCache()	__asm volatile ("mcr p15, 0, %0, c7, c10, 0" : : "r" (0) : "memory")

//
// Barriers
//
#define DataSyncBarrier()	__asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define DataMemBarrier() 	__asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")

#define InstructionSyncBarrier() FlushPrefetchBuffer()
#define InstructionMemBarrier()	FlushPrefetchBuffer()



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
    if (!buffer) return NULL;
    
    buffer_aligned = (void*)((((unsigned int)buffer)+15) & ~0xf);
    memcpy(buffer_aligned, data, size);
    
    
    CleanDataCache ();
    DataSyncBarrier ();
    
    while(_read(MAILBOX1_STATUS) & MAILBOX_STATUS_FULL);
    _write(MAILBOX1_WRITE, channel | 0x40000000 | (unsigned int) buffer_aligned);
    
    unsigned long read;
    do {
        while (_read(MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY);
        read = _read(MAILBOX0_READ);
    } while ((read & 0xf) != channel);
    
    InvalidateDataCache ();
    
    reply = PyBytes_FromStringAndSize(buffer_aligned, size);
    if (reply == NULL) {
        free(buffer);
        return NULL;
    }
    Py_INCREF(reply);
    free(buffer);
    return reply;
    

}


static PyMethodDef _RpiMethods[] = {
    {"peek",  _rpi_peek, METH_VARARGS, "Read a memory address."},
    {"poke",  _rpi_poke, METH_VARARGS, "Write a memory address."},
    {"writereadmailbox", _rpi_writereadmailbox, METH_VARARGS, "Write to mailbox and read reply."},

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
