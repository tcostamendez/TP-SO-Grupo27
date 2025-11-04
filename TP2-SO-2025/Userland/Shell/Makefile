include ../Makefile.inc

MODULE=shell.bin
SOURCES=$(filter-out _loader.c, $(wildcard *.c ./commands/*.c))

all: $(MODULE)

$(MODULE): $(SOURCES)
	$(GCC) $(GCCFLAGS) -T shellModule.ld _loader.c -L../ $(SOURCES) -l:libc.a -l:libsys.a -o ../$(MODULE)

clean:
	rm -rf *.o

.PHONY: all clean print