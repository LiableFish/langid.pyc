.PHONY: all clean build install upload upload-test lib-clean lib-all

lib-clean:
	$(MAKE) -C lib clean

lib-all:
	$(MAKE) -C lib all

clean: lib-clean
	rm -rdf build dist langid_pyc.egg-info langid_pyc/__pycache__
	rm -f langid_pb2.py langid_pyc/*.pmodel

all: lib-all langid_pb2.py ldpy3.pmodel

# Rule to generate protobuf model from .model files
%.pmodel: models/%.model langid_pb2.py ldpy_to_protobuf.py
	python ldpy_to_protobuf.py -o langid_pyc/$@ $<

# Generate Python protobuf file
langid_pb2.py: proto/langid.proto
	protoc --proto_path=proto --python_out=. $<

build: clean all
	python -m build

install: build
	pip install dist/*.whl --force-reinstall

upload-test: build
	twine upload -r testpypi dist/* --config-file .pypirc

upload: build
	twine upload dist/* --config-file .pypirc
