include Makefile.inc

KERNEL=kernel.bin
SOURCES=$(wildcard *.c ./drivers/*.c ./idt/*.c ./memory/*.c ./datastructures/*.c)
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

# font.o:
# 	objcopy -O elf64-x86-64 -B i386 -I binary ./font_assets/Solarize.12x29.psf font.o

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf */*.o *.o *.bin

.PHONY: all clean

#aca se separan los makefiles

# # Incluye las variables de configuración (GCC, GCCFLAGS, etc.)
# include Makefile.inc

# # Agregamos los directorios de 'includes' que no están en Makefile.inc
# GCCFLAGS += -I./include -I./font_assets

# # --- Detección automática de archivos ---
# C_SOURCES   := $(shell find . -name "*.c")
# ASM_SOURCES := $(shell find . -name "*.asm")
# OBJECTS     := $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o)
# TARGET      = kernel.bin

# # --- Reglas de compilación ---
# .PHONY: all clean

# all: $(TARGET)

# # Regla de enlace
# $(TARGET): $(OBJECTS)
# 	@echo "Enlazando el kernel..."
# 	$(LD) $(LDFLAGS) -T kernel.ld -o $(TARGET) $(OBJECTS)

# # Regla para compilar C
# %.o: %.c
# 	@echo "Compilando C: $<"
# 	$(GCC) $(GCCFLAGS) -c $< -o $@

# # Regla para ensamblar ASM
# %.o: %.asm
# 	@echo "Ensamblando: $<"
# 	$(ASM) $(ASMFLAGS) $< -o $@

# # Regla de limpieza
# clean:
# 	@echo "Limpiando archivos del Kernel..."
# 	rm -f $(TARGET) $(shell find . -name "*.o")