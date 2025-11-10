// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stdio.h>
#include "syscalls.h"
#include "test_util.h"
#include "entry_points.h"
#include <libsys.h>

#define SEM_ID "sem"
#define TOTAL_PAIR_PROCESSES 2

typedef void* sem_t;

int64_t global; // shared memory

void slowInc(int64_t *p, int64_t inc) {
  uint64_t aux = *p;
  my_yield(); // This makes the race condition highly probable
  aux += inc;
  *p = aux;
}

uint64_t my_process_inc(uint64_t argc, char *argv[]) {
  uint64_t n;
  int64_t inc;
  int8_t use_sem;
  sem_t sem;
  uint64_t i;

  if (argc != 4) {
    printf("argc: %d (expected 4)\n", argc);
    return -1;
  }

  // argv[0] = process name, argv[1] = n, argv[2] = inc, argv[3] = use_sem
  if ((n = satoi(argv[1])) <= 0) {
    printf("Invalid n value: %s\n", argv[1]);
    return -1;
  }

  if ((inc = satoi(argv[2])) == 0) {
    printf("Invalid inc value: %s\n", argv[2]);
    return -1;
  }

  if ((use_sem = satoi(argv[3])) < 0) {
    printf("Invalid use_sem value: %s\n", argv[3]);
    return -1;
  }
  
  if (use_sem) {
    sem = my_sem_open(SEM_ID, 1);
    if (!sem) {
      printf("test_sync: ERROR opening semaphore\n");
      return -1;
    }
  }
  for (i = 0; i < n; i++) {
    if (use_sem) {
      my_sem_wait(sem);
    }
    printf("Global before: %ld\nSoy: %d\n", global, my_getpid());
    slowInc(&global, inc);
    printf("Global after: %ld\nSoy: %d\n", global, my_getpid());
    if (use_sem) {
      my_sem_post(sem);
    }
  }

  if (use_sem)
    my_sem_close(sem);  

  printf("Done!\n");
  return 0;
}

uint64_t _test_sync(uint64_t argc, char *argv[]) { //{process_name, value, use_sem}
  uint64_t pids[2 * TOTAL_PAIR_PROCESSES];

  if (argc != 3) {
    return -1;
  }
  
  // Allocate space for 4 pointers (process_name + 3 args)
  char **argvDec = allocMemory(sizeof(char *) * 4);
  char **argvInc = allocMemory(sizeof(char *) * 4);

  // Setup arguments for decrement processes
  argvDec[0] = "test_sync_process_dec";
  argvDec[1] = argv[1];  // n (number of iterations)
  argvDec[2] = "-1";     // inc (decrement)
  argvDec[3] = argv[2];  // use_sem
  
  // Setup arguments for increment processes
  argvInc[0] = "test_sync_process_inc";
  argvInc[1] = argv[1];  // n (number of iterations)
  argvInc[2] = "1";      // inc (increment)
  argvInc[3] = argv[2];  // use_sem

  global = 0;

  printf("\nCreating processes...\n");
  printf("argvDec: [%s, %s, %s, %s]\n", argvDec[0], argvDec[1], argvDec[2], argvDec[3]);
  printf("argvInc: [%s, %s, %s, %s]\n", argvInc[0], argvInc[1], argvInc[2], argvInc[3]);

  uint64_t i;
  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    // Call createProcess with entry point wrapper and explicit argc=4
    pids[i] = createProcess(4, argvDec, (void (*)(int, char**))entry_my_process_inc, 3, (int[]){0, 1, 2}, 0);

    // Call createProcess with entry point wrapper and explicit argc=4
    pids[i + TOTAL_PAIR_PROCESSES] = createProcess(4, argvInc, (void (*)(int, char**))entry_my_process_inc, 3, (int[]){0, 1, 2}, 0);
  }

  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    my_wait(pids[i]);
    my_wait(pids[i + TOTAL_PAIR_PROCESSES]);
  }

  printf("Final value: %d\n", global);

  return 0;
}
