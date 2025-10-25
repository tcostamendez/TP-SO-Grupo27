# include Makefile.inc

# KERNEL=kernel.bin
# SOURCES=$(wildcard *.c ./drivers/*.c ./idt/*.c ./memory/*.c ./datastructures/*.c)
# SOURCES_ASM=$(wildcard asm/*.asm)
# HOT_OBJECTS=./drivers/video.o fonts.o # Compiled with -O3
# OBJECTS=$(SOURCES:.c=.o)
# OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
# LOADERSRC=loader.asm

# LOADEROBJECT=$(LOADERSRC:.asm=.o)
# STATICLIBS=

# all: $(KERNEL)

# $(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
# 	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

# $(HOT_OBJECTS) : %.o: %.c
# 	$(GCC) -O3 $(GCCFLAGS) -I./include -c $< -o $@

# $(filter-out $(HOT_OBJECTS),$(OBJECTS)) : %.o: %.c
# 	$(GCC) $(GCCFLAGS) -I./include -I./font_assets -c $< -o $@

# %.o : %.asm
# 	$(ASM) $(ASMFLAGS) $< -o $@

# # font.o:
# # 	objcopy -O elf64-x86-64 -B i386 -I binary ./font_assets/Solarize.12x29.psf font.o

# $(LOADEROBJECT):
# 	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

# clean:
# 	rm -rf */*.o *.o *.bin

# .PHONY: all clean

#aca se separan los makefiles

include Makefile.inc

# --- SELECCIÓN DEL MEMORY MANAGER ---
# Define la implementación de memoria a usar ('first_fit' o 'buddy')
# El valor por defecto es 'first_fit'.
# Para cambiarlo desde la línea de comandos, ejecuta: make all MM=buddy
MM ?= first_fit

# Basado en la variable MM, elegimos el nombre de archivo .c correcto
ifeq ($(MM), buddy)
    MM_FILE_NAME = buddy_system_mm.c
else
    # Por defecto, o si MM=first_fit, usamos este:
    MM_FILE_NAME = first_fit_mm.c
endif
# ------------------------------------


KERNEL=kernel.bin

# Modificamos SOURCES:
# 1. Quitamos ./memory/*.c de la detección automática
SOURCES=$(wildcard *.c ./drivers/*.c ./idt/*.c ./datastructures/*.c)

# 2. Añadimos explícitamente el .c del memory manager elegido
SOURCES += ./memory/$(MM_FILE_NAME)

# 3. (OPCIONAL) Si tienes otros archivos .c en /memory/ que SÍ son comunes
#    a ambas implementaciones (ej. 'memory_utils.c'), añádelos aquí:
# SOURCES += ./memory/memory_utils.c 


SOURCES_ASM=$(wildcard asm/*.asm)
HOT_OBJECTS=./drivers/video.o fonts.o # Compiled with -O3
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm

LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

all: $(KERNEL)

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

$(HOT_OBJECTS) : %.o: %.c
	$(GCC) -O3 $(GCCFLAGS) -I./include -c $< -o $@

$(filter-out $(HOT_OBJECTS),$(OBJECTS)) : %.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -I./font_assets -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf */*.o *.o *.bin

.PHONY: all clean