#include <stdio.h>
#include <stdlib.h>

int _mem(int argc, char *argv[]) {
	if (argc > 1) {
		perror("Usage: mem\n");
		return 1;
	}

	int free_bytes = get_free_bytes();
	int used_bytes = get_used_bytes();
	int total_bytes = get_total_bytes();

	if (free_bytes < 0 || used_bytes < 0 || total_bytes < 0) {
		perror("Error retrieving memory status\n");
		return 1;
	}

	printf("Memory Status:\n");
	printf("\tFree Bytes: %d\n", free_bytes);
	printf("\tUsed Bytes: %d\n", used_bytes);
	printf("\tTotal Bytes: %d\n", total_bytes);

	return 0;
}