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

typedef struct {
    PyObject_HEAD LanguageIdentifier* identifier;
} LangIdObject;

static void langid_dealloc(LangIdObject* self);
static PyObject* langid_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int langid_init(LangIdObject* self, PyObject* args, PyObject* kwds);
static PyObject* langid_classify(LangIdObject* self, PyObject* args);
static PyObject* langid_rank(LangIdObject* self, PyObject* args);

static PyMethodDef LangIdObject_methods[] = {
    {"classify", (PyCFunction)langid_classify, METH_VARARGS,
     "Identify the language and confidence of a piece of text."},
    {"rank", (PyCFunction)langid_rank, METH_VARARGS, "Rank the confidences of the languages for a given text."},
    {NULL, NULL, 0, NULL} // Sentinel
};

static PyTypeObject LangIdType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "_langid.LangId",
    .tp_doc = PyDoc_STR("Off-the-shelf language identifier"),
    .tp_basicsize = sizeof(LangIdObject),
    .tp_itemsize = 0,
    .tp_new = langid_new,
    .tp_init = (initproc)langid_init,
    .tp_dealloc = (destructor)langid_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = LangIdObject_methods,
};

static struct PyModuleDef langidmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_langid",
    .m_doc = "This module provides an off-the-shelf language identifier.",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit__langid(void) {
    PyObject* m;
    if (PyType_Ready(&LangIdType) < 0)
        return NULL;

    m = PyModule_Create(&langidmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&LangIdType);
    if (PyModule_AddObject(m, "LangId", (PyObject*)&LangIdType) < 0) {
        Py_DECREF(&LangIdType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

static PyObject* langid_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    LangIdObject* self;
    self = (LangIdObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->identifier = NULL;
    }
    return (PyObject*)self;
}

static void langid_dealloc(LangIdObject* self) {
    if (self->identifier != NULL) {
        destroy_identifier(self->identifier);
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Initialize the LangIdObject with a LanguageIdentifier instance loaded from the model
static int langid_init(LangIdObject* self, PyObject* args, PyObject* kwds) {
    const char* model_path;
    if (!PyArg_ParseTuple(args, "s", &model_path)) {
        return -1;
    }

    // TODO: refactor load_identifier so it throughs error we can catch here
    self->identifier = load_identifier(model_path);
    if (self->identifier == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load LanguageIdentifier from model");
        return -1;
    }

    return 0;
}

/* langid.classify() Python method */
static PyObject* langid_classify(LangIdObject* self, PyObject* args) {
    const char* text;
    Py_ssize_t text_length;
    PyObject* result;

    if (!PyArg_ParseTuple(args, "s#", &text, &text_length))
        return NULL;

    LanguageConfidence language_confidence = classify(self->identifier, text, text_length);

    result = Py_BuildValue("(s,d)", language_confidence.language, language_confidence.confidence);

    return result;
}

/* langid.rank() Python method */
static PyObject* langid_rank(LangIdObject* self, PyObject* args) {
    const char* text;
    Py_ssize_t text_length;

    if (!PyArg_ParseTuple(args, "s#", &text, &text_length)) {
        return NULL;
    }

    LanguageConfidence* confidences =
        (LanguageConfidence*)malloc(self->identifier->num_langs * sizeof(LanguageConfidence));

    if (confidences == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    rank(self->identifier, text, text_length, confidences);

    PyObject* lang_conf_list = PyList_New(self->identifier->num_langs);

    if (lang_conf_list == NULL) {
        free(confidences);
        return NULL;
    }

    for (Py_ssize_t i = 0; i < self->identifier->num_langs; ++i) {
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
