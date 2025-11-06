#include <stdio.h>
#include "../shell.h"

#include <stdio.h>
#include "../shell.h"

#define IS_VOWEL(c) ((c) == 'a' || (c) == 'e' || (c) == 'i' || (c) == 'o' || (c) == 'u' || \
                     (c) == 'A' || (c) == 'E' || (c) == 'I' || (c) == 'O' || (c) == 'U')

int _filter(int argc, char *argv[]) {
    // Si hay argumentos: filtra los argumentos
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            for (char *p = argv[i]; *p; ++p) {
                char c = *p;
                if (!IS_VOWEL(c)) {
                    putchar(c);
                }
            }
            if (i + 1 < argc) {
                putchar(' ');
            }
        }
        putchar('\n');
        return 0;
    }

    // Sin argumentos: lee de STDIN (hasta '\n' en TTY o hasta EOF si viene por pipe)
    int c;
    while ((c = getchar()) != EOF) {
        if (c == '\n') {
            break;  // modo interactivo: corta en newline
        }
        if (!IS_VOWEL(c)) {
            putchar(c);
        }
    }
    putchar('\n');
    return 0;
}