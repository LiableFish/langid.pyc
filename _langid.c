/*
 * Python binding to liblangid
 * Based on a tutorial by Dan Foreman-Mackey
 * http://dan.iel.fm/posts/python-c-extensions/
 * and on the implementation of chromium-compact-language-detector by Mike McCandless
 * https://code.google.com/p/chromium-compact-language-detector
 *
 * Marco Lui <saffsd@gmail.com>, September 2014
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "liblangid.h"

/* Docstrings */
static char module_docstring[] =
    "This module provides an off-the-shelf language identifier.";
static char identify_docstring[] =
    "Identify the language of a piece of text.";

/* Available functions */
static PyObject *langid_identify(PyObject *self, PyObject *args);

/* Module specification */
static PyMethodDef module_methods[] = {
    {"identify", langid_identify, METH_VARARGS, identify_docstring},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef langidmodule = {
    PyModuleDef_HEAD_INIT,
    "_langid",     /* name of module */
    module_docstring,  /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    module_methods
};

/* Global LanguageIdentifier instance */
LanguageIdentifier *identifier;

/* Initialize the module */
PyMODINIT_FUNC PyInit__langid(void)
{
    PyObject *m = PyModule_Create(&langidmodule);

    if (m == NULL)
        return NULL;

    identifier = get_default_identifier();

    return m;
}

static PyObject *langid_identify(PyObject *self, PyObject *args)
{
    const char *bytes;
    const char *lang;
    Py_ssize_t numBytes;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "s#", &bytes, &numBytes))
        return NULL;

    /* Run our identifier */
    lang = identify(identifier, bytes, numBytes);

    /* Build the output object */
    ret = Py_BuildValue("s", lang);
    return ret;
}
