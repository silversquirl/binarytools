CC := clang -std=c99
CFLAGS := -Wall -pedantic
LDFLAGS :=

BINARIES := $(patsubst %.c,%,$(wildcard *.c))

.PHONY: all clean
all: $(BINARIES)
clean:
	rm -f $(BINARIES)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
