#include <stdio.h>
#include <stdlib.h>
#include "../shell.h"

extern Command commands[];
extern const int commands_size;

int _man(int argc, char *argv[]) {
	char *command;

	if (argc < 2 || (command = argv[1]) == NULL || argc > 2) {
		perror("Usage: man <command>\n");
		return 1;
	}

	for (int i = 0; i < commands_size; i++) {
		if (strcasecmp(commands[i].name, command) == 0) {
			printf("Command: %s\nInformation: %s\n", commands[i].name, commands[i].description);
			return 0;
		}
	}

	perror("Command not found\n");
	return 1;
}