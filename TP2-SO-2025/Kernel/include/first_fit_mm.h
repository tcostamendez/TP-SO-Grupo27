#ifndef FIRST_FIT_MM_H
#define FIRST_FIT_MM_H

#include "memory_manager.h" // Incluimos la interfaz común
#include <string.h> 

// Definición de la unidad de asignación.
// Usaremos un long para asegurar un alineamiento adecuado en 64-bit.
typedef long Align; 

// La estructura del header del bloque de memoria
typedef union header {
    struct {
        union header *ptr;  // Puntero al siguiente bloque libre en la lista (si está libre)
        size_t size;        // Tamaño del bloque (en unidades de union header)
    } s;
    Align x; // Miembro dummy para forzar el alineamiento.
} Header;

// Las funciones de la interfaz ya están definidas en memory_manager.h

#endif // FIRST_FIT_MM_H