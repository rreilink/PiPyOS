#include "Python.h"
#include "hal.h"


#define SPI_START_DOC "spi_start(channel, chipselect, frequency, cspol, cpol, cpha)"

static SPIConfig cfg; //TODO: the low level driver should copy the settings, or we should manage them

static PyObject *
spi_start(PyObject *self, PyObject *args) {
    char *error = NULL;
    int channel, cs, freq, cspol, cpol, cpha;
    int divider;
    if (!PyArg_ParseTuple(args, "iiippp", &channel, &cs, &freq, &cspol, &cpol, &cpha)) {
        return NULL;
    }
    
    memset(&cfg, 0, sizeof(cfg));
    
    if (channel != 0) error = "Only channel==0 is supported";
    if (cs<0 || cs>=1) error = "cs should be 0 or 1";
    if (freq <= 0) error = "frequency should be > 0";
    
    if (error) {
        PyErr_SetString(PyExc_ValueError, error);
        return NULL;
    }
    
    divider = 250000000 / freq;
    if (divider == 0) divider = 1;
    if (divider & 1) divider++; // Ensure even; round up
    
    //cfg.lossiEnabled = 0;
    cfg.chip_select = cs;
    cfg.clock_polarity = cpol;
    cfg.clock_phase = cpha;
    //cfg.clock_divider = divider;
    cfg.clock_frequency = freq;
    spiStart(&SPI0, &cfg);

    Py_RETURN_NONE;
}



static PyObject *
spi_select_unselect(PyObject *self, PyObject *args, int select) {
    int channel;
    if (!PyArg_ParseTuple(args, "i", &channel)) {
        return NULL;
    }
    if (channel != 0) {
        PyErr_SetString(PyExc_ValueError, "Only channel==0 is supported");
        return NULL;    
    }
    
    if (select) spiSelect(&SPI0); else spiUnselect(&SPI0);
    
    Py_RETURN_NONE;
}

#define SPI_SELECT_DOC "spi_select(channel)"

static PyObject *
spi_select(PyObject *self, PyObject *args) {
    return spi_select_unselect(self, args, 1);
}



#define SPI_UNSELECT_DOC "spi_unselect(channel)"

static PyObject *
spi_unselect(PyObject *self, PyObject *args) {
    return spi_select_unselect(self, args, 0);
}



#define SPI_EXCHANGE_DOC \
"spi_exchange(channel, txdata, rx)\n" \
"\n" \
"txdata: bytes buffer or None, in which case data is transmitted\n" \
"rx:     when txdata is None: number of bytes to be received\n" \
"        when txdata is not None: boolean whether or not to receive data\n"

static PyObject *
spi_exchange(PyObject *self, PyObject *args) {
    int channel, rx, txdatasize, size;
    char *txdata;
    PyObject *rxbytes = NULL;
    
    if (!PyArg_ParseTuple(args, "iz#i", &channel, &txdata, &txdatasize, &rx)) {
        return NULL;
    }
    
    if (channel != 0) {
        PyErr_SetString(PyExc_ValueError, "Only channel==0 is supported");
        return NULL;    
    }
    
    // Determine transaction size = len(txdata) if txdata is not None else rx
    
    if (txdata) {
        size = txdatasize;
    } else {
        if (rx<=0) {
            PyErr_SetString(PyExc_ValueError, "rx should be > 0 if txdata is None");
            return NULL;    
        }
        size = rx;
    }

    if (rx) {
        rxbytes = PyBytes_FromStringAndSize(NULL, size);
    }

    // Now use: rxbytes as boolean to check whether to receive, txdata as boolean wether to transmit, size as size
    
    if (txdata) {
        if (rxbytes) {
            // transmit and receive
            spiExchange(&SPI0, size, txdata, PyBytes_AS_STRING(rxbytes));
        } else {
            // only transmit
            spiSend(&SPI0, size, txdata);
        }
    } else {
        assert(rxbytes);
        // only receive
        spiReceive(&SPI0, size, PyBytes_AS_STRING(rxbytes));	
    }
    
    if (rxbytes) {
        return rxbytes;
    } else {
        Py_RETURN_NONE;
    }
}

static PyMethodDef SpiMethods[] = {
    {"start",  spi_start, METH_VARARGS, SPI_START_DOC},
    {"select", spi_select, METH_VARARGS, SPI_SELECT_DOC},
    {"unselect", spi_unselect, METH_VARARGS, SPI_UNSELECT_DOC},
    {"exchange", spi_exchange, METH_VARARGS, SPI_EXCHANGE_DOC},
    
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef spimodule = {
    PyModuleDef_HEAD_INIT,
    "spi",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    SpiMethods
};


PyMODINIT_FUNC
PyInit_spi(void) {

    PyObject *module = NULL;
    
    module = PyModule_Create(&spimodule);
    if (!module) goto error;

    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}
