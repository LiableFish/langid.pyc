import langid.langid as langid_py
import pytest

from langid_pyc.default import DEFAULT_IDENTIFIER


@pytest.fixture(scope="session")
def langid_py_identifier():
    identifier = langid_py.LanguageIdentifier.from_modelstring(
        langid_py.model,
        norm_probs=True,
    )
    yield identifier


@pytest.fixture(scope="function")
def langid_pyc_identifier():
    yield DEFAULT_IDENTIFIER
    DEFAULT_IDENTIFIER.set_languages()
