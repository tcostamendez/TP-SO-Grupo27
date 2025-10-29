#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys.h>

int _nice(int argc, char *argv[]) {
	if (argc != 3) {
		perror("Usage: nice <pid> <value>\n");
		return 1;
	}
	int pid = 0;
	if (sscanf(argv[1], "%d", &pid) != 1 || pid <= 0) {
		perror("Invalid pid\n");
		return 1;
	}

	int priority = 0;
	if (sscanf(argv[2], "%d", &priority) != 1) {
		perror("Invalid value\n");
		return 1;
	}

    if (priority < 0 || priority > 3) {
        perror("Invalid value\n");
        return 1;
    }

    set_process_priority(pid, priority);
    printf("Process (PID %d) priority changed to %d\n", pid, priority);

    return 0;
}