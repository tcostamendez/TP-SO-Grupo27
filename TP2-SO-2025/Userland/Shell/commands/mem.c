#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int _mem(int argc, char *argv[]) {
	if (argc > 1) {
		perror("Usage: mem\n");
		return 1;
	}

	int free_bytes = getFreeBytes();
	int used_bytes = getUsedBytes();
	int total_bytes = getTotalBytes();

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