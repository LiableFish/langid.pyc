CC := cc
CFLAGS := -Os -Wall -I/opt/homebrew/include
LDFLAGS := -L/opt/homebrew/lib
LDLIBS := -lm -lprotobuf-c

# Object files
OBJS := liblangid.o sparseset.o langid.pb-c.o

# Phony targets are not associated with files
.PHONY: all clean

# Default target
all: langid

# Clean up generated files
clean:
	rm -f langid $(OBJS) langid.pb-c.c langid.pb-c.h

# Rules for generating .o files from .c files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Header dependencies for object files
liblangid.o: liblangid.h langid.pb-c.h sparseset.h
sparseset.o: sparseset.h
langid.pb-c.o: langid.pb-c.h

# Final linking command
langid: langid.c $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) langid.c $(OBJS) $(LDLIBS) -o $@

# Rule to generate protobuf-c source and header from .proto files
langid.pb-c.c langid.pb-c.h: ../proto/langid.proto
	protoc-c --proto_path=../proto --c_out=. $<
