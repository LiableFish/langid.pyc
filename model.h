#ifndef _MODEL_H
#define _MODEL_H

#define NUM_FEATS 7480
#define NUM_LANGS 97
#define NUM_STATES 9118

/*
Glossary:
- tk is [byte n-gram] Tokenizer based on Aho-Corasick Deterministic Finite Automaton [DFA]
- tk_nextmove is an array that represents transition in DFA from given state through given char (byte)
- tk_output is a flat array of features that can be completed upon entering any state
- tk_output_s is an array that holds the starting index in the `tk_output` array for each state
- tk_output_c is an array that holds the count of features that can be completed by entering each state
- nb is Naive Bayes
- nb_pc is P(C), prior [log] probability of class == language
- nb_ptc is P(t|C), [log] probability of token (== byte n-gram) given class == language
*/
extern unsigned tk_nextmove[NUM_STATES][256]; // 256 [alphabet in DFA] == 2 ** 8, and "8" states for 8 bit in UTF-8
extern unsigned tk_output_c[NUM_STATES];
extern unsigned tk_output_s[NUM_STATES];
extern unsigned tk_output[];
extern double nb_pc[NUM_LANGS];
extern double nb_ptc[725560]; // 725560 = NUM_FEATS * NUM_LANGS
extern char* nb_classes[NUM_LANGS];

#endif
