#include "stdio.h"
#include "libsys.h"


int _loop(int argc, char *argv[]) {
    if (argc != 2) {
		perror("Usage: loop <miliseconds>\n");
		return 1;
	}

    int milliseconds = 0;

    if (argv != NULL && argv[1] != NULL && (sscanf(argv[1], "%d", &milliseconds) != 1 || milliseconds <= 0)) {
		perror("Invalid time interval\n");
		return 1;
	}

    int pid = getMyPid();

    // Intentional infinite loop for demonstration purposes
    //-V776 (PVS error)
    while (1) {
		printf("Hello from PID: %d\n", pid);
		sleep(milliseconds);
	}

    return 0;
}