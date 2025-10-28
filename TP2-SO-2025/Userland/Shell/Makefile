include ../Makefile.inc

MODULE=shell.bin
# Only include shell.c, test_mm.c, and test_util.c - exclude test_memory.c and test_processes.c
SOURCES=shell.c test_mm.c test_util.c

all: $(MODULE)

$(MODULE): $(SOURCES)
	$(GCC) $(GCCFLAGS) -T shellModule.ld _loader.c -L../ $(SOURCES) -l:libc.a -l:libsys.a -o ../$(MODULE)

clean:
	rm -rf *.o

.PHONY: all clean print
