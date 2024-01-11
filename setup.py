from setuptools import setup, Extension

# Extension definition
langid_extension = Extension(
    "_langid",
    language='c',
    libraries=['protobuf-c'],  # Link against the protobuf-c library
    include_dirs=[
        '/opt/homebrew/include',  # Include directory for protobuf-c headers
    ],
    library_dirs=[
        '/opt/homebrew/lib',  # Library directory for protobuf-c
    ],
    sources=["lib/_langid.c", "lib/liblangid.c", "lib/sparseset.c", "lib/langid.pb-c.c"],
)

# Setup function call with updated Extension
setup(
    name='langid',
    version='1.0.0',
    ext_modules=[langid_extension],
)
