// #include "../include/first_fit_mm.h"
// #include "../include/memory_manager.h"

// // Variables globales para la gestión de memoria
// static Header base;     // Bloque inicial de tamaño cero para empezar la lista.
// static Header *freep = NULL; // Puntero al último bloque libre encontrado.

// // Variables para estadísticas
// static size_t total_mem_size = 0;
// static size_t free_mem_size = 0;
// // NOTA: En un OS real, el kernel podría tener su propio heap.
// // Esta implementación asumirá la memoria que le pase mm_init.

// // Función utilitaria para obtener más memoria del sistema (como sbrk en UNIX)
// // En un OS, esto toma una página/bloque de memoria física nueva del PMM
// // static Header * morecore(size_t nu);

// // Definición de las unidades de asignación (en bytes)
// #define UNIT_SIZE sizeof(Header)

// void mm_init(void *base_address, size_t total_size) {
//     // Inicialización de las estadísticas
//     total_mem_size = total_size;
//     free_mem_size = total_size;

//     // 1. Convertir el espacio a un bloque Header grande
//     Header *initial_block = (Header *)base_address;
    
//     // El tamaño se mide en unidades Header.
//     // Aseguramos que el espacio sea suficiente para, al menos, un Header.
//     initial_block->s.size = total_size / UNIT_SIZE;
    
//     // 2. Configurar el puntero base y el freep para que apunten al bloque inicial,
//     //    creando una lista circular de un solo elemento (el bloque libre total).
//     base.s.ptr = initial_block;
//     base.s.size = 0; // El bloque base es de tamaño cero
    
//     initial_block->s.ptr = &base;
    
//     freep = &base;
// }

// #define MIN_ALLOC_UNITS 1 // Tamaño mínimo en unidades de Header

// void *mm_alloc(size_t nbytes) {
//     if (nbytes == 0) {
//         return NULL;
//     }
    
//     size_t nunits = (nbytes + UNIT_SIZE - 1) / UNIT_SIZE + 1;
//     if (nunits < MIN_ALLOC_UNITS) {
//         nunits = MIN_ALLOC_UNITS;
//     }
    
//     Header *p, *prevp;
//     prevp = freep;

//     for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
//         if (p->s.size >= nunits) { // Bloque encontrado
//             Header *allocated_block;
            
//             if (p->s.size == nunits) { // Ajuste perfecto
//                 prevp->s.ptr = p->s.ptr;
//                 allocated_block = p;
//             } else { // El bloque es más grande, se divide
//                 // Lógica corregida para dividir un bloque sin corromper el iterador 'p'
//                 p->s.size -= nunits;
//                 allocated_block = p + p->s.size;
//                 allocated_block->s.size = nunits;
//             }
            
//             freep = prevp;
//             free_mem_size -= (nunits * UNIT_SIZE);
//             return (void *)(allocated_block + 1); // Devuelve puntero al área de datos
//         }
        
//         if (p == freep) { // Se dio la vuelta completa a la lista
//             return NULL; 
//         }
//     }
// }
// void mm_free(void *ap) {
//     if (ap == NULL) return; 

//     Header *bp = (Header *)ap - 1;

//     // Se actualiza la estadística ANTES de unir bloques para evitar errores de contabilidad.
//     free_mem_size += (bp->s.size * UNIT_SIZE);

//     Header *p;
//     for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
//         if (p >= p->s.ptr && (bp > p || bp < p->s.ptr)) {
//             break; 
//         }
//     }

//     // Unir con el vecino de la derecha
//     if (bp + bp->s.size == p->s.ptr) { 
//         bp->s.size += p->s.ptr->s.size; 
//         bp->s.ptr = p->s.ptr->s.ptr;
//     } else {
//         bp->s.ptr = p->s.ptr;
//     }

//     // Unir con el vecino de la izquierda
//     if (p + p->s.size == bp) { 
//         p->s.size += bp->s.size; 
//         p->s.ptr = bp->s.ptr;
//     } else {
//         p->s.ptr = bp;
//         p = bp;
//     }
    
//     freep = p;
// }

// MemoryStats mm_get_stats() {
//     MemoryStats stats;
    
//     stats.total_memory = total_mem_size;
//     stats.free_memory = free_mem_size;
    
//     // La memoria ocupada es la diferencia entre el total y el libre
//     if (total_mem_size >= free_mem_size) {
//         stats.occupied_memory = total_mem_size - free_mem_size;
//     } else {
//         // Esto no debería suceder si la aritmética es correcta
//         stats.occupied_memory = 0; 
//     }
    
//     // NOTA: Si quieres proporcionar más detalles (como el número de bloques libres), 
//     // tendrías que recorrer la lista libre 'freep' y contarlos aquí.
    
//     return stats;
// }

// // ---------------------------------------------------------
// // Debug / introspección de la free list
// // ---------------------------------------------------------

// MemoryFreeListInfo mm_get_free_list_info() {
//     MemoryFreeListInfo info = {0,0,(size_t)-1,0};
//     if (freep == NULL) {
//         info.smallest_free_block = 0;
//         return info;
//     }
//     Header *p = freep;
//     do {
//         size_t block_bytes = p->s.size * UNIT_SIZE;
//         info.free_blocks++;
//         if (block_bytes > info.largest_free_block) info.largest_free_block = block_bytes;
//         if (block_bytes < info.smallest_free_block) info.smallest_free_block = block_bytes;
//         info.total_free_bytes += block_bytes;
//         p = p->s.ptr;
//     } while (p != freep);
//     if (info.free_blocks == 0) info.smallest_free_block = 0;
//     return info;
// }

// // void mm_debug_print_free_list() {
// //     MemoryFreeListInfo fi = mm_get_free_list_info();
// //     MemoryStats st = mm_get_stats();
// //     printf("[MM] total=%zu free=%zu occupied=%zu | blocks=%zu largest=%zu smallest=%zu free_bytes=%zu\n",
// //            st.total_memory, st.free_memory, st.occupied_memory,
// //            fi.free_blocks, fi.largest_free_block, fi.smallest_free_block, fi.total_free_bytes);
// // }

// // Pequeña función auxiliar para probar escritura
// static int _check_pattern(uint8_t *ptr, size_t sz, uint8_t val) {
//     for (size_t i=0;i<sz;i++) if (ptr[i] != val) return 0;
//     return 1;
// }

// int mm_self_test() {
//     // Guardamos estado inicial
//     MemoryStats init = mm_get_stats();
//     // 1. Alloc simple
//     size_t test_size = 256;
//     uint8_t *a = (uint8_t*)mm_alloc(test_size);
//     if (!a) return 1;
//     memset(a, 0xA5, test_size);
//     if (!_check_pattern(a, test_size, 0xA5)) return 2;
//     // 2. Segundo alloc
//     uint8_t *b = (uint8_t*)mm_alloc(512);
//     if (!b) return 3;
//     memset(b, 0x5A, 512);
//     if (!_check_pattern(b, 512, 0x5A)) return 4;
//     // 3. Free intercalado
//     mm_free(a);
//     mm_free(b);
//     MemoryStats after = mm_get_stats();
//     if (after.free_memory != init.free_memory) return 5; // Debe volver al estado
//     // 4. Fragmentación / coalescing
//     void *p1 = mm_alloc(1024);
//     void *p2 = mm_alloc(1024);
//     void *p3 = mm_alloc(1024);
//     if (!p1 || !p2 || !p3) return 6;
//     mm_free(p2);
//     mm_free(p1);
//     mm_free(p3);
//     after = mm_get_stats();
//     if (after.free_memory != init.free_memory) return 7;
//     return 0; // OK
// }