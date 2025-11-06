#include "stdio.h"

int _cat(int argc, char *argv[]) {
    if(argc > 1){
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
        return 0;
    }
    int c;
    while((c = getchar()) != EOF){
        printf("%c", c);
    }
    return 0;
}
