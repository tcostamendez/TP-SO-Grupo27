#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int _kill(int argc, char *argv[]) {
	if (argc != 2) {
		perror("Usage: kill <pid>\n");
		return 1;
	}
	
	int pid = 0;
	sscanf(argv[1], "%d", &pid);
	if (pid <= 0) {
		perror("Invalid PID\n");
		return 1;
	}
	int ret = killProcess(pid);
	if (ret == -1) {
		perror("Error killing process\n");
		return 1;
	}
	return 0;
}
