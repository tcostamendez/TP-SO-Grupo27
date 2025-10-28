#include <stdio.h>
#define IS_VOWEL(c) ((c) == 'a' || (c) == 'e' || (c) == 'i' || (c) == 'o' || (c) == 'u' || \
                     (c) == 'A' || (c) == 'E' || (c) == 'I' || (c) == 'O' || (c) == 'U')


int _filter(int argc, char *argv[]) {
    int c;
    while ((c = getchar()) != EOF) {
        if (!IS_VOWEL(c)) {
            putchar(c);
        }
    }
    return 0;
}