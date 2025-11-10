/* _loader.c */
#include <stddef.h>
#include <stdint.h>

extern char bss;
extern char endOfBinary;

int main();

static void *memset(void *destiny, int32_t c, uint64_t length);

int _start() {
	// Clean BSS
	// Avoid pointer-subtraction between distinct objects for static analyzers
	// by computing sizes using integer casts.
	uintptr_t start = (uintptr_t)&bss;
	uintptr_t end = (uintptr_t)&endOfBinary;
	uint64_t size = (end > start) ? (uint64_t)(end - start) : 0;
	memset(&bss, 0, size);

	return main();
}

static void *memset(void *destiny, int32_t c, uint64_t length) {
	uint8_t chr = (uint8_t)c;
	char *dst = (char *)destiny;

	while (length--) dst[length] = chr;

	return destiny;
}
