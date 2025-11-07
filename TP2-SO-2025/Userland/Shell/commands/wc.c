#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "../shell.h"

int _wc(int argc, char *argv[]){
    if (argc > 1) {
        printf("Se registro 1 linea por stdin\n"); //consideramos 1 linea los argumentos
        return 0;
    }
    uint32_t lines = 0;
    int c;
    while ((c = getchar()) != EOF && c != '\0') {
        if (c == '\n') {
            lines++;
        }
    }
    printf("Se registraron %d lineas por stdin\n", lines);
    return 0;
}