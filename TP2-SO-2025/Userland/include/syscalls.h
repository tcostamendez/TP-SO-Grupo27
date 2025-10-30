#ifndef _LIBC_SYSCALLS_H_
#define _LIBC_SYSCALLS_H_

#include <stdint.h>
#include <sys.h>

// Linux syscall prototypes
int32_t sys_write(int64_t fd, const void * buf, int64_t count);
int32_t sys_read(int64_t fd, void * buf, int64_t count);

// Custom syscall prototypes
/* 0x80000000 */
int32_t sys_start_beep(uint32_t nFrequence);
/* 0x80000001 */
int32_t sys_stop_beep(void);
/* 0x80000002 */
int32_t sys_fonts_text_color(uint32_t color);
/* 0x80000003 */
int32_t sys_fonts_background_color(uint32_t color);
/* 0x80000007 */
int32_t sys_fonts_decrease_size(void);
/* 0x80000008 */
int32_t sys_fonts_increase_size(void);
/* 0x80000009 */
int32_t sys_fonts_set_size(uint8_t size);
/* 0x8000000A */
int32_t sys_clear_screen(void);
/* 0x8000000B */
int32_t sys_clear_input_buffer(void);

// Date syscall prototypes
/* 0x80000010 */
int32_t sys_hour(int * hour);
/* 0x80000011 */
int32_t sys_minute(int * minute);
/* 0x80000012 */
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

// Process management syscalls (0x80000102-0x8000010B)
int sys_create_process(int argc, char** argv, void (*entry_point)(int, char**), int priority);
int sys_get_pid(void);
int sys_kill_process(int pid);
void sys_modify_priority(int pid, int new_priority);
void sys_list_processes(void);
void sys_block_process(int pid);
void sys_unblock_process(int pid);
void sys_yield(void);
int sys_wait_pid(int pid);
int sys_wait_for_children(void);

// Pipe syscalls (0x80000110 - 0x80000114)
int sys_pipe_open(void);
int sys_pipe_attach(uint8_t id);
int sys_pipe_close(uint8_t id);
int sys_set_read_target(uint8_t id);
int sys_set_write_target(uint8_t id);

// Semaphore syscalls (0x80000120 - 0x80000123)
void * sys_sem_open(const char *name, uint16_t value);
int sys_sem_close(void *sem);
int sys_sem_wait(void *sem);
int sys_sem_post(void *sem);

#endif