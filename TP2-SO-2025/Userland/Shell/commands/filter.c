#include <stdio.h>
#include "../shell.h"

#include <stdio.h>
#include "../shell.h"

#define IS_VOWEL(c) ((c) == 'a' || (c) == 'e' || (c) == 'i' || (c) == 'o' || (c) == 'u' || \
                     (c) == 'A' || (c) == 'E' || (c) == 'I' || (c) == 'O' || (c) == 'U')

int _filter(int argc, char *argv[]) {
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
    int c;
    int printed_any = 0;
    while ((c = getchar()) != EOF) {
        if (c == '\n') {
            putchar('\n');
            printed_any = 1;
            continue;
        }
        if (!IS_VOWEL(c)) {
            putchar(c);
            printed_any = 1;
        }
    }
    if (!printed_any || (printed_any && c == EOF)) {
        putchar('\n');
    }
    return 0;
}