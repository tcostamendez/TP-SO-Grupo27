// #include "../include/buddy_system_mm.h"
// #include "../include/memory_manager.h"

// // Inicialización del arreglo de listas (global)
// free_block_t *buddy_lists[MAX_ORDER + 1] = {NULL};
// static void *start_addr = NULL;

// void mm_init(void *base_address, size_t total_size) {
//     // 1. Limpiar todas las listas
//     for (int i = 0; i <= MAX_ORDER; i++) {
//         buddy_lists[i] = NULL;
//     }

//     // 2. Determinar el orden más alto
//     // Encontrar la potencia de 2 más cercana y menor que total_size.
//     int max_k = 0;
//     while ((1 << (max_k + 1)) * MIN_BLOCK_SIZE <= total_size) {
//         max_k++;
//     }

//     // 3. Colocar el bloque inicial grande en la lista del orden más alto
//     start_addr = base_address;
//     free_block_t *initial_block = (free_block_t *)base_address;
//     initial_block->next = NULL;
    
//     // Asumiendo que max_k no excede MAX_ORDER
//     buddy_lists[max_k] = initial_block;
    
//     // (Actualizar estadísticas)
// }

// void *mm_alloc(size_t size) {
//     // 1. Determinar el tamaño y orden de bloque requerido (nuestro destino)
//     size_t required_size = size + sizeof(alloc_metadata_t); // Overhead del metadato
//     if (required_size < MIN_BLOCK_SIZE) {
//         required_size = MIN_BLOCK_SIZE;
//     }
    
//     int target_order = 0;
//     while ((1 << target_order) * MIN_BLOCK_SIZE < required_size) {
//         target_order++;
//     }

//     // 2. Buscar el bloque libre: Empezamos en el orden destino y subimos
//     for (int current_order = target_order; current_order <= MAX_ORDER; current_order++) {
        
//         if (buddy_lists[current_order] != NULL) {
//             // ¡Bloque encontrado!
//             free_block_t *block = buddy_lists[current_order];
//             buddy_lists[current_order] = block->next; // Eliminar de la lista
            
//             // 3. Dividir si el bloque es más grande que el solicitado
//             while (current_order > target_order) {
//                 current_order--;
                
//                 // Dividir el bloque en dos buddies de 2^(k-1)
//                 size_t half_size = (1 << current_order) * MIN_BLOCK_SIZE;
                
//                 // El primer buddy es 'block', el segundo es 'buddy'
//                 free_block_t *buddy = (free_block_t *)((uint8_t *)block + half_size);
                
//                 // El segundo buddy va a la lista de bloques libres del orden actual
//                 buddy->next = buddy_lists[current_order];
//                 buddy_lists[current_order] = buddy;
//             }

//             // 4. Asignar y devolver: Guardar metadatos en el bloque y devolver el puntero de datos
//             alloc_metadata_t *metadata = (alloc_metadata_t *)block;
//             metadata->order = (uint8_t)target_order;
            
//             // Devolver la dirección *después* de los metadatos
//             return (void *)((uint8_t *)block + sizeof(alloc_metadata_t));
//         }
//     }

//     return NULL; // Memoria agotada
// }

// void mm_free(void *ptr) {
//     if (ptr == NULL) return;

//     // 1. Obtener metadatos: Volver al inicio del bloque asignado
//     alloc_metadata_t *metadata = (alloc_metadata_t *)((uint8_t *)ptr - sizeof(alloc_metadata_t));
//     free_block_t *block_to_free = (free_block_t *)metadata;
//     int current_order = metadata->order;

//     // 2. Coalescing (Unión) con el Buddy
//     while (current_order < MAX_ORDER) {
//         size_t block_size = (1 << current_order) * MIN_BLOCK_SIZE;
        
//         // Calcular la dirección del buddy (XORing la dirección base con el tamaño del bloque)
//         // Esta es la propiedad mágica de los Buddies.
//         uint64_t block_addr = (uint64_t)block_to_free;
//         uint64_t buddy_addr = block_addr ^ block_size; 
        
//         free_block_t *buddy = (free_block_t *)buddy_addr;
        
//         // Comprobar si el buddy está libre en la lista del mismo orden
//         if (is_buddy_free(buddy, current_order)) { 
//             // Sí, el buddy está libre -> UNIR
            
//             // Eliminar al buddy de la lista
//             remove_from_list(buddy, current_order);
            
//             // El nuevo bloque unificado es el que tiene la dirección más baja
//             if (block_addr < buddy_addr) {
//                 // block_to_free ya es el bloque menor
//             } else {
//                 // El buddy original es ahora el inicio del bloque grande
//                 block_to_free = buddy; 
//             }
            
//             current_order++; // Pasamos al siguiente orden
            
//         } else {
//             break; // El buddy no está libre o no está en el mismo orden, se detiene la unión
//         }
//     }

//     // 3. Colocar el bloque (posiblemente unido) en su lista final
//     block_to_free->next = buddy_lists[current_order];
//     buddy_lists[current_order] = block_to_free;
// }