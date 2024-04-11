/*
 * Implementation of the Language Identification method of Lui & Baldwin 2011
 * in pure C, based largely on langid.py, using the sparse set structures suggested
 * by Dawid Weiss.
 *
 * Marco Lui <saffsd@gmail.com>, July 2014
 */

#include "liblangid.h"
#include "langid.pb-c.h"
#include "sparseset.h"
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


LanguageIdentifier* load_identifier(const char* model_path) {
    Langid__LanguageIdentifier* msg;
    int fd;
    unsigned char* model_buf;
    LanguageIdentifier* lid;

    fd = open(model_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Unable to open: %s\n", model_path);
        return NULL;
    }

    off_t model_len = lseek(fd, 0, SEEK_END);
    model_buf = mmap(NULL, model_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (model_buf == MAP_FAILED) {
        fprintf(stderr, "Failed to map the model file: %s\n", model_path);
        close(fd);
        return NULL;
    }

    msg = langid__language_identifier__unpack(NULL, model_len, model_buf);
    if (msg == NULL) {
        fprintf(stderr, "Error unpacking model from: %s\n", model_path);
        munmap(model_buf, model_len);
        close(fd);
        return NULL;
    }

    lid = (LanguageIdentifier*)malloc(sizeof(LanguageIdentifier));
    if (lid == NULL) {
        fprintf(stderr, "Memory allocation failed for LanguageIdentifier\n");
        langid__language_identifier__free_unpacked(msg, NULL);
        munmap(model_buf, model_len);
        close(fd);
        return NULL;
    }

    lid->sv = alloc_set(msg->num_states);
    lid->fv = alloc_set(msg->num_feats);

    lid->num_feats = msg->num_feats;
    lid->num_langs = msg->num_langs;
    lid->num_states = msg->num_states;

    lid->tk_nextmove = (unsigned(*)[][256])msg->tk_nextmove;
    lid->tk_output_c = (unsigned(*)[])msg->tk_output_c;
    lid->tk_output_s = (unsigned(*)[])msg->tk_output_s;
    lid->tk_output = (unsigned(*)[])msg->tk_output;

    lid->nb_pc = (double(*)[])msg->nb_pc;
    lid->nb_ptc = (double(*)[])msg->nb_ptc;
    lid->nb_classes = (char*(*)[])msg->nb_classes;

    lid->protobuf_model = msg;

    lid->nb_classes_mask = malloc(sizeof(bool) * msg->num_langs);

    if (lid->nb_classes_mask == NULL) {
        fprintf(stderr, "Memory allocation failed for language_mask\n");
        free(lid);
        langid__language_identifier__free_unpacked(msg, NULL);
        munmap(model_buf, model_len);
        close(fd);
        return NULL;
    } else {
        for (size_t i = 0; i < msg->num_langs; ++i) {
            lid->nb_classes_mask[i] = true;
        }
    }
    return lid;
}

void destroy_identifier(LanguageIdentifier* lid) {
    if (lid->protobuf_model != NULL) {
        langid__language_identifier__free_unpacked(lid->protobuf_model, NULL);
    }
    free(lid->nb_classes_mask);
    free_set(lid->sv);
    free_set(lid->fv);
    free(lid);
}

/* 
 * Convert a text stream into a feature vector. The feature vector counts
 * how many times each sequence is seen.
 */
static void text_to_fv(LanguageIdentifier* lid, const char* text, unsigned int text_len, Set* sv, Set* fv) {
    unsigned int i, j, m, s = 0;

    clear(sv);
    clear(fv);

    for (i = 0; i < text_len; ++i) {
        s = (*lid->tk_nextmove)[s][(unsigned char)text[i]];
        add(sv, s, 1);
    }

    /* convert the SV into the FV */
    for (i = 0; i < sv->members; ++i) {
        m = sv->dense[i];
        for (j = 0; j < (*lid->tk_output_c)[m]; ++j) {
            add(fv, (*lid->tk_output)[(*lid->tk_output_s)[m] + j], sv->counts[i]);
        }
    }

    return;
}

static void fv_to_logprob(LanguageIdentifier* lid, Set* fv, double logprob[]) {
    unsigned int i, j, m;
    double* nb_ptc_p;

    /* Initialize using prior taking into account supported language mask */
    for (i = 0; i < lid->num_langs; ++i) {
        if (lid->nb_classes_mask[i]) {
            logprob[i] = (*lid->nb_pc)[i];
        } else {
            logprob[i] = -INFINITY;
        }
    }

    /* Compute posterior for each class */
    for (i = 0; i < fv->members; ++i) {
        m = fv->dense[i];
        /* NUM_FEATS * NUM_LANGS */
        nb_ptc_p = &(*lid->nb_ptc)[m * lid->num_langs];
        for (j = 0; j < lid->num_langs; ++j) {
            logprob[j] += fv->counts[i] * (*nb_ptc_p);
            nb_ptc_p += 1;
        }
    }

    return;
}

static void logprob_to_prob(double logprob[], unsigned int size) {
    /*  python reference: pd = 1 / np.exp(pd[None,:] - pd[:,None]).sum(1)
    this is basically softmax:
        x = pd[i]
        new_pd[i] = 1 / sum_{y in pd} e^(y - x) = e^x / sum_{y in pd} e^y = softmax(x)
    */

    unsigned int i;
    double sum = 0.0;
    double max_logprob = -INFINITY;

    for (i = 0; i < size; ++i) {
        if (logprob[i] > max_logprob) {
            max_logprob = logprob[i];
        }
    }

    for (i = 0; i < size; ++i) {
        if (logprob[i] == -INFINITY) {
            logprob[i] = 0;
        } else {
            logprob[i] = exp(logprob[i] - max_logprob);
        }
        sum += logprob[i];
    }

    if (sum == 0) {
        return;
    }

    for (i = 0; i < size; ++i) {
        logprob[i] /= sum;
    }

    return;
}

static unsigned int prob_to_pred_idx(double prob[], unsigned int size) {
    unsigned int i, m = 0;

    for (i = 1; i < size; ++i) {
        if (prob[m] < prob[i]) {
            m = i;
        }
    }

    return m;
}

LanguageConfidence classify(LanguageIdentifier* lid, const char* text, unsigned int text_len) {
    double lp[lid->num_langs];
    unsigned int pred_idx;
    LanguageConfidence pred;

    text_to_fv(lid, text, text_len, lid->sv, lid->fv);
    fv_to_logprob(lid, lid->fv, lp);
    logprob_to_prob(lp, lid->num_langs);

    pred_idx = prob_to_pred_idx(lp, lid->num_langs);

    pred.language = (*lid->nb_classes)[pred_idx];
    pred.confidence = lp[pred_idx];

    return pred;
}

static int compare_language_confidence(const void* first, const void* second) {
    LanguageConfidence* first_lc = (LanguageConfidence*)first;
    LanguageConfidence* second_ls = (LanguageConfidence*)second;

    return (first_lc->confidence < second_ls->confidence) - (first_lc->confidence > second_ls->confidence);
}

void rank(LanguageIdentifier* lid, const char* text, unsigned int text_len, LanguageConfidence* out) {
    double lp[lid->num_langs];
    unsigned int i;

    text_to_fv(lid, text, text_len, lid->sv, lid->fv);
    fv_to_logprob(lid, lid->fv, lp);
    logprob_to_prob(lp, lid->num_langs);

    for (i = 0; i < lid->num_langs; ++i) {
        out[i].language = (*lid->nb_classes)[i];
        out[i].confidence = lp[i];
    }

    // sort in descending order
    qsort(out, lid->num_langs, sizeof(LanguageConfidence), compare_language_confidence);
}

int set_languages(LanguageIdentifier* lid, const char* langs[], unsigned int num_langs) {
    if (langs == NULL) {
        for (size_t i = 0; i < lid->num_langs; ++i) {
            lid->nb_classes_mask[i] = true;
        }
        return 0;
    }

    size_t lang_to_nb_classes_index[num_langs];

    for (size_t i = 0; i < num_langs; ++i) {
        const char* lang = langs[i];
        bool lang_found = false;

        for (size_t j = 0; j < lid->num_langs; ++j) {
            if (strcmp(lang, (*lid->nb_classes)[j]) == 0) {
                lang_found = true;
                lang_to_nb_classes_index[i] = j;
                break;
            }
        }

        if (!lang_found) {
            fprintf(stderr, "Unsupported language code %s\n", lang);
            return -1;
        }
    }

    for (size_t i = 0; i < lid->num_langs; ++i) {
        lid->nb_classes_mask[i] = false;
    }

    for (size_t i = 0; i < num_langs; ++i) {
        lid->nb_classes_mask[lang_to_nb_classes_index[i]] = true;
    }

    return 0;
}
