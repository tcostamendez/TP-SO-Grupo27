// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscalls.h>
#include <libsys.h>

int _font(int argc, char *argv[]) {
	if (argc < 2) {
		perror("Usage: font <increase|decrease>\n");
		return 1;
	}
	char *arg = argv[1];
	if (strcasecmp(arg, "increase") == 0) {
		return increaseFontSize();
	} else if (strcasecmp(arg, "decrease") == 0) {
		return decreaseFontSize();
	}

	perror("Invalid argument\n");
	return 0;
}    