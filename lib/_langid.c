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
    PyObject* nb_classes;      // Python list of strings
    PyObject* nb_classes_mask; // Python list of booleans
} LangIdObject;

static void LangId_dealloc(LangIdObject* self);
static PyObject* LangId_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int LangId_init(LangIdObject* self, PyObject* args, PyObject* kwds);
static PyObject* LangId_get_nb_classes(LangIdObject* self, void* closure);
static PyObject* LangId_get_nb_classes_mask(LangIdObject* self, void* closure);
static PyObject* LangId_classify(LangIdObject* self, PyObject* args);
static PyObject* LangId_rank(LangIdObject* self, PyObject* args);
static PyObject* LangId_set_languages(LangIdObject* self, PyObject* args);

// TODO: add module level methods (or maybe in python code and not here?)
static PyMethodDef LangIdObject_methods[] = {
    {"classify", (PyCFunction)LangId_classify, METH_VARARGS,
     "Identify the language and confidence of a piece of text."},
    {"rank", (PyCFunction)LangId_rank, METH_VARARGS, "Rank the confidences of the languages for a given text."},
    {"set_languages", (PyCFunction)LangId_set_languages, METH_VARARGS, "Set languages to classify from."},
    {NULL} // Sentinel
};

static PyGetSetDef LangId_getseters[] = {
    {"nb_classes", (getter)LangId_get_nb_classes, NULL, "List of nb_classes", NULL},
    {"nb_classes_mask", (getter)LangId_get_nb_classes_mask, NULL, "Mask of supported languages", NULL},
    {NULL} // Sentinel
};

static PyTypeObject LangIdType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "_langid.LangId",
    .tp_doc = PyDoc_STR("Off-the-shelf language identifier"),
    .tp_basicsize = sizeof(LangIdObject),
    .tp_itemsize = 0,
    .tp_new = LangId_new,
    .tp_init = (initproc)LangId_init,
    .tp_dealloc = (destructor)LangId_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = LangIdObject_methods,
    .tp_getset = LangId_getseters,
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

static PyObject* LangId_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    LangIdObject* self;
    self = (LangIdObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->identifier = NULL;

        self->nb_classes = PyList_New(0);
        if (self->nb_classes == NULL) {
            Py_DECREF(self);
            return NULL;
        }

        self->nb_classes_mask = PyList_New(0);
        if (self->nb_classes_mask == NULL) {
            Py_DECREF(self->nb_classes);
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject*)self;
}

static void LangId_dealloc(LangIdObject* self) {
    if (self->identifier != NULL) {
        destroy_identifier(self->identifier);
    }
    Py_XDECREF(self->nb_classes);
    Py_XDECREF(self->nb_classes_mask);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Initialize the LangIdObject with a LanguageIdentifier instance loaded from the model
static int LangId_init(LangIdObject* self, PyObject* args, PyObject* kwds) {
    const char* model_path;
    if (!PyArg_ParseTuple(args, "s", &model_path)) {
        return -1;
    }

    self->identifier = load_identifier(model_path);
    if (self->identifier == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load LanguageIdentifier from model path");
        return -1;
    }

    // Clear existing lists
    PyList_SetSlice(self->nb_classes, 0, PyList_Size(self->nb_classes), NULL);
    PyList_SetSlice(self->nb_classes_mask, 0, PyList_Size(self->nb_classes_mask), NULL);

    // Populate nb_classes
    for (size_t i = 0; i < self->identifier->num_langs; ++i) {
        PyObject* lang = PyUnicode_FromString((*self->identifier->nb_classes)[i]);
        if (lang == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to create Python string from language code");
            return -1;
        }
        PyList_Append(self->nb_classes, lang);
        Py_DECREF(lang); // The list now owns the reference
    }

    return 0;
}

static PyObject* LangId_get_nb_classes(LangIdObject* self, void* closure) {
    Py_INCREF(self->nb_classes);
    return self->nb_classes;
}

static PyObject* LangId_get_nb_classes_mask(LangIdObject* self, void* closure) {
    PyList_SetSlice(self->nb_classes_mask, 0, PyList_Size(self->nb_classes_mask), NULL);

    for (size_t i = 0; i < self->identifier->num_langs; ++i) {
        PyObject* value = self->identifier->nb_classes_mask[i] ? Py_True : Py_False;
        Py_INCREF(value); // Increment ref count as PyList_Append will steal a reference
        PyList_Append(self->nb_classes_mask, value);
        Py_DECREF(value); // Decrement ref count as we own this reference
    }

    Py_INCREF(self->nb_classes_mask);
    return self->nb_classes_mask;
}

/* langid.classify() Python method */
static PyObject* LangId_classify(LangIdObject* self, PyObject* args) {
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
static PyObject* LangId_rank(LangIdObject* self, PyObject* args) {
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

/* langid.set_languages() Python method */
static PyObject* LangId_set_languages(LangIdObject* self, PyObject* args) {
    PyObject* lang_list;
    if (!PyArg_ParseTuple(args, "|O", &lang_list)) {
        return NULL;
    }
    
    if (lang_list == Py_None) {
        set_languages(self->identifier, NULL, 0);
        Py_RETURN_NONE;
    }

    if (!PyList_Check(lang_list)) {
        PyErr_SetString(PyExc_TypeError, "Argument must be a list or None.");
        return NULL;
    }

    Py_ssize_t num_langs = PyList_Size(lang_list);
    const char* langs[num_langs];

    for (Py_ssize_t i = 0; i < num_langs; ++i) {
        PyObject* lang_item = PyList_GetItem(lang_list, i);
        if (!PyUnicode_Check(lang_item)) {
            PyErr_SetString(PyExc_TypeError, "All items in the language list must be strings.");
            return NULL;
        }
        langs[i] = PyUnicode_AsUTF8(lang_item);
    }

    int result = set_languages(self->identifier, langs, num_langs);
    if (result != 0) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to set languages in LanguageIdentifier.");
        return NULL;
    }

    Py_RETURN_NONE;
}