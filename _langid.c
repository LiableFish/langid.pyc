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
static char classify_docstring[] = "Identify the language and confidence of a piece of text.";
static char rank_docstring[] = "Rank the confidences of the languages for a given text.";

/* Available functions */
static PyObject* langid_classify(PyObject* self, PyObject* args);
static PyObject* langid_rank(PyObject* self, PyObject* args);
static void langid_free(void* module);

/* Module specification */
static PyMethodDef module_methods[] = {
    {"classify", langid_classify, METH_VARARGS, classify_docstring},
    {"rank", langid_rank, METH_VARARGS, rank_docstring},
    {NULL, NULL, 0, NULL} // Sentinel
};

/* Global LanguageIdentifier instance */
static LanguageIdentifier* identifier = NULL;

/* Initialize the module */
PyMODINIT_FUNC PyInit__langid(void) {
    static struct PyModuleDef module_def = {
        PyModuleDef_HEAD_INIT,
        "_langid",        // m_name
        module_docstring, // m_doc
        -1,               // m_size
        module_methods,   // m_methods
        NULL,             // m_reload
        NULL,             // m_traverse
        NULL,             // m_clear
        langid_free       // m_free
    };

    PyObject* m = PyModule_Create(&module_def);
    if (m == NULL)
        return NULL;

    // Initialize global LanguageIdentifier instance
    identifier = get_default_identifier();
    if (identifier == NULL) {
        Py_DECREF(m); // Decrease reference count to free module
        return NULL;
    }

    return m;
}

/* Module destructor */
static void langid_free(void* module) {
    if (identifier != NULL) {
        destroy_identifier(identifier);
        identifier = NULL;
    }
}

/* langid.classify() Python function definition */
static PyObject* langid_classify(PyObject* self, PyObject* args) {
    const char* text;
    Py_ssize_t text_length;
    PyObject* result;

    if (!PyArg_ParseTuple(args, "s#", &text, &text_length))
        return NULL;

    LanguageConfidence language_confidence = classify(identifier, text, text_length);

    result = Py_BuildValue("(s,d)", language_confidence.language, language_confidence.confidence);

    return result;
}

static PyObject* langid_rank(PyObject* self, PyObject* args) {
    const char* text;
    Py_ssize_t text_length;

    if (!PyArg_ParseTuple(args, "s#", &text, &text_length)) {
        return NULL;
    }

    LanguageConfidence* confidences = (LanguageConfidence*)malloc(identifier->num_langs * sizeof(LanguageConfidence));

    if (confidences == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    rank(identifier, text, text_length, confidences);

    PyObject* lang_conf_list = PyList_New(identifier->num_langs);

    if (lang_conf_list == NULL) {
        free(confidences);
        return NULL;
    }

    for (Py_ssize_t i = 0; i < identifier->num_langs; ++i) {
        PyObject* conf_tuple = Py_BuildValue("(s,d)", confidences[i].language, confidences[i].confidence);
        if (conf_tuple == NULL) {
            Py_DECREF(lang_conf_list);
            free(confidences);
            return NULL;
        }
        PyList_SET_ITEM(lang_conf_list, i, conf_tuple);
    }

    free(confidences);

    return lang_conf_list;
}