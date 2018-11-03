
#include "Python.h"
#include "ch.h"

#define DOC_GET_THREADS \
"get_threads()\n"\
"\n"\
"returns a list of tuples (name, state, priority, time, refcount, address, stack address)\n"\
"for each ChibiOS thread\n"

static const char *states[] = {THD_STATE_NAMES};

static PyObject *
chibios_get_threads(PyObject *self, PyObject *args){

    Thread *tp;
    PyObject *list = PyList_New(0);
    PyObject *item = NULL;
    
    if (!list) return NULL;
    
    tp = chRegFirstThread();
    //chprintf(chp, "    addr    stack prio refs     state time    name\r\n");
    // name state prio time refs addr stack 
    while(tp) {
        item = Py_BuildValue("ssIIIII", 
            tp->p_name, states[tp->p_state], (uint32_t)tp->p_prio, 
            (uint32_t)tp->p_time, (uint32_t)(tp->p_refs - 1), (uint32_t)tp, 
            (uint32_t)tp->p_ctx.r13);
        
        if (!item) goto error;
        if (PyList_Append(list, item)) goto error;
        
        tp = chRegNextThread(tp);
    }
    
    return list;
error:
    Py_XDECREF(item);    
    Py_XDECREF(list);   
    return NULL; 
}




static PyMethodDef _ChibiOSMethods[] = {
    {"get_threads",  chibios_get_threads, METH_NOARGS, DOC_GET_THREADS},
    
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef chibios_module = {
    PyModuleDef_HEAD_INIT,
    "chibios",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    _ChibiOSMethods
};


PyMODINIT_FUNC
PyInit_chibios(void) {

    PyObject *module = NULL;
    
    module = PyModule_Create(&chibios_module);
    if (!module) goto error;

    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}
