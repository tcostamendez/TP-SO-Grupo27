
#include "strings.h"

int strlen(const char *str) {
    int i = 0;
    while (str[i] != 0) {
		i++;
    }
    return i;
}

void my_strcpy(char *dest, const char *src) {
	int i = 0;
	while (src[i] != 0) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
}   