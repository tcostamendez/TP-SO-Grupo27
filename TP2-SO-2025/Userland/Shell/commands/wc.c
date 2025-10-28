#include <stdio.h>

int _wc(int argc, char *argv[]) {
    int c, lines = 0;
    while(1) {
        c = getchar();
        if (c == EOF)
            break;
        if (c == '\n')
            lines++;
	}
    printf("%d line%s in total\n", lines, lines == 1 ? "" : "s");
    return 0;
}