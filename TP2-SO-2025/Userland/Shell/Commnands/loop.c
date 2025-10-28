#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys.h>

int _loop(int argc, char *argv[]) {

	int milliseconds = 1000;

	if (argc != 2) {
		perror("Usage: loop <miliseconds>\n");
		return 1;
	}

	if (argv != NULL && argv[1] != NULL && (sscanf(argv[1], "%d", &milliseconds) != 1 || milliseconds <= 0)) {
		perror("Invalid time interval\n");
		return 1;
	}
    
    int pid = getpid();
	while (1) {
		printf("Hello from PID: %d\n", pid);
		sleep(milliseconds);
	}

	return 0;
}