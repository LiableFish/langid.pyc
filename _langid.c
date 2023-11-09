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
#include "liblangid.h"
#include <Python.h>

/* Docstrings */
static char module_docstring[] = "This module provides an off-the-shelf language identifier.";
static char classify_docstring[] = "Identify the language of a piece of text.";

/* Available functions */
static PyObject* langid_classify(PyObject* self, PyObject* args);

/* Module specification */
static PyMethodDef module_methods[] = {{"classify", langid_classify, METH_VARARGS, classify_docstring},
                                       {NULL, NULL, 0, NULL}};

static struct PyModuleDef langid_module = {PyModuleDef_HEAD_INIT, "_langid", /* name of module */
                                           module_docstring,                 /* module documentation, may be NULL */
                                           -1, /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
                                           module_methods};

/* Global LanguageIdentifier instance */
LanguageIdentifier* identifier;

/* Initialize the module */
PyMODINIT_FUNC PyInit__langid(void) {
    PyObject* m = PyModule_Create(&langid_module);

    if (m == NULL)
        return NULL;

    identifier = get_default_identifier();

    return m;
}

static PyObject* langid_classify(PyObject* self, PyObject* args) {
    const char* bytes;
    Py_ssize_t numBytes;
    PyObject* ret;

    if (!PyArg_ParseTuple(args, "s#", &bytes, &numBytes))
        return NULL;

    /* Run our identifier */
    LanguageConfidence language_confidence = classify(identifier, bytes, numBytes);

    /* Build the output object */
    ret = Py_BuildValue("(s,d)", language_confidence.language, language_confidence.confidence);
    return ret;
}
