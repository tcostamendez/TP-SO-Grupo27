#include <stdio.h>
#include "syscalls.h"
#include "test_util.h"

enum TEST_PROCESS_STATE { _RUNNING,
             _BLOCKED,
             _KILLED };

typedef struct P_rq {
  int32_t pid;
  enum TEST_PROCESS_STATE state;
} p_rq;

int64_t _test_processes(uint64_t argc, char *argv[]) {
  uint8_t rq;
  uint8_t alive = 0;
  uint8_t action;
  uint64_t max_processes;
  char *argvAux[] = {0};
  int iteration = 0;

  if (argc == 1)
    return -1;

  if ((max_processes = satoi(argv[1])) <= 0)
    return -1;

  printf("test_processes: starting with max_processes=%u\n", (unsigned)max_processes);
  p_rq p_rqs[max_processes];

  while (1) {
    iteration++;
    printf("\n=== test_processes: iteration %d ===\n", iteration);

    // Create max_processes processes
    for (rq = 0; rq < max_processes; rq++) {
      p_rqs[rq].pid = my_create_process(endless_loop, 0, argvAux);

      if (p_rqs[rq].pid == -1) {
        printf("test_processes: ERROR creating process\n");
        return -1;
      } else {
        p_rqs[rq].state = _RUNNING;
        alive++;
        printf("test_processes: created pid=%d (index=%d)\n", p_rqs[rq].pid, rq);
      }
    }

    // Randomly kills, blocks or unblocks processes until every one has been killed
    while (alive > 0) {

      for (rq = 0; rq < max_processes; rq++) {
        action = GetUniform(100) % 2;

        switch (action) {
          case 0:
            if (p_rqs[rq].state == _RUNNING || p_rqs[rq].state == _BLOCKED) {
              printf("test_processes: killing pid=%d (index=%d)\n", p_rqs[rq].pid, rq);
              if (my_kill(p_rqs[rq].pid) == -1) {
                printf("test_processes: ERROR killing process pid=%d\n", p_rqs[rq].pid);
                return -1;
              }
              p_rqs[rq].state = _KILLED;
              alive--;
              printf("test_processes: killed pid=%d, alive=%d\n", p_rqs[rq].pid, alive);
            }
            break;

          case 1:
            if (p_rqs[rq].state == _RUNNING) {
              printf("test_processes: blocking pid=%d (index=%d)\n", p_rqs[rq].pid, rq);
              if (my_block(p_rqs[rq].pid) == -1) {
                printf("test_processes: ERROR blocking process pid=%d\n", p_rqs[rq].pid);
                return -1;
              }
              p_rqs[rq].state = _BLOCKED;
              printf("test_processes: blocked pid=%d\n", p_rqs[rq].pid);
            }
            break;
        }
      }

      // Randomly unblocks processes
      for (rq = 0; rq < max_processes; rq++)
        if (p_rqs[rq].state == _BLOCKED && GetUniform(100) % 2) {
          printf("test_processes: unblocking pid=%d (index=%d)\n", p_rqs[rq].pid, rq);
          if (my_unblock(p_rqs[rq].pid) == -1) {
            printf("test_processes: ERROR unblocking process pid=%d\n", p_rqs[rq].pid);
            return -1;
          }
          p_rqs[rq].state = _RUNNING;
          printf("test_processes: unblocked pid=%d\n", p_rqs[rq].pid);
        }
    }
    printf("test_processes: iteration %d completed, restarting...\n", iteration);
  }
}
