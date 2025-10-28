#include <stdio.h>
#include <unistd.h>

int _unblock(int argc, char * argv[]) {
    if (argc != 2) {
		perror("Usage: unblock <pid>\n");
		return 1;
	}

	int pid = 0;
	sscanf(argv[1], "%d", &pid);
	if (pid <= 0) {
		perror("Invalid PID\n");
		return 1;
	}
    unblockProcess(pid);
    printf("Process (PID %d) blocked\n", pid);

    return 0;
}