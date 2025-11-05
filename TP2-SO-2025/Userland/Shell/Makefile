include ../Makefile.inc

MODULE=shell.bin
SOURCES=$(filter-out _loader.c, $(wildcard *.c ./commands/*.c))
ASM_SOURCES=./commands/entry_points.asm
ASM_OBJECTS=$(ASM_SOURCES:.asm=.o)

all: $(MODULE)

$(ASM_OBJECTS): $(ASM_SOURCES)
	$(ASM) $(ASMFLAGS) $(ASM_SOURCES) -o $(ASM_OBJECTS)

$(MODULE): $(SOURCES) $(ASM_OBJECTS)
	$(GCC) $(GCCFLAGS) -T shellModule.ld _loader.c -L../ $(SOURCES) $(ASM_OBJECTS) -l:libc.a -l:libsys.a -o ../$(MODULE)

clean:
	rm -rf *.o ./commands/*.o

.PHONY: all clean print