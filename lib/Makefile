CC := cc
CFLAGS := -Os -Wall -I/opt/homebrew/include
LDFLAGS := -L/opt/homebrew/lib
LDLIBS := -lm -lprotobuf-c

OBJS := liblangid.o sparseset.o langid.pb-c.o

.PHONY: all clean

all: langid

clean:
	rm -f langid $(OBJS) langid.pb-c.c langid.pb-c.h

# Rules for generating .o files from .c files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

liblangid.o: liblangid.h langid.pb-c.h sparseset.h
sparseset.o: sparseset.h
langid.pb-c.o: langid.pb-c.h

langid: langid.c $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) langid.c $(OBJS) $(LDLIBS) -o $@

# Rule to generate protobuf-c source and header from .proto files
langid.pb-c.c langid.pb-c.h: ../proto/langid.proto
	protoc-c --proto_path=../proto --c_out=. $<
