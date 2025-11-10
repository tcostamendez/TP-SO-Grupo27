#include <stdint.h>
#include <stdio.h>
#include <libsys.h>
#include "test_util.h"

#define TOTAL_PROCESSES 3

#define LOWEST 0  
#define MEDIUM 1  
#define HIGHEST 3 

int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

/* Use the canonical constant for maximum uint64_t value. The original
 * expression used `2^64 -1` which is parsed as (2 ^ 64) - 1 (bitwise XOR)
 * and triggers a warning; also shifting by 64 is undefined. Prefer the
 * macro from <stdint.h> or the bitwise inversion idiom. */
uint64_t max_value = UINT64_MAX;

void zero_to_max() {
  uint64_t value = 0;
  while (value++ != max_value);

  /* my_getpid() returns a pid type; print as int for this printf implementation */
  printf("PROCESS %d DONE!\n", (int)my_getpid());
}

uint64_t _test_prio(uint64_t argc, char *argv[]) {
  printf("Empezando test_prio\n");
  int64_t pids[TOTAL_PROCESSES];
  char *ztm_argv[] = {0};
  uint64_t i;

  /* argc is a 64-bit value here; project printf supports %d, so cast to int */
  printf("argc: %d\n", (int)argc);
  if (argc != 2)
    return -1;

  if ((max_value = satoi(argv[1])) <= 0)
    return -1;

  printf("SAME PRIORITY...\n");

  for (i = 0; i < TOTAL_PROCESSES; i++) {
    pids[i] = my_create_process(zero_to_max, 0, ztm_argv);
  }
  // Expect to see them finish at the same time

  for (i = 0; i < TOTAL_PROCESSES; i++) {
    my_wait(pids[i]);
  }

 
  printf("SAME PRIORITY, THEN CHANGE IT...\n");

  for (i = 0; i < TOTAL_PROCESSES; i++) {
    pids[i] = my_create_process(zero_to_max, 0, ztm_argv);
    my_nice(pids[i], prio[i]);
    printf("PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
  }

  // Expect the priorities to take effect

  for (i = 0; i < TOTAL_PROCESSES; i++)
    my_wait(pids[i]);

  printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

  for (i = 0; i < TOTAL_PROCESSES; i++) {
    pids[i] = my_create_process(zero_to_max, 0, ztm_argv);
    my_block(pids[i]);
    my_nice(pids[i], prio[i]);
    printf("PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
  }

  for (i = 0; i < TOTAL_PROCESSES; i++)
    my_unblock(pids[i]);

  // Expect the priorities to take effect

  for (i = 0; i < TOTAL_PROCESSES; i++)
    my_wait(pids[i]);
  
  return 0;
}
