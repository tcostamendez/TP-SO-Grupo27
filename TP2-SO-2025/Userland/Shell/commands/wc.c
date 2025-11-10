// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
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
    int saw_char = 0;
    while ((c = getchar()) != EOF) {
        saw_char = 1;
        if (c == '\n') {
            lines++;
        }
    }
    // Si hubo caracteres y el último no era newline, contar la última línea parcial (convención usual de wc)
    if (saw_char && lines == 0) {
        // Si no hubo newlines pero hubo datos, es 1 línea
        lines = 1;
    }
    printf("Se registraron %d lineas por stdin\n", lines);
    return 0;
}