# --- .gdbinit CORREGIDO ---

# PASO 1: Especificar la arquitectura ANTES de conectar.
# Esto le dice a GDB que se prepare para un entorno de 64 bits.
set architecture i386:x86-64

# PASO 2: Cargar el archivo ejecutable con los símbolos.
# Esto le da a GDB el "mapa" del código. Ahora sabe dónde está 'main'.
file Kernel/kernel.elf

# PASO 3: Conectarse al servidor GDB de QEMU.
# Ahora que GDB sabe la arquitectura, la conexión será exitosa.
target remote host.docker.internal:1234

# (Opcional) Cargar símbolos adicionales para los módulos de usuario.
# add-symbol-file Userland/Shell/shell.elf 0x400000