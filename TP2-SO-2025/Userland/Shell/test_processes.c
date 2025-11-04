/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libsys.h>

// Simple test function that runs in a loop
void test_worker(int argc, char** argv) {
    int pid = getMyPid();
    int delay = 2; // default 2 seconds
    
    if (argc > 1) {
        delay = atoi(argv[1]);
    }
    
    printf("Worker process %d started (delay: %d seconds)\n", pid, delay);
    
    int count = 0;
    while (1) {
        sleep(delay * 1000);
        printf("Worker %d: iteration %d\n", pid, ++count);
    }
}

int main(int argc, char** argv) {
    printf("=== Process Management Test ===\n");
    
    // Show current process
    printf("Current PID: %d\n", getMyPid());
    
    // List all processes
    printf("\nInitial process list:\n");
    listProcesses();
    
    // Create some test processes
    printf("\nCreating test processes...\n");
    
    char* args1[] = {"worker1", "1"};
    char* args2[] = {"worker2", "3"};
    char* args3[] = {"worker3", "2"};
    
    int pid1 = createProcess(2, args1, test_worker, 1); // priority 1
    int pid2 = createProcess(2, args2, test_worker, 2); // priority 2  
    int pid3 = createProcess(2, args3, test_worker, 0); // priority 0
    
    printf("Created processes: %d, %d, %d\n", pid1, pid2, pid3);
    
    // Wait a bit
    printf("\nWaiting 5 seconds...\n");
    sleep(5000);
    
    // Show process list again
    printf("\nProcess list after creation:\n");
    listProcesses();
    
    // Test priority change
    printf("\nChanging process %d priority to 3...\n", pid1);
    setProcessPriority(pid1, 3);
    
    // Test blocking
    printf("Blocking process %d...\n", pid2);
    blockProcess(pid2);
    
    // Wait a bit more
    printf("\nWaiting 3 seconds...\n");
    sleep(3000);
    
    // Show final process list
    printf("\nFinal process list:\n");
    listProcesses();
    
    // Unblock the process
    printf("\nUnblocking process %d...\n", pid2);
    unblockProcess(pid2);
    
    // Kill one process
    printf("Killing process %d...\n", pid3);
    killProcess(pid3);
    
    // Final process list
    printf("\nProcess list after killing %d:\n", pid3);
    listProcesses();
    
    printf("\nTest completed! Processes will continue running...\n");
    printf("Use 'ps' command in shell to see them, or 'kill <pid>' to stop them.\n");
    
    return 0;
}
*/