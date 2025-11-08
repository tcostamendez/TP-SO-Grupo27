// MVar-like demo: multiple writers/readers on a single-slot variable
#include <libsys.h>
#include "entry_points.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void random_delay(void) {
	unsigned int ms = (rand() % 200) + 10;  // [10..209] ms
	sleep(ms);
}

// Main mvar command: creates writers and readers, then exits immediately
// argv: ["mvar", <readers>, <writers>]
int _mvar(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: mvar <readers> <writers>\n");
		return -1;
	}

	int readers = atoi(argv[1]);
	int writers = atoi(argv[2]);
	
	if (readers <= 0 || writers <= 0) {
		printf("mvar: readers and writers must be > 0\n");
		return -1;
	}

	// Initialize the MVar
	if (mvarInit(readers, writers) < 0) {
		printf("mvar: failed to initialize MVar\n");
		return -1;
	}

	// Create writer processes
	for (int i = 0; i < writers; i++) {
		char letter[2];
		letter[0] = 'A' + (i % 26);
		letter[1] = '\0';
		
		char *writer_argv[] = {"mvar_writer", letter, NULL};
		int pid = createProcess(2, writer_argv, (void (*)(int, char**))entry_mvar_writer, 
		                       1, (int[]){0, 1, 2}, 0);
		if (pid < 0) {
			printf("mvar: failed to create writer %c\n", letter[0]);
		}
	}

	// Create reader processes
	for (int i = 0; i < readers; i++) {
		char idStr[16];
		itoa(i, idStr, 10);
		
		char *reader_argv[] = {"mvar_reader", idStr, NULL};
		int pid = createProcess(2, reader_argv, (void (*)(int, char**))entry_mvar_reader,
		                       1, (int[]){0, 1, 2}, 0);
		if (pid < 0) {
			printf("mvar: failed to create reader %d\n", i);
		}
	}

	// Parent exits immediately
	return 0;
}

// Writer process: repeatedly writes its assigned letter
// argv: ["mvar_writer", <letter>]
int _mvar_writer(int argc, char **argv) {
	if (argc != 2) {
		return -1;
	}
	
	char letter = argv[1][0];
	if (letter < 'A' || letter > 'Z') {
		letter = '?';
	}

	// Seed random number generator with unique value per writer
	srand(getMyPid() + letter);

	// Loop and write multiple times
	while (1) {
		random_delay();
		
		if (mvarPut(letter) < 0) {
			printf("Writer %c: failed to put value\n", letter);
			return -1;
		}
		
		// Writers just write, they don't print
	}

	return 0;
}

// Reader process: repeatedly reads and prints values
// argv: ["mvar_reader", <id>]
int _mvar_reader(int argc, char **argv) {
	if (argc != 2) {
		return -1;
	}
	
	int readerId = atoi(argv[1]);

	// Array of colors for different readers (bright colors)
	uint32_t colors[] = {
		0x00FF0000,  // Bright Red
		0x0000FF00,  // Bright Green
		0x00FFFF00,  // Bright Yellow
		0x000000FF,  // Bright Blue
		0x00FF00FF,  // Bright Magenta
		0x0000FFFF,  // Bright Cyan
		0x00DE382B,  // Red
		0x0039B54A,  // Green
		0x00FFC706,  // Yellow
		0x00006FB8,  // Blue
	};
	
	// Assign a unique color to this reader
	uint32_t myColor = colors[readerId % (sizeof(colors) / sizeof(colors[0]))];
	
	// Seed random number generator with unique value per reader
	srand(getMyPid() * 41 + readerId);

	// Loop and read multiple times
	while (1) {
		random_delay();
		
		char value = mvarGet();
		if (value == 0) {
			printf("Reader %d: failed to get value\n", readerId);
			return -1;
		}
		
		// Set color and print what was read with reader identifier
		setTextColor(myColor);
		printf("Reader %d read: %c\n", readerId, value);
		setTextColor(0x00FFFFFF);  // Reset to white
	}

	return 0;
}

