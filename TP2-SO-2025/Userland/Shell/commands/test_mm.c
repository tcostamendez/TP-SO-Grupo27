/*#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int _test_mm(int argc, char *argv[]) {
	if (argc != 2) {
		perror("Usage: test_mm <amount>\n");
		return 1;
	}

	int amount;
	if (sscanf(argv[1], "%d", &amount) != 1 || amount <= 0) {
		perror("Invalid amount\n");
		return 1;
	}
	char buf[16] = {0};
	sprintf(buf, "%d", amount);
	char *args[] = {"test_mm", buf, NULL};
	int targets[] = {PIPE_STDIN, PIPE_STDOUT};
	int pid = proc(test_mm, 2, args, 0, targets, 0);
	if (pid < 0) {
		perror("Failed to run test_mm\n");
		return 1;
	}
	
	// waitpid(pid);

	printf("Test completed\n");
	return 0;
}
*/