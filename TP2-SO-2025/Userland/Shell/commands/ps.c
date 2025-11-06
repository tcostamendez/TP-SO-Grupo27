#include "string.h"
#include "libsys/libsys.h"
#include "stdio.h"

int _ps(int argc, char *argv[]) {
    if (argc > 1) {
        perror("Usage: ps\n");
        return 1;
    }

    ProcessInfo process_info[MAX_PROCESSES] = {0};
    
    if (ps(process_info) != 0) {
        perror("Error: Failed to get process information\n");
        return -1;
    }

    // Print header
    printf("\e[0;32m%s %s %s %s %s %s\n\e[0m\n", 
           "PID", "PPID", "Name", "State", "Priority", "FG/BG");
    printf("-----------------------------------------------------------\n");

    // Print process info
    for (int i = 0; i < MAX_PROCESSES; i++) {
        // Only print active processes (pid > 0 and not terminated)
        if(i == 0){
            // Convert state to string
            const char *state_str;
            switch (process_info[i].state) {
                case READY:
                    state_str = "READY";
                    break;
                case RUNNING:
                    state_str = "RUNNING";
                    break;
                case BLOCKED:
                    state_str = "BLOCKED";
                    break;
                default:
                    state_str = "UNK";
            }
            
            // Convert foreground/background
            const char *fg_bg = (process_info[i].ground == 1) ? "FG" : "BG";
            
            printf(" %d   %d   %s %s %d %s\n",
                   process_info[i].pid,
                   process_info[i].ppid,
                   process_info[i].name,
                   state_str,
                   process_info[i].priority,
                   fg_bg);
        }


        if (process_info[i].pid > 0 && process_info[i].state != TERMINATED) {
            // Convert state to string
            const char *state_str;
            switch (process_info[i].state) {
                case READY:
                    state_str = "READY";
                    break;
                case RUNNING:
                    state_str = "RUNNING";
                    break;
                case BLOCKED:
                    state_str = "BLOCKED";
                    break;
                default:
                    state_str = "UNK";
            }
            
            // Convert foreground/background
            const char *fg_bg = (process_info[i].ground == 1) ? "FG" : "BG";
            
            printf(" %d   %d   %s %s %d %s\n",
                   process_info[i].pid,
                   process_info[i].ppid,
                   process_info[i].name,
                   state_str,
                   process_info[i].priority,
                   fg_bg);
        }
    }

    return 0;
}
