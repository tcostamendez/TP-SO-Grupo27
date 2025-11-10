#include <stdlib.h>

// C's stdlib pseudo random number generator
// https://wiki.osdev.org/Random_Number_Generator

static unsigned long int next = 1;	// NB: "unsigned long int" is assumed to be 32 bits wide

int rand(void) {  // RAND_MAX assumed to be 32767

	next = next * 1103515245 + 12345;
	return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) { next = seed; }

int atoi(const char *str) {
	int result = 0;
	int sign = 1;
	int i = 0;

	// Skip whitespace
	while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
		i++;
	}

	// Handle sign
	if (str[i] == '-') {
		sign = -1;
		i++;
	} else if (str[i] == '+') {
		i++;
	}

	// Convert digits
	while (str[i] >= '0' && str[i] <= '9') {
		result = result * 10 + (str[i] - '0');
		i++;
	}

	return sign * result;
}

char *itoa(int value, char *str, int base) {
	if (str == NULL || base < 2 || base > 36) {
		return NULL;
	}

	char *ptr = str;
	char *ptr1 = str;
	char tmp_char;
	int tmp_value;

	// Handle 0 explicitly
	if (value == 0) {
		*ptr++ = '0';
		*ptr = '\0';
		return str;
	}

	// Handle negative numbers for base 10
	int is_negative = 0;
	if (value < 0 && base == 10) {
		is_negative = 1;
		value = -value;
	}

	// Process individual digits
	while (value != 0) {
		tmp_value = value % base;
		*ptr++ = (tmp_value < 10) ? (tmp_value + '0') : (tmp_value - 10 + 'a');
		value /= base;
	}

	// Add negative sign for base 10
	if (is_negative) {
		*ptr++ = '-';
	}

	*ptr-- = '\0';

	// Reverse the string
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	return str;
}

void *memset(void *destination, int32_t c, uint64_t length) {
	uint8_t chr = (uint8_t)c;
	char *dst = (char *)destination;

	while (length--) dst[length] = chr;

	return destination;
}

// Same as Kernel's memcpy
void *memcpy(void *destination, const void *source, uint64_t length) {
	/*
   * memcpy does not support overlapping buffers, so always do it
   * forwards. (Don't change this without adjusting memmove.)
   *
   * For speedy copying, optimize the common case where both pointers
   * and the length are word-aligned, and copy word-at-a-time instead
   * of byte-at-a-time. Otherwise, copy by bytes.
   *
   * The alignment logic below should be portable. We rely on
   * the compiler to be reasonably intelligent about optimizing
   * the divides and modulos out. Fortunately, it is.
   */
	uint64_t i;

#ifdef DEBUG_LIB_MEM_FNS
	if (destination == NULL || source == NULL) {
		panic("memcpy: destination or source is NULL\n");
		return NULL;
	}
	// If the destination and source overlap, panic
	if (((uint8_t *)destination >= (uint8_t *)source &&
		 ((int64_t)(uint8_t *)destination - (int64_t)(uint8_t *)source <= (int64_t)length)) ||
		((uint8_t *)source >= (uint8_t *)destination &&
		 ((int64_t)(uint8_t *)source - (int64_t)(uint8_t *)destination <= (int64_t)length))) {
		print("Overlapping buffers detected:\n");
		print("  Destination: 0x");
		printHex((uint64_t)destination);
		print("\n");
		print("  Source:      0x");
		printHex((uint64_t)source);
		print("\n");
		print("  Length:      ");
		printDec(length);
		print("\n");
		panic("memcpy: destination and source overlap\n");
		return NULL;
	}
#endif

	if ((uint64_t)destination % sizeof(uint32_t) == 0 && (uint64_t)source % sizeof(uint32_t) == 0 &&
		length % sizeof(uint32_t) == 0) {
		uint32_t *d = (uint32_t *)destination;
		const uint32_t *s = (const uint32_t *)source;

		for (i = 0; i < length / sizeof(uint32_t); i++) d[i] = s[i];
	} else {
		uint8_t *d = (uint8_t *)destination;
		const uint8_t *s = (const uint8_t *)source;

		for (i = 0; i < length; i++) d[i] = s[i];
	}

	return destination;
}

int get_free_bytes(void) {
	// Placeholder implementation
	return 0;
}
int get_used_bytes(void) {
	// Placeholder implementation
	return 0;
}
int get_total_bytes(void) {
	// Placeholder implementation
	return 0;
}