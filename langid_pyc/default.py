from langid_pyc.identifier import LanguageIdentifier
from pathlib import Path
from typing import List, Optional, Tuple


DEFAULT_MODEL_PATH = Path(__file__).parent / "ldpy3.pmodel"
DEFAULT_IDENTIFIER = LanguageIdentifier.from_modelpath(DEFAULT_MODEL_PATH)


def classify(text: str) -> Tuple[str, float]:
    return DEFAULT_IDENTIFIER.classify(text)


def rank(text: str) -> List[Tuple[str, float]]:
    return DEFAULT_IDENTIFIER.rank(text)


def set_languages(langs: Optional[List[str]] = None) -> None:
    global DEFAULT_IDENTIFIER
    DEFAULT_IDENTIFIER.set_languages(langs)
    return


def nb_classes() -> List[str]:
    return DEFAULT_IDENTIFIER.nb_classes
