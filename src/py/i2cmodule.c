#include "Python.h"
#include "hal.h"


#define I2C_START_DOC "start(channel, bitrate)"

#define NUM_I2C_PERIPHERALS 2

static I2CDriver *channels[NUM_I2C_PERIPHERALS] = {&I2C0, &I2C1};
static I2CConfig cfg[NUM_I2C_PERIPHERALS]; 

static PyObject *
i2c_start(PyObject *self, PyObject *args) {
    char *error = NULL;
    int channel, bitrate;
    I2CDriver *ch;
    
    if (!PyArg_ParseTuple(args, "ii", &channel, &bitrate)) {
        return NULL;
    }
    
    memset(&cfg, 0, sizeof(cfg));
    
    if ((channel != 0) && (channel !=1)) error = "Channel must be 0 or 1";
    if (bitrate <=0) error = "Bitrate must be > 0";
    if (error) {
        PyErr_SetString(PyExc_ValueError, error);
        return NULL;
    }
    
    cfg[channel].ic_speed = bitrate;

    ch = channels[channel];
    i2cAcquireBus(ch);
    i2cStart(ch, &cfg[channel]);
    i2cReleaseBus(ch);

    Py_RETURN_NONE;
}



#define I2C_TRANSFER_DOC \
"transfer(channel, address, txdata, rxcnt, timeout)\n" \
"\n" \
"address: 7-bit I2C address\n" \
"txdata: bytes buffer or None, in which case data is transmitted\n" \
"rxcnt:  number of bytes to be received\n" \
"timeout: timeout [s], float\n" \
"\n" \
"returns: received data : bytes\n"

static PyObject *
i2c_transfer(PyObject *self, PyObject *args) {
    int channel, address, rxcnt, txdatasize;
    unsigned char *txdata;
    char *error = NULL;
    float timeout;
    PyObject *rxbytes = NULL;
    systime_t timeout_val;
    msg_t result;
    I2CDriver *ch;
    
    if (!PyArg_ParseTuple(args, "iiz#if", &channel, &address, &txdata, &txdatasize, &rxcnt, &timeout)) {
        return NULL;
    }
    
    if ((channel != 0) && (channel !=1)) error = "channel must be 0 or 1";
    if (address<0 || address >127) error = "address should be 0<=channel<=127";
    if (rxcnt <0) error = "rxcnt should be >= 0";
    if (timeout<=0 || isnan(timeout) || isinf(timeout)) error = "timeout should be >0";
    if (error) {
        PyErr_SetString(PyExc_ValueError, error);
        return NULL;    
    }

    timeout_val = timeout * CH_FREQUENCY;
    
    rxbytes = PyBytes_FromStringAndSize(NULL, rxcnt);
    if (!rxbytes) return NULL;

    ch = channels[channel];
    i2cAcquireBus(ch);
    
    if (txdatasize > 0) {
        result = i2cMasterTransmitTimeout(ch, address, txdata, txdatasize, (unsigned char*)PyBytes_AS_STRING(rxbytes), rxcnt, timeout_val);
    } else {
        result = i2cMasterReceiveTimeout(ch, address, (unsigned char*)PyBytes_AS_STRING(rxbytes), rxcnt, timeout_val);
    }
    
    i2cReleaseBus(ch);
    
    switch(result) {
    case RDY_OK:
        return rxbytes;
    case RDY_RESET:
        PyErr_SetString(PyExc_IOError, "an I2C error occurred");
        break;
    case RDY_TIMEOUT:
        PyErr_SetString(PyExc_TimeoutError, "I2C operation timed out");
        break;
    }

    Py_DECREF(rxbytes);
    return NULL;
}

static PyMethodDef i2cMethods[] = {
    {"start",  i2c_start, METH_VARARGS, I2C_START_DOC},
    {"transfer", i2c_transfer, METH_VARARGS, I2C_TRANSFER_DOC},
    
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef i2cmodule = {
    PyModuleDef_HEAD_INIT,
    "i2c",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    i2cMethods
};


PyMODINIT_FUNC
PyInit_i2c(void) {
    
    return PyModule_Create(&i2cmodule); // could be NULL in case of error

}
