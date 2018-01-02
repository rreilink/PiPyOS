/* -*- C -*- ***********************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Module configuration */

/* !!! !!! !!! This file is edited by the makesetup script !!! !!! !!! */

/* This file contains the table of built-in modules.
   See create_builtin() in import.c. */

#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PyObject* PyInit_posix(void);
extern PyObject* PyInit__weakref(void);
extern PyObject* PyInit__io(void);
extern PyObject* PyInit_zipimport(void);
extern PyObject* PyInit__codecs(void);
extern PyObject* PyInit_errno(void);
extern PyObject* PyInit__struct(void);
extern PyObject* PyInit_math(void);
extern PyObject* PyInit_itertools(void);
extern PyObject* PyInit_time(void);
extern PyObject* PyInit__functools(void);
extern PyObject* PyInit__rpi(void);
extern PyObject* PyInit_app(void);

/* -- ADDMODULE MARKER 1 -- */

extern PyObject* PyMarshal_Init(void);
extern PyObject* PyInit_imp(void);
extern PyObject* PyInit_gc(void);
extern PyObject* PyInit__ast(void);
extern PyObject* _PyWarnings_Init(void);
extern PyObject* PyInit__string(void);

struct _inittab _PyImport_Inittab[] = {

{"posix", PyInit_posix},
{"_weakref", PyInit__weakref},
{"_io", PyInit__io},
{"zipimport", PyInit_zipimport},
{"_codecs", PyInit__codecs},
{"errno", PyInit_errno},
{"_struct", PyInit__struct},
{"math", PyInit_math},
{"itertools", PyInit_itertools},
{"time", PyInit_time},
{"_functools", PyInit__functools},
{"_rpi", PyInit__rpi},
{"app", PyInit_app},
/* -- ADDMODULE MARKER 2 -- */

    /* This module lives in marshal.c */
    {"marshal", PyMarshal_Init},

    /* This lives in import.c */
    {"_imp", PyInit_imp},

    /* This lives in Python/Python-ast.c */
    {"_ast", PyInit__ast},

    /* These entries are here for sys.builtin_module_names */
    {"builtins", NULL},
    {"sys", NULL},

    /* This lives in gcmodule.c */
    {"gc", PyInit_gc},

    /* This lives in _warnings.c */
    {"_warnings", _PyWarnings_Init},

    /* This lives in Objects/unicodeobject.c */
    {"_string", PyInit__string},

    /* Sentinel */
    {0, 0}
};


#ifdef __cplusplus
}
#endif
