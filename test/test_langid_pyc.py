import numpy as np
import pytest

from langid_pyc import LanguageIdentifier


def test_nb_classes(langid_py_identifier, langid_pyc_identifier):
    assert langid_pyc_identifier.nb_classes == langid_py_identifier.nb_classes


@pytest.mark.parametrize(
    "text, true_lang",
    (
        ("", "en"),
        ("this is english text", "en"),
        ("это текст на русском", "ru"),
        ("tämä on suomenkielinen teksti", "fi"),
    ),
)
def test_classify(langid_py_identifier, langid_pyc_identifier, text, true_lang):
    lang, prob = langid_py_identifier.classify(text)
    pyc_lang, pyc_prob = langid_pyc_identifier.classify(text)

    assert pyc_lang == lang == true_lang
    assert pyc_prob == pytest.approx(prob)


@pytest.mark.parametrize(
    "text",
    (
        "",
        "this is english text",
        "это текст на русском",
        "tämä on suomenkielinen teksti",
    ),
)
def test_rank(langid_py_identifier, langid_pyc_identifier, text):
    langs, probs = zip(*langid_py_identifier.rank(text))
    pyc_langs, pyc_probs = zip(*langid_pyc_identifier.rank(text))

    assert len(pyc_langs) == len(pyc_probs) == len(langid_pyc_identifier.nb_classes)
    assert set(pyc_langs) == set(langs)
    assert np.allclose(pyc_probs, probs)


@pytest.mark.parametrize("langs", (["en"], ["en", "fi"]))
def test_set_languages(langid_pyc_identifier, langs):
    langid_pyc_identifier.set_languages(langs)
    assert langid_pyc_identifier.nb_classes == langs

    lang, _ = langid_pyc_identifier.classify("это текст на русском")
    assert lang in langs


def test_set_empty_languages(langid_pyc_identifier):
    langid_pyc_identifier.set_languages([])
    assert not langid_pyc_identifier.nb_classes

    _, prob = langid_pyc_identifier.classify("text")
    assert prob == 0.0


def test_reset_set_languages(langid_pyc_identifier):
    all_classes = langid_pyc_identifier.nb_classes
    assert langid_pyc_identifier.nb_classes == all_classes

    langid_pyc_identifier.set_languages(["en", "ru"])
    assert langid_pyc_identifier.nb_classes == ["en", "ru"]

    langid_pyc_identifier.set_languages()
    assert langid_pyc_identifier.nb_classes == all_classes


def test_set_languages_raises_error_if_unknown_language(langid_pyc_identifier, capsys):
    with pytest.raises(RuntimeError, match="Failed to set"):
        langid_pyc_identifier.set_languages(["unknown_language"])
        assert capsys.readouterr().err.startswith("Unsupported language code")


def test_load_model_raises_error_if_path_not_found(capsys):
    with pytest.raises(RuntimeError, match="Failed to load"):
        LanguageIdentifier.from_modelpath("unknown_path")
        assert capsys.readouterr().err.startswith("Unable to open")
