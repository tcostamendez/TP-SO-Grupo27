#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>

// Simple test function that runs in a loop
void test_worker(int argc, char** argv) {
    int pid = get_pid();
    int delay = 2; // default 2 seconds
    
    if (argc > 1) {
        delay = atoi(argv[1]);
    }
    
    printf("Worker process %d started (delay: %d seconds)\n", pid, delay);
    
    int count = 0;
    while (1) {
        sleep_milliseconds(delay * 1000);
        printf("Worker %d: iteration %d\n", pid, ++count);
    }
}

int _test_processes(int argc, char** argv) {
    printf("=== Process Management Test ===\n");
    
    // Show current process
    printf("Current PID: %d\n", get_pid());
    
    // List all processes
    printf("\nInitial process list:\n");
    list_processes();
    
    // Create some test processes
    printf("\nCreating test processes...\n");
    
    char* args1[] = {"worker1", "1"};
    char* args2[] = {"worker2", "3"};
    char* args3[] = {"worker3", "2"};
    
    int pid1 = create_process(2, args1, test_worker, 1); // priority 1
    int pid2 = create_process(2, args2, test_worker, 2); // priority 2  
    int pid3 = create_process(2, args3, test_worker, 0); // priority 0
    
    printf("Created processes: %d, %d, %d\n", pid1, pid2, pid3);
    
    // Wait a bit
    printf("\nWaiting 5 seconds...\n");
    sleep_milliseconds(5000);
    
    // Show process list again
    printf("\nProcess list after creation:\n");
    list_processes();
    
    // Test priority change
    printf("\nChanging process %d priority to 3...\n", pid1);
    set_process_priority(pid1, 3);
    
    // Test blocking
    printf("Blocking process %d...\n", pid2);
    block_process(pid2);
    
    // Wait a bit more
    printf("\nWaiting 3 seconds...\n");
    sleep_milliseconds(3000);
    
    // Show final process list
    printf("\nFinal process list:\n");
    list_processes();
    
    // Unblock the process
    printf("\nUnblocking process %d...\n", pid2);
    unblock_process(pid2);
    
    // Kill one process
    printf("Killing process %d...\n", pid3);
    kill_process(pid3);
    
    // Final process list
    printf("\nProcess list after killing %d:\n", pid3);
    list_processes();
    
    printf("\nTest completed! Processes will continue running...\n");
    printf("Use 'ps' command in shell to see them, or 'kill <pid>' to stop them.\n");
    
    return 0;
}
