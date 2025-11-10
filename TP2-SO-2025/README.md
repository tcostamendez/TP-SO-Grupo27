# TP2 - Sistemas Operativos (72.11)
## Núcleo de SO y Administración de Recursos

## Integrantes

| Name                   | Legajo     | Email                      |
|------------------------|------------|----------------------------|
| Pablo Gorostiaga       | 65148      | pgorostiaga@itba.edu.ar    |
| Santiago Rapan         | 65510      | srapan@itba.edu.ar         |
| Tomas Costa Menendez   | 65625      | tcostamenendez@itba.edu.ar | 


## Estructura del Repositorio
```text
/
├── Bootloader/ 
├── Image/
├── Kernel/
│   ├── ASM/
│   ├── Datastructures/
│   ├── IDT/
│   ├── Memory Managers/
│   ├── Kernel.c
│   ├── lib.c
│   ├── mvar.c
│   ├── pipe.c
|   ├── process.c
|   ├── scheduler.c
|   ├── sem.c
|   └── strings.c
├── Toolchain/
├── Userland
│   ├── libc/
│   ├── libsys/
│   ├── Shell
│       ├── commands/
|       └── shell.c
├── Makefile
├── compile.sh
├── run.sh
└── README.md
```

## Entorno de compilacion y ejecuccion:
Es necesario compilar usando la imagen provista por la cátedra.
```sh
docker pull agodio/itba-so-multi-platform:3.0
```
Requisitos locales
* Docker Desktop 
* Soporte para virtualización (QEMU)

Para compilar, simplemente ejectuar el comando
```sh
./compile.sh && ./run.sh
```
### Aclaraciones
* Si es que se quiere usar el memory manager buddy, se debe incluir el argumento `buddy`
```sh
./compile.sh buddy && ./run.sh
```
* Todas las invocaciones se deben realizar desde el directorio raíz del repositorio.

## Comandos y tests (Nombres, utilidad y parametros)
### Utilidades
* sh: Shell de usuario. Soporta `&` (Background) y `|` (Pipe). Soporta las señal `ctrl + c` para matar el proceso corriendo actualmente en foreground y `ctrl + d` para enviar el EOF.
* help: Lista todos los comandos disponibles, incluyendo los tests
* man `comando`: Muestra información acerca de un comando
* echo `texto`: Imprime el texto proporcionado
* clear: Limpia la pantalla

### Memoria
* mem: Imprime el estado actual de la memoria (total, ocupada, libre)

### Procesos / Scheduling / Context Switching
* ps: Lista todos los procesos, mostrando: nombre, PID, PPID, prioridad, foreground/background, estado (RUNNING, READY o BLOCKED)
* loop `milliseconds`: Imprime un saludo con su PID cada N millisegundos
* kill `PID`: Mata el proceso con PID = `PID` 
* nice `PID PRIORITY`: Cambia la prioridad de un proceso dado su PID y la nueva prioridad
* block `PID`: Bloquea o desbloquea el proceso con PID = `PID` 

### Comunicacion entre procesos (IPC)
* cat: Copia `stdin` a `stdout`
* wc: Cuenta líneas del input
* filter: Filtra vocales del input
* mvar `NUM_READERS NUM_WRITERS`: Simula MVar (lectores/escritores múltiples) sobre una variable global. Cada escritor espera aleatoriamente, luego espera a que la variable esté vacía y escribe un valor único (p.ej. A, B, C). Cada lector espera aleatoriamente, luego espera a que haya valor, lo lee e imprime con un identificador (p.ej. color). El proceso principal finaliza tras crear lectores/escritores.

### Tests
* test_mm `bytes`: Ciclo infinito que pide y libera bloques de tamaño aleatorio, chequeando que no se solapen. El parámetro indica la cantidad máxima de memoria a utilizar en bytes.
* test_processes `MAX_PROC`: Crea/bloquea/desbloquea/mata procesos dummy aleatoriamente. El parámetro representa la cantidad máxima de procesos a crear.
* test_prio `TARGET`: Lanza 3 procesos que incrementan una variable desde 0. Primero con la misma prioridad y luego con prioridades distintas para observar diferencias. El parámetro representa el valor al que debe llegar la variable para que un proceso finalize.
* test_sync `NUM_PROC NUM_ITER`: Ejecuta incrementos/decrementos concurrentes sobre una variable global con semáforos; el resultado final debe ser 0. Toma como parámetros la cantidad de procesos y la cantidad de incrementos/decrementos.
* test_sync `NUM_PROC NUM_ITER` (modo no sincronizado): Igual que el anterior pero sin semáforos; el resultado varía entre ejecuciones debido a race conditions.


## Ejemplos de Uso

### Ejecución en Background
Para ejecutar un proceso en background, agregar `&` al final del comando:
```sh
loop 1000 &
loop 2000 &
```
Esto ejecutará dos procesos `loop` en background simultáneamente y se podra seguir interactuando con la shell.

### Uso de Pipes
Para conectar dos procesos mediante un pipe, usar el símbolo `|`:
```sh
cat | filter
cat | wc
help | filter
```

### Tests de Memoria
```sh
test_mm 100000
```
Ejecuta el test de stress del memory manager con 100000 bytes de memoria.

