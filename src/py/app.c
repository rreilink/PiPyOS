/* Weak implementations of the app* functions
 *
 */
#include "Python.h"

#pragma weak PyInit_app


PyMODINIT_FUNC
PyInit_app(void) {
    PyErr_SetString(PyExc_NotImplementedError, "app module is not present");
    return NULL;
}

#pragma weak app_init

void app_init(void) { }