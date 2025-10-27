# TP2 - Sistemas Operativos (72.11)
## Núcleo de SO y Administración de Recursos

## Integrantes

| Name                   | Legajo     | Email                      |
|------------------------|------------|----------------------------|
| Pablo Gorostiaga       | 65148      | pgorostiaga@itba.edu.ar    |
| Santiago Rapan         | .....      | srapan@itba.edu.ar         |
| Tomas Costa Menendez   | .....      | @itba.edu.ar   | 


## Estructura del Repositorio
* /boot/ 
* /kernel/ 
- ├─ arch/ 
- ├─ mm/ 
- ├─ proc/ 
- ├─ ipc/ 
├─ sync/ 
├─ drivers/ 
└─ lib/ 
/userland/ 
/Image/
Makefile 
compile.sh
run.sh 
README.md 

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
./run.sh && ./compile.sh
```
### Aclaraciones
* Si es que se quiere usar el memory manager buddy, se debe incluir el argumento `buddy`
* Todas las invocaciones se deben realizar desde el directorio raíz del repositorio.

## Comandos y tests (Nombres, utilidad y parametros)
### Utilidades
* sh: Shell de suuario. Soporta `&` (Background) y `|` (Pipe). Soporta las señal `ctrl + c`
* help: Lista todos los comandos disponibles, incluyendo los tests

### Memoria
* 
* 

### Procesos / Scheduling / Context Switching
* ps: Lista todos los procesos, mostrando: nombre, PID, prioridad, foreground/background, estado, (Y UN PAR DE COSAS MAS)
* kill `PID`: Mata el proceso con PID = `PID` 
* block `PID`: Bloquea el proceso con PID = `PID`
* test_processes `MAX_PROC`: Crea/bloquea/desbloquea/mata procesos dummy aleatoriamente. El parametro representa la cantidad maxima de procesos soportados.
* test_priority `TARGET`: Lanza 3 procesos que incrementan una variable desde 0. Primero con misma prioridad; luego con prioridades distintas para observar diferencias. El parametro representa el valor al que debe llegar la variable para que un proceso finalize. 

### Syncronizacion
* test_synchro `NUM_PROC, NUM_ITER`: Ejecuta incrementos/decrementos concurrentes sobre una variable global con semáforos; el resultado final debe ser 0.
* test_no_synchro `NUM_PROC, NUM_ITER`: Igual que el anterior sin semáforos; el resultado varía entre ejecuciones debido a race conditions.

### Comunicacion entre procesos (IPC)
* cat: Copia `stdin` a `stodout`
* wc: Cuenta lineas del input
* filter: Filtra vocales del input
* mvar `NUM_WRITERS, NUM_READERS`: Simula MVar (lectores/escritores múltiples) sobre una variable global. Cada escritor espera aleatoriamente, luego espera a que la variable esté vacía y escribe un valor único (p.ej. A, B, C). Cada lector espera aleatoriamente, luego espera a que haya valor, lo lee e imprime con un identificador (p.ej. color). El proceso principal finaliza tras crear lectores/escritores.

### Pipes y background


## Ejemplos 


## Limitaciones
* Encadenamiento de pipes de 
### Requerimientos faltantes
Ninguno pq somos unos cracks -_-


### Creditos / Citas / IA
Este proyecto fue creado a partir del ultimo commit hecho al repositorio [TPE-ARQ-2024](https://github.com/itba-tpietravallo/TPE-ARQ-2024), el cual fue desarrollado por Tomas Pietravallo (tpietravallo@itba.edu.ar), Lucia Oliveto (loliveto@itba.edu.ar), y Maximo Wehncke (mwehncke@itba.edu.ar). 

Este proyecto fue realizado con ayuda de Inteligencia Artificial. 