### Tests de Procesos y Scheduling
```sh
test_processes 10
test_prio 3
```

### Tests de Sincronización
```sh
test_sync 100 0
test_sync 500 1
```
Ejecuta procesos que realizan 100 y 500 incrementos/decrementos no syncronizados y syncronizados respectivamente.

### MVar (Lectores/Escritores)
```sh
mvar 2 3
```
Crea 2 lectores y 3 escritores que operan sobre una variable compartida. 

### Decisiones de Usabilidad

#### Protección de Procesos Críticos
Contamos con el proceso idle (PID = 0) y el proceso init (PID = 1). Consideramos estos procesos como críticos por lo cual **no se puede ni bloquear ni matar**. Esto garantiza la estabilidad del sistema.

#### Proceso Init como Guardián del Sistema
El proceso init actúa como un "watchdog" que monitorea la shell. Si la shell es matada o termina inesperadamente, **el proceso init la recrea automáticamente**, garantizando que siempre haya una interfaz de usuario disponible.

#### Reparenting de Procesos Huérfanos
Para evitar tener procesos huérfanos cuando su proceso padre termina o es matado, implementamos un mecanismo de **reparenting automático**: todos los hijos de un proceso que muere son reasignados al proceso init (PID = 1), que asume la responsabilidad de estos procesos.

#### Limitación de Pipes
El shell soporta **únicamente pipes de exactamente 2 procesos** (ejemplo: `cat | filter`). No se permite el encadenamiento de más de 2 comandos (ejemplo: `p1 | p2 | p3`). Esta decisión simplifica la implementación y cubre los casos de uso más comunes.

#### Built-ins No Soportan Pipes
Los comandos built-in (`man`, `exit`) **no pueden usarse en pipelines**. Esto se debe a que los built-ins se ejecutan en el contexto del shell y no como procesos separados, lo que los hace incompatibles con el mecanismo de pipes implementado.

#### EOF Automático en Pipes
Los pipes detectan **EOF automáticamente** cuando no quedan procesos escritores activos. Esto previene bloqueos indefinidos y permite que los readers finalicen correctamente cuando el writer termina.

#### `ctrl + c` Solo Mata Procesos en Foreground
La señal `ctrl + c` únicamente mata al proceso que está corriendo en foreground en ese momento. Los procesos en background **no son afectados** por esta señal. Dado que los procesos en background pueden ser matados o bloqueados manualmente desde la shell usando `kill` o `block`, no consideramos necesario tener una señal para interrumpirlos. Además, esto protege procesos en background de ser interrumpidos accidentalmente.

#### Memory Managers Intercambiables
Los dos memory managers implementados (First Fit y Buddy System) **comparten la misma interfaz**, permitiendo cambiar entre ellos en tiempo de compilación sin modificar el resto del código del kernel. Esto facilita la comparación de rendimiento entre algoritmos.

#### Sincronización con WaitPid
Cada proceso cuenta con un **semáforo nombrado único** que permite la sincronización entre padre e hijo. Cuando un padre ejecuta `waitPid()`, se bloquea en este semáforo hasta que el hijo termine, momento en el cual el hijo hace `post` sobre el semáforo antes de terminar.

### API de MVar
Decidimos implementar el comando MVar completamente desde Userland, a execpcion de dos syscalls para poder acceder al valor de la variable compartida utilizada por los procesos. Tenemos en cuenta que esto se podria haber generalizado y podriamos haber implementado una API de memoria compartida, sin embargo consideramos que esto extiende los requisitos de la materia. 

## Limitaciones
* **Liberacion de memoria por procesos individuales**: Nuestro Kernel confia que todo proceso que solicita memoria se va a encargar de liberarla, dado que no contamos con un registro de las alocaciones de memoria por parte de cada proceso. Sabemos que esto genera memory leaks, por ejemplo en el caso del `test_mm`, dado que cuando se mata este proceso suele estar la memoria reservada pero no se llega a liberar, por lo que la cantidad de bytes pasados como parametros van a quedar desreferenciados. 
* **Encadenamiento de pipes**: Solo se soportan pipes de exactamente 2 procesos. No es posible encadenar más de 2 comandos (ej: `p1 | p2 | p3`).
* **Built-ins en pipes**: Los comandos built-in no pueden ser usados en pipelines.
* **Máximo de procesos e hijos**: La tabla de procesos tiene un tamaño fijo definido por `MAX_PROCESSES` y ademas cada proceso tiene un maximo numero de hijos definido por `MAX_CHILDREN`.

### Requerimientos faltantes
Ninguno. Todos los requerimientos del enunciado fueron implementados exitosamente.

### Creditos / Citas / IA
Este proyecto fue creado a partir del ultimo commit hecho al repositorio [TPE-ARQ-2024](https://github.com/itba-tpietravallo/TPE-ARQ-2024), el cual fue desarrollado por Tomas Pietravallo (tpietravallo@itba.edu.ar), Lucia Oliveto (loliveto@itba.edu.ar), y Maximo Wehncke (mwehncke@itba.edu.ar). Un sitio el cual fue utilizado para el desarrollo de este proyecto fue [Wiki OS Dev](https://wiki.osdev.org/Expanded_Main_Page). 

Este proyecto fue realizado con la ayuda de Inteligencia Artificial. 
