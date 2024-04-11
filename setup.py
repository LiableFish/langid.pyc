from setuptools import setup, Extension


langid_extension = Extension(
    "_langid",
    language="c",
    libraries=["protobuf-c"],
    include_dirs=[
        "/opt/homebrew/include",  # Include directory for protobuf-c headers
        "lib",
    ],
    library_dirs=[
        "/opt/homebrew/lib",  # Library directory for protobuf-c
    ],
    sources=[
        "lib/_langid.c",
        "lib/liblangid.c",
        "lib/sparseset.c",
        "lib/langid.pb-c.c",
    ],
)

setup(
    packages=["langid_pyc"],
    ext_modules=[langid_extension],
    package_data={
        "langid_pyc": ["ldpy3.pmodel"],
    },
    long_description_content_type="text/markdown",
)
