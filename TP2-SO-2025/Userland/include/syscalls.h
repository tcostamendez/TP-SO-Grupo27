#ifndef _LIBC_SYSCALLS_H_
#define _LIBC_SYSCALLS_H_

#include <stdint.h>
#include "syscall_numbers.h"
#include "stdlib.h"

#define MAX_PROCESS_NAME 32

// Enum of registerable keys.
// Note: Does not include TAB or RETURN
enum REGISTERABLE_KEYS {
    ESCAPE_KEY        = 0x01,
    KEY_1             = 0x02,
    KEY_2             = 0x03,
    KEY_3             = 0x04,
    KEY_4             = 0x05,
    KEY_5             = 0x06,
    KEY_6             = 0x07,
    KEY_7             = 0x08,
    KEY_8             = 0x09,
    KEY_9             = 0x0A,
    KEY_0             = 0x0B,
    MINUS_KEY         = 0x0C,
    EQUALS_KEY        = 0x0D,
    BACKSPACE_KEY     = 0x0E,
    TABULATOR_KEY     = 0x0F,
    Q_KEY             = 0x10,
    W_KEY             = 0x11,
    E_KEY             = 0x12,
    R_KEY             = 0x13,
    T_KEY             = 0x14,
    Y_KEY             = 0x15,
    U_KEY             = 0x16,
    I_KEY             = 0x17,
    O_KEY             = 0x18,
    P_KEY             = 0x19,
    LEFT_BRACKET_KEY  = 0x1A,
    RIGHT_BRACKET_KEY = 0x1B,
    RETURN_KEY        = 0x1C,
    CONTROL_KEY_L     = 0x1D,
    A_KEY             = 0x1E,
    S_KEY             = 0x1F,
    D_KEY             = 0x20,
    F_KEY             = 0x21,
    G_KEY             = 0x22,
    H_KEY             = 0x23,
    J_KEY             = 0x24,
    K_KEY             = 0x25,
    L_KEY             = 0x26,
    SEMICOLON_KEY     = 0x27,
    APOSTROPHE_KEY    = 0x28,
    GRAVE_KEY         = 0x29,
    SHIFT_KEY_L       = 0x2A,
    BACKSLASH_KEY     = 0x2B,
    Z_KEY             = 0x2C,
    X_KEY             = 0x2D,
    C_KEY             = 0x2E,
    V_KEY             = 0x2F,
    B_KEY             = 0x30,
    N_KEY             = 0x31,
    M_KEY             = 0x32,
    COMMA_KEY         = 0x33,
    PERIOD_KEY        = 0x34,
    SLASH_KEY         = 0x35,
    SHIFT_KEY_R       = 0x36,
    KP_ASTERISK_KEY   = 0x37,
    ALT_KEY_L         = 0x38,
    SPACE_KEY         = 0x39,
    CAPS_LOCK_KEY     = 0x3A,
    F1_KEY            = 0x3B,
    F2_KEY            = 0x3C,
    F3_KEY            = 0x3D,
    F4_KEY            = 0x3E,
    F5_KEY            = 0x3F,
    F6_KEY            = 0x40,
    F7_KEY            = 0x41,
    F8_KEY            = 0x42,
    F9_KEY            = 0x43,
    F10_KEY           = 0x44,
    NUM_LOCK_KEY      = 0x45,
    SCROLL_LOCK_KEY   = 0x46,
    KP_HOME_KEY       = 0x47,
    KP_UP_KEY         = 0x48,
    KP_PAGE_UP_KEY    = 0x49,
    KP_MINUS_KEY      = 0x4A,
    KP_LEFT_KEY       = 0x4B,
    KP_BEGIN_KEY      = 0x4C,
    KP_RIGHT_KEY      = 0x4D,
    KP_PLUS_KEY       = 0x4E,
    KP_END_KEY        = 0x4F,
    KP_DOWN_KEY       = 0x50,
    KP_PAGE_DOWN_KEY  = 0x51,
    KP_INSERT_KEY     = 0x52,
    KP_DELETE_KEY     = 0x53,
    F11_KEY           = 0x57,
    F12_KEY           = 0x58
};

// Enumeración de los estados de un proceso
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED // Usaremos este estado para 'limpiar' procesos
} ProcessState;

