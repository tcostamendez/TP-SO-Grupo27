#include "string.h"
#include "libsys/libsys.h"
#include "stdio.h"

static int count_digits(int num) {
    if (num == 0) return 1;
    int count = 0;
    if (num < 0) {
        count = 1;
        num = -num;
    }
    while (num > 0) {
        count++;
        num /= 10;
    }
    return count;
}

static void print_spaces(int count) {
    for (int i = 0; i < count; i++) {
        printf(" ");
    }
}

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

    // First pass: calculate maximum widths
    int max_pid_width = 3;  // At least "PID"
    int max_ppid_width = 4; // At least "PPID"
    int max_name_width = 4; // At least "Name"
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        // pid == -1 means uninitialized slot, skip it
        if (process_info[i].pid >= 0 && process_info[i].state != TERMINATED) {
            int pid_width = count_digits(process_info[i].pid);
            int ppid_width = count_digits(process_info[i].ppid);
            int name_len = strlen(process_info[i].name);
            
            if (pid_width > max_pid_width) max_pid_width = pid_width;
            if (ppid_width > max_ppid_width) max_ppid_width = ppid_width;
            if (name_len > max_name_width) max_name_width = name_len;
        }
    }

    // Print header
    printf("\e[0;32mPID");
    print_spaces(max_pid_width -1);
    printf("PPID");
    print_spaces(max_ppid_width -2);
    printf("Name");
    print_spaces(max_name_width -2);
    printf("State      Priority  FG/BG\n\e[0m");
    
    // Print separator line
    for (int i = 0; i < max_pid_width + max_ppid_width + max_name_width + 35; i++) {
        printf("-");
    }
    printf("\n");

    // Print process info
    for (int i = 0; i < MAX_PROCESSES; i++) {
        // pid == -1 means uninitialized slot, skip it
        // pid >= 0 includes the idle process (pid = 0)
        if (process_info[i].pid >= 0 && process_info[i].state != TERMINATED) {
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
            
            // Print PID with padding
            printf("%d", process_info[i].pid);
            print_spaces(max_pid_width - count_digits(process_info[i].pid) + 2);
            
            // Print PPID with padding
            printf("%d", process_info[i].ppid);
            print_spaces(max_ppid_width - count_digits(process_info[i].ppid) + 2);
            
            // Print Name with padding
            printf("%s", process_info[i].name);
            print_spaces(max_name_width - strlen(process_info[i].name) + 2);
            
            // Print State with padding (fixed width)
            printf("%s", state_str);
            print_spaces(11 - strlen(state_str));
            
            // Print Priority with padding
            printf("%d", process_info[i].priority);
            print_spaces(10 - count_digits(process_info[i].priority));
            
            // Print FG/BG
            printf("%s\n", fg_bg);
        }
    }

    return 0;
}
