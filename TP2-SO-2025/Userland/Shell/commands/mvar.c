// MVar-like demo: multiple writers/readers on a single-slot variable
#include <libsys.h>
#include "entry_points.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Single global slot (shared among children within same address space in this TP)
static volatile char g_mvar_value = 0;

// Tiny PRNG-derived delay to create interleavings using sys sleep
static unsigned prng(unsigned seed) {
	seed ^= (unsigned)getMyPid();
	seed = seed * 1103515245u + 12345u;
	return seed;
}

static void random_delay_ms(unsigned base) {
	unsigned r = prng(base);
	unsigned ms = (r % 200u) + 10u; // [10..209] ms
	sleep(ms);
}

// Helpers: integer to string (base 10)
static void intToStr(int v, char *buf) {
	if (!buf) return;
	if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
	int neg = 0; if (v < 0) { neg = 1; v = -v; }
	char tmp[16]; int i = 0;
	while (v > 0 && i < (int)sizeof(tmp)-1) { tmp[i++] = (char)('0' + (v % 10)); v /= 10; }
	int k = 0; if (neg) buf[k++] = '-';
	while (i > 0) buf[k++] = tmp[--i];
	buf[k] = '\0';
}

// argv: ["mvar_writer", emptyName, fullName, letter]
int _mvar_writer(int argc, char **argv) {
	if (argc < 4) return -1;
	const char *emptyName = argv[1];
	const char *fullName  = argv[2];
	char letter = argv[3] && argv[3][0] ? argv[3][0] : '?';

	sem_t empty = semOpen(emptyName, 0);
	sem_t full  = semOpen(fullName, 0);
	if (!empty || !full) return -1;

	random_delay_ms((unsigned)letter);

	semWait(empty);
	g_mvar_value = letter;
	semPost(full);

	printf("[writer %c] wrote %c\n", letter, letter);
	return 0;
}

// argv: ["mvar_reader", emptyName, fullName, readerId]
int _mvar_reader(int argc, char **argv) {
	if (argc < 4) return -1;
	const char *emptyName = argv[1];
	const char *fullName  = argv[2];
	int rid = atoi(argv[3]);

	sem_t empty = semOpen(emptyName, 0);
	sem_t full  = semOpen(fullName, 0);
	if (!empty || !full) return -1;

	random_delay_ms((unsigned)(rid * 41 + 7));

	semWait(full);
	char v = g_mvar_value;
	g_mvar_value = 0;
	semPost(empty);

	printf("[reader %d] read %c\n", rid, v);
	return 0;
}

// argv: ["mvar", writers, readers]
int _mvar(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: mvar <writers> <readers>\n");
		return -1;
	}
	int writers = atoi(argv[1]);
	int readers = atoi(argv[2]);
	if (writers <= 0 || readers <= 0) {
		printf("mvar: writers/readers must be > 0\n");
		return -1;
	}

	// Unique semaphore names per mvar based on this command's pid
	int pid = getMyPid();
	char pidStr[16]; intToStr(pid, pidStr);
	char emptyName[24]; char fullName[24];
	strcpy(emptyName, "mvarE_"); strcpy(fullName, "mvarF_");
	strcpy(emptyName + strlen("mvarE_"), pidStr);
	strcpy(fullName  + strlen("mvarF_"), pidStr);

	// Initialize semaphores: empty=1 (slot free), full=0 (slot empty)
	sem_t empty = semOpen(emptyName, 1);
	sem_t full  = semOpen(fullName, 0);
	if (!empty || !full) { printf("mvar: semOpen failed\n"); return -1; }

	// Spawn writers
	for (int i = 0; i < writers; ++i) {
		char letterStr[2] = { (char)('A' + (i % 26)), 0 };
		char *wargv[5];
		wargv[0] = "mvar_writer";
		wargv[1] = emptyName;
		wargv[2] = fullName;
		wargv[3] = letterStr;
		wargv[4] = 0;
		int targets[3] = {0,1,2};
		createProcess(4, wargv, (void(*)(int,char**))entry_mvar_writer, 2, targets, 0);
	}

	// Spawn readers
	for (int j = 0; j < readers; ++j) {
		char idbuf[12]; intToStr(j, idbuf);
		char *rargv[5];
		rargv[0] = "mvar_reader";
		rargv[1] = emptyName;
		rargv[2] = fullName;
		rargv[3] = idbuf;
		rargv[4] = 0;
		int targets[3] = {0,1,2};
		createProcess(4, rargv, (void(*)(int,char**))entry_mvar_reader, 2, targets, 0);
	}

	// Main returns immediately
	return 0;
}