typedef struct {
    size_t total_memory;
    size_t free_memory;
    size_t occupied_memory;
} MemoryStats;

// Límite de procesos que podemos tener.
#define MAX_PROCESSES 64
#define MAX_CHILDREN 32

// Nombre máximo para un proceso (debugging)
#define MAX_PROCESS_NAME 32

// Prioridades del proceso
#define MIN_PRIORITY 0
#define MAX_PRIORITY 3
#define DEFAULT_PRIORITY 0

typedef struct ProcessInfo {
    int pid;
    int ppid;
    char name[MAX_PROCESS_NAME];
    ProcessState state;
    uint64_t rsp;
    void *stackBase;
    int priority;
    int ground; 
} ProcessInfo;

// Linux syscall prototypes
int32_t sys_write(int64_t fd, const void * buf, int64_t count);
int32_t sys_read(int64_t fd, void * buf, int64_t count);

// Custom syscall prototypes
/* SYS_START_BEEP = 10 */
int32_t sys_start_beep(uint32_t nFrequence);
/* SYS_STOP_BEEP = 11 */
int32_t sys_stop_beep(void);
/* SYS_FONTS_TEXT_COLOR = 12 */
int32_t sys_fonts_text_color(uint32_t color);
/* SYS_FONTS_BACKGROUND_COLOR = 13 */
int32_t sys_fonts_background_color(uint32_t color);
/* SYS_FONTS_DECREASE_SIZE = 14 */
int32_t sys_fonts_decrease_size(void);
/* SYS_FONTS_INCREASE_SIZE = 15 */
int32_t sys_fonts_increase_size(void);
/* SYS_FONTS_SET_SIZE = 16 */
int32_t sys_fonts_set_size(uint8_t size);
/* SYS_CLEAR_SCREEN = 17 */
int32_t sys_clear_screen(void);
/* SYS_CLEAR_INPUT_BUFFER = 18 */
int32_t sys_clear_input_buffer(void);

// Date syscall prototypes
/* SYS_HOUR = 19 */
int32_t sys_hour(int * hour);
/* SYS_MINUTE = 20 */
int32_t sys_minute(int * minute);
/* SYS_SECOND = 21 */
int32_t sys_second(int * second);

int32_t sys_circle(int color, long long int topleftX, long long int topLefyY, long long int diameter);

int32_t sys_rectangle(int color, long long int width_pixels, long long int height_pixels, long long int initial_pos_x, long long int initial_pos_y);

int32_t sys_fill_video_memory(uint32_t hexColor);

int32_t sys_exec(int32_t (*fnPtr)(void));

int32_t sys_register_key(uint8_t scancode, void (*fn)(enum REGISTERABLE_KEYS scancode));

int32_t sys_window_width(void);

int32_t sys_window_height(void);

int32_t sys_sleep_milis(uint32_t milis);

int32_t sys_get_register_snapshot(int64_t * registers);

int32_t sys_get_character_without_display(void);

void * sys_malloc(uint64_t size);

void sys_free(void * ap);

void sys_get_memory_stats(int * total, int * avaliable, int * used);

// Process management syscalls (34-43)
int sys_create_process(int argc, char** argv, void (*entry_point)(int, char**), int priority, int * targets, int hasForeground);
int sys_get_pid(void);
int sys_kill_process(int pid);
void sys_modify_priority(int pid, int new_priority);
int sys_ps(ProcessInfo* process_info);
int sys_block_process(int pid);
int sys_unblock_process(int pid);
void sys_yield(void);
int sys_wait_pid(int pid);
int sys_wait_for_children(void);
int sys_get_process_info(ProcessInfo* info, int pid);

// Pipe syscalls (44-48)
int sys_pipe_open(void);
int sys_pipe_close(uint8_t id);
int sys_set_read_target(uint8_t id);
int sys_set_write_target(uint8_t id);

// Semaphore syscalls (49-52)
void * sys_sem_open(const char *name, uint16_t value);
int sys_sem_close(void *sem);
int sys_sem_wait(void *sem);
int sys_sem_post(void *sem);

// Shutdown syscall (53)
void sys_shutdown(void);

// Shared MVar value storage (minimal kernel support for userland MVar)
void sys_set_mvar_value(char value);
char sys_get_mvar_value(void);

#endif