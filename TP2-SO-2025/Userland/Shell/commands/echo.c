#include "stdio.h"
int _echo(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Cantidad de parametros invalida, uso: echo <string>\n");
        return -1;
    }
    for (int i = 1; i < argc; i++) {
        printf(argv[i]);
        if (i + 1 < argc) printf(" ");
    }
    printf("\n");
    return 0;
}