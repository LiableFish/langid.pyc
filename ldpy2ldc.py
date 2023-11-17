"""
Repack the Python model into a form that langid.c can read.

Marco Lui, July 2014

Glossary:
- tk is [byte n-gram] Tokenizer based on Aho-Corasick Deterministic Finite Automaton [DFA]
- tk_nextmove is an array that represents transition in DFA from given state through given char (byte)
- tk_output is a flat array of features that can be completed upon entering any state
- tk_output_s is an array that holds the starting index in the `tk_output` array for each state
- tk_output_c is an array that holds the count of features that can be completed by entering each state
- nb is Naive Bayes
- nb_pc is P(C), prior [log] probability of class == language
- nb_ptc is P(t|C), [log] probability of token (== byte n-gram) given class == language
"""

import argparse
import langid.langid as langid
import sys
from itertools import islice

model_template = """\
#include "model.h"

unsigned tk_nextmove[NUM_STATES][256] = {tk_nextmove};
unsigned tk_output_c[NUM_STATES] = {tk_output_c};
unsigned tk_output_s[NUM_STATES] = {tk_output_s};
unsigned tk_output[] = {tk_output};
double nb_pc[NUM_LANGS] = {nb_pc};
double nb_ptc[{nb_ptc_size}] = {nb_ptc};
char* nb_classes[NUM_LANGS] = {nb_classes};
"""

header_template = """\
#ifndef _MODEL_H
#define _MODEL_H

#define NUM_FEATS {num_feats}
#define NUM_LANGS {num_langs}
#define NUM_STATES {num_states}

extern unsigned tk_nextmove[NUM_STATES][256];
extern unsigned tk_output_c[NUM_STATES];
extern unsigned tk_output_s[NUM_STATES];
extern unsigned tk_output[];
extern double nb_pc[NUM_LANGS];
extern double nb_ptc[{nb_ptc_size}];
extern char* nb_classes[NUM_LANGS];

#endif
"""


def pack_tk_output(identifier):
    """
    `identifier.tk_output` is a mapping from state to list of feats completed by entering that state
    (all the feats that have been recognized upon reaching the state).
    We encode it as a single array of 2-byte ("H" typecode == unsigned short) values:
    the index of the next state is located at `current state * alphabet size [== 256]  + ord(c)`.

    We want to transform `tk_output` python dictionary into three lists
    that can be easily translated into C static arrays:
    - `tk_output_c`: This list will hold the count of features that can be completed by entering each state.
    - `tk_output_s`: This list will hold the starting index in the `tk_output` list
                    where the features for a given state are located.
    - `tk_output`: This flat list will hold all the features that can be completed upon entering any state.

    """
    num_states = len(identifier.tk_nextmove) >> 8

    tk_output_c = []
    tk_output_s = []
    tk_output = []
    for i in range(num_states):
        if i in identifier.tk_output and identifier.tk_output[i]:
            count = len(identifier.tk_output[i])
            feats = identifier.tk_output[i]
        else:
            count = 0
            feats = []

        tk_output_c.append(count)
        tk_output_s.append(len(tk_output))
        tk_output.extend(feats)

    return tk_output_c, tk_output_s, tk_output


def as_c_array_init(seq):
    return "{" + ",".join(map(str, seq)) + "}"


def chunk(seq, chunksize):
    """
    Break a sequence into chunks not exceeding a predetermined size
    """
    seq_iter = iter(seq)
    while True:
        chunk = tuple(islice(seq_iter, chunksize))
        if not chunk:
            break
        yield chunk


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", "-o", default=sys.stdout, help="write exported model to",
                        type=argparse.FileType('w'))
    parser.add_argument("--header", action="store_true", help="produce header file")
    parser.add_argument("--protobuf", action="store_true", help="produce model in protocol buffer format")
    parser.add_argument("model", help="read model from")
    args = parser.parse_args()

    if args.protobuf and args.header:
        parser.error("can only specify one of --protobuf or --header")

    identifier = langid.LanguageIdentifier.from_modelpath(args.model)

    print("NB_PTC", type(identifier.nb_ptc), identifier.nb_ptc.shape, identifier.nb_ptc.dtype)
    print("NB_PC", type(identifier.nb_pc), identifier.nb_pc.shape, identifier.nb_pc.dtype)
    print("NB_NUMFEATS", type(identifier.nb_numfeats), identifier.nb_numfeats)
    print("NB_CLASSES", type(identifier.nb_classes), len(identifier.nb_classes))
    print("TK_NEXTMOVE", type(identifier.tk_nextmove), len(identifier.tk_nextmove), identifier.tk_nextmove.typecode,
          identifier.tk_nextmove.itemsize)
    print("TK_OUTPUT", type(identifier.tk_output), len(identifier.tk_output))

    num_feats, num_langs = identifier.nb_ptc.shape
    # '>> 8' == 'divide by 256', and 256 is the utf-8 alphabet size (number of unique values for 8 bit == 2 ** 8)
    num_states = len(identifier.tk_nextmove) >> 8
    print("NUM_STATES", num_states)

    nb_ptc_size = num_feats * num_langs

    if args.protobuf:
        import langid_pb2

        tk_output_c, tk_output_s, tk_output = pack_tk_output(identifier)

        lid = langid_pb2.LanguageIdentifier()

        # basic parameters
        lid.num_feats = num_feats
        lid.num_langs = num_langs
        lid.num_states = num_states

        # pack the tokenizer
        lid.tk_nextmove.extend(identifier.tk_nextmove)
        lid.tk_output_c.extend(tk_output_c)
        lid.tk_output_s.extend(tk_output_s)
        lid.tk_output.extend(tk_output)

        # pack the classifier parameters
        lid.nb_pc.extend(identifier.nb_pc.tolist())
        lid.nb_ptc.extend(identifier.nb_ptc.ravel().tolist())

        # pack the class labels
        lid.nb_classes.extend('{}'.format(c) for c in identifier.nb_classes)

        args.output.write(lid.SerializeToString())

    elif args.header:
        args.output.write(header_template.format(**locals()))
    else:
        # chunk tk_nextmove back into length-256 array initializers, to avoid C warnings
        # about initialization mismatches
        tk_nextmove = as_c_array_init(as_c_array_init(c) for c in chunk(identifier.tk_nextmove, 256))

        tk_output_c, tk_output_s, tk_output = pack_tk_output(identifier)
        tk_output_c = as_c_array_init(tk_output_c)
        tk_output_s = as_c_array_init(tk_output_s)
        tk_output = as_c_array_init(tk_output)

        nb_pc = as_c_array_init(identifier.nb_pc)
        nb_ptc = as_c_array_init(identifier.nb_ptc.ravel())
        nb_classes = as_c_array_init('"{}"'.format(c) for c in identifier.nb_classes)

        args.output.write(model_template.format(**locals()))


def find_dict():
    import builtins
    builtins_types = [t for t in builtins.__dict__.values() if isinstance(t, type)]
    for t in builtins_types:
        if hasattr(t, "get"):
            return t
