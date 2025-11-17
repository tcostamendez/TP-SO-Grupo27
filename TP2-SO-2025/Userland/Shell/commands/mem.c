#include <stdio.h>
#include <stdlib.h>
#include "libsys.h"

typedef long Align; 

typedef union header {
    struct {
        union header *ptr;  
        size_t size;         
    } s;
    Align x; 
} Header;

int _mem(int argc, char *argv[]) {
	if (argc > 1) {
		printf("Cantidad invlaida de argumentos\n");
		return 1;
	}

	int free_bytes = 0;
	int used_bytes = 0;
	int total_bytes = 0;

	memoryStats(&total_bytes, &free_bytes, &used_bytes);
	printf("%d\n", sizeof(Header));
	printf("Datos de memoria:\nTotal de bytes: %d\nBytes Disponibles: %d\nBytes usados: %d\n", total_bytes, free_bytes, used_bytes);
	return 0;
	
}