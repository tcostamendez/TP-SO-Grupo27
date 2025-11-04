
#include <stdio.h>
#include <stdlib.h>

#include "../shell.h"

extern Command commands[];
extern const int commands_size;

int _help(int argc, char *argv[]) {
	if (argc > 1) {
		perror("Usage: help\n");
		return 1;
	}
	
	printf("Available commands:\n");
	uint8_t max_name_length = 0;
	for (int i = 0; i < commands_size; i++) {
		if (strlen(commands[i].name) > max_name_length) {
			max_name_length = strlen(commands[i].name);
		}
	}

	for (int i = 0; i < commands_size; i++) {
		char padding[max_name_length - strlen(commands[i].name)];
		int j = 0;
		for (; j < sizeof(padding) / sizeof(*padding); j++) padding[j] = ' ';
		padding[j] = '\0';

		printf("%s%s -\t%s\n\n", commands[i].name, padding, commands[i].description);
	}

	printf("\n");
	return 0;
}
