MODEL := ldpy3.model
CC := cc
CFLAGS := -Os -Wall -I/opt/homebrew/include
LDFLAGS := -L/opt/homebrew/lib
LDLIBS := -lm -lprotobuf-c

# Object files
OBJS := liblangid.o model.o sparseset.o langid.pb-c.o

# Phony targets are not associated with files
.PHONY: all clean

# Default target
all: langid langid_pb2.py

# Clean up generated files
clean:
	rm -f langid $(OBJS) model.c model.h langid.pb-c.c langid.pb-c.h langid_pb2.py

# Rules for generating .o files from .c files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Header dependencies for object files
liblangid.o: liblangid.h langid.pb-c.h model.h sparseset.h
model.o: model.h
sparseset.o: sparseset.h
langid.pb-c.o: langid.pb-c.h

# Final linking command
langid: langid.c $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) langid.c $(OBJS) $(LDLIBS) -o $@

# Rules for generating model headers and source from MODEL
model.h: $(MODEL) ldpy2ldc.py
	python ldpy2ldc.py --header $< -o $@

model.c: $(MODEL) ldpy2ldc.py model.h
	python ldpy2ldc.py $< -o $@

# Generate Python protobuf file
langid_pb2.py: langid.proto
	protoc --python_out=. $<

# Rule to generate protobuf-c source and header from .proto files
%.pb-c.c %.pb-c.h: %.proto
	protoc-c --c_out=. $<

# Rule to generate protobuf model from .model files
%.pmodel: %.model langid_pb2.py ldpy2ldc.py
	python ldpy2ldc.py --protobuf -o $@ $<
