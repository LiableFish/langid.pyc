clean:
	rm -f langid_pb2.py *.pmodel

all: langid_pb2.py ldpy3.pmodel

# Rule to generate protobuf model from .model files
%.pmodel: models/%.model langid_pb2.py ldpy_to_protobuf.py
	python ldpy_to_protobuf.py -o $@ $<

# Generate Python protobuf file
langid_pb2.py: proto/langid.proto
	protoc --proto_path=proto --python_out=. $<
