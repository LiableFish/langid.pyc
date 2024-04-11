from _langid import LangId as _LangId
from pathlib import Path
from typing import List, Optional, Tuple


class LanguageIdentifier:
    def __init__(self, backend: _LangId) -> None:
        self._backend = backend

    @classmethod
    def from_modelpath(cls, path: Path) -> "LanguageIdentifier":
        return cls(backend=_LangId(str(path)))

    def classify(self, text: str) -> Tuple[str, float]:
        return self._backend.classify(text)

    def rank(self, text: str) -> List[Tuple[str, float]]:
        return self._backend.rank(text)

    def set_languages(self, langs: Optional[List[str]] = None) -> None:
        return self._backend.set_languages(langs)

    @property
    def nb_classes(self) -> List[str]:
        return [
            nb_class
            for nb_class, mask in zip(
                self._backend.nb_classes, self._backend.nb_classes_mask
            )
            if mask
        ]
