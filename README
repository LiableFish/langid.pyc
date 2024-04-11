
# langid.pyc
The modified version of `langid.c` with Python bindings -- a straightforward replacement for `langid.py`, offering the same features, but 200 times as faster.

## Installation
```bash
pip install langid-pyc
```

## Usage
### Basic
```python
from langid_pyc import (
    classify,
    rank,
)

classify("This is English text")
# ('en', 0.9999999239251556)

rank("This is English text")
# [('en', 0.9999999239251556),
#  ('la', 5.0319768731501096e-08),
#  ('br', 1.2684715402216825e-08),
#  ...]
```
### Language set constraint
```python
from langid_pyc import (
    classify,
    nb_classes,
    set_languages,
)

nb_classes()
# ['af',
#  'am',
#  'an',
#  ...]

len(nb_classes())
# 97

set_languages(["en", "ru"])
nb_classes()
# ['en', 'ru']

classify("This is English text")
# ('en', 1.0)

classify("А это текст на русском")
# ('ru', 1.0)

set_languages() # reset languages
len(nb_classes())
# 97
```
### `LanguageIdentifier` class
```python
from langid_pyc import LanguageIdentifier

identifier = LanguageIdentifier.from_modelpath("ldpy3.pmodel")  # default model

len(identifier.nb_classes)
# 97

identifier.classify("This is English text")
# ('en', 0.9999999239251556)

# identifier.rank(...)
# identifier.set_languages(...)
```

## How to build?
Install relevant `protobuf` packages
```bash
apt install protobuf-c-compiler libprotobuf-c-dev
```

Install dev python requirements
```bash
pip install -r requirements.txt
```

Run build
```
make build
```

See [Makefile](Makefile) for more details.

## How to add a new model?
Train a new model using `langid.py` package. You will get the model file as described [here](https://github.com/saffsd/langid.py/blob/master/langid/train/train.py#L283):
```python
# output the model
output_path = os.path.join(model_dir, 'your_new_model.model')
model = nb_ptc, nb_pc, nb_classes,tk_nextmove, tk_output
string = base64.b64encode(bz2.compress(cPickle.dumps(model)))
with open(output_path, 'w') as f:
f.write(string)
print "wrote model to %s (%d bytes)" % (output_path, len(string))
```

Move `your_new_model.model` to `models` dir and run
```bash
make your_new_model.model
```

Now you have `your_new_model.pmodel` file in the root which can be feed to `LanguageIdentifer.from_modelpath`

```python
from langid_pyc import LanguageIdentifier

your_new_identifier = LanguageIdentifier.from_modelpath("your_new_model.pmodel")
```

## Benchmark
Benchmark was calculated on Mac M2 Max, 32Gb RAM with python 3.8.18 and can be found [here](benchmark/benchmark.html).

TL;DR `langid.pyc` is ~200x faster than `langid.py` and ~1-1.5x faster than `pycld2`, especially on long texts.

# Original README

================
``langid.c`` readme
================

Introduction
------------
`langid.c` is an experimental implementation of the language identifier
described by [1] in pure C. It is largely based on the design of
`langid.py`[2], and uses `langid.py` to train models. 

Planned features
----------------
See TODO

Speed
-----

Initial comparisons against Google's cld2[3] suggest that `langid.c` is about
twice as fast.

    (langid.c) @mlui langid.c git:[master] wc -l wikifiles 
    28600 wikifiles
    (langid.c) @mlui langid.c git:[master] time cat wikifiles | ./compact_lang_det_batch > xxx
    cat wikifiles  0.00s user 0.00s system 0% cpu 7.989 total
    ./compact_lang_det_batch > xxx  7.77s user 0.60s system 98% cpu 8.479 total
    (langid.c) @mlui langid.c git:[master] time cat wikifiles | ./langidOs -b > xxx           
    cat wikifiles  0.00s user 0.00s system 0% cpu 3.577 total
    ./langidOs -b > xxx  3.44s user 0.24s system 97% cpu 3.759 total

    (langid.c) @mlui langid.c git:[master] wc -l rcv2files 
    20000 rcv2files
    (langid.c) @mlui langid.c git:[master] time cat rcv2files | ./langidO2 -b > xxx     
    cat rcv2files  0.00s user 0.00s system 0% cpu 31.702 total
    ./langidO2 -b > xxx  8.23s user 0.54s system 22% cpu 38.644 total
    (langid.c) @mlui langid.c git:[master] time cat rcv2files | ./compact_lang_det_batch > xxx 
    cat rcv2files  0.00s user 0.00s system 0% cpu 18.343 total
    ./compact_lang_det_batch > xxx  18.14s user 0.53s system 97% cpu 19.155 total


Model Training
--------------

Google's protocol buffers [4] are used to transfer models between languages. The
Python program `ldpy2ldc.py` can convert a model produced by langid.py [2] into
the protocol-buffer format, and also the C source format used to compile an
in-built model directly into executable.

Dependencies
------------
Protocol buffers [4]
protobuf-c [5]

Contact
-------
Marco Lui <saffsd@gmail.com>

References
----------
[1] http://aclweb.org/anthology-new/I/I11/I11-1062.pdf
[2] https://github.com/saffsd/langid.py
[3] https://code.google.com/p/cld2/
[4] https://github.com/google/protobuf/
[5] https://github.com/protobuf-c/protobuf-c
