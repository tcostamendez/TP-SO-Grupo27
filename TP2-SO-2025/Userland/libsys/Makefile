include ../Makefile.inc

MODULE=libsys.a
SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard asm/*.asm)

OBJECTS_C=$(SOURCES_C:.c=.o)
# Keep ASM object files in asm/ directory to avoid naming conflicts
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)

all: $(MODULE)

$(MODULE): $(OBJECTS_C) $(OBJECTS_ASM)
	$(AR) rcs ../$@ $^

%.o : %.c
	$(GCC) -c $(GCCFLAGS) $<

# Build ASM files in their own directory
asm/%.o : asm/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f asm/*.o
	rm -f ../$(MODULE)

.PHONY: all clean print
