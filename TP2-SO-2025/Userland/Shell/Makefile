include ../Makefile.inc

MODULE=shell.bin
SOURCES=$(wildcard [^_]*.c ./tests/*.c ./commands/*.c)

all: $(MODULE)

$(MODULE): $(SOURCES)
	$(GCC) $(GCCFLAGS) -T shellModule.ld _loader.c -L../ $(SOURCES) -l:libc.a -l:libsys.a -o ../$(MODULE)

clean:
	rm -rf *.o ./tests/*.o ./commands/*.o

.PHONY: all clean print
