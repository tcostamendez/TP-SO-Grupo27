#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int _nice(int argc, char *argv[]) {
	if (argc != 3) {
		perror("Usage: nice <pid> <value> (Where value is between 0 and 3)\n");
		return 1;
	}
	int pid = 0;
	if (sscanf(argv[1], "%d", &pid) != 1 || pid <= 0) {
		perror("Invalid pid\n");
		return 1;
	}

	int new_prio = 0;
	if (sscanf(argv[2], "%d", &new_prio) != 1 || new_prio < 0 || new_prio > 3) {
		perror("Invalid value\n");
		return 1;
	}

	int ret = setProcessPriority(pid, new_prio);
	if (ret == -1) {
		perror("Error changing process priority\n");
		return 1;
	}

	return 0;
}