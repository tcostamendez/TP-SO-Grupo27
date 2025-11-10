// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "buddy_system_mm.h"
#include "fd.h"
#include "first_fit_mm.h"
#include "memory_manager.h"
#include "pipe.h"
#include "sem.h"
#include "syscall_numbers.h"
#include <fonts.h>
#include <keyboard.h>
#include <lib.h>
#include <process.h>
#include <sound.h>
#include <stddef.h>
#include <syscallDispatcher.h>
#include <time.h>
#include <video.h>
#ifndef SECONDS_TO_TICKS
#include "time.h"
#ifndef SECONDS_TO_TICKS
#define SECONDS_TO_TICKS 18
#endif
#endif
extern int64_t register_snapshot[18];
extern int64_t register_snapshot_taken;

int32_t syscallDispatcher(Registers *registers) {
  switch (registers->rax) {
  case SYS_READ:
    return sys_read(registers->rdi, (signed char *)registers->rsi,
                    registers->rdx);
  case SYS_WRITE:
    return sys_write(registers->rdi, (char *)registers->rsi, registers->rdx);
  case SYS_START_BEEP:
    return sys_start_beep(registers->rdi);
  case SYS_STOP_BEEP:
    return sys_stop_beep();
  case SYS_FONTS_TEXT_COLOR:
    return sys_fonts_text_color(registers->rdi);
  case SYS_FONTS_BACKGROUND_COLOR:
    return sys_fonts_background_color(registers->rdi);
  case SYS_FONTS_DECREASE_SIZE:
    return sys_fonts_decrease_size();
  case SYS_FONTS_INCREASE_SIZE:
    return sys_fonts_increase_size();
  case SYS_FONTS_SET_SIZE:
    return sys_fonts_set_size((uint8_t)registers->rdi);
  case SYS_CLEAR_SCREEN:
    return sys_clear_screen();
  case SYS_CLEAR_INPUT_BUFFER:
    return sys_clear_input_buffer();
  case SYS_HOUR:
    return sys_hour((int *)registers->rdi);
  case SYS_MINUTE:
    return sys_minute((int *)registers->rdi);
  case SYS_SECOND:
    return sys_second((int *)registers->rdi);

  case SYS_CIRCLE:
    return sys_circle(registers->rdi, registers->rsi, registers->rdx,
                      registers->rcx);
  case SYS_RECTANGLE:
    return sys_rectangle(registers->rdi, registers->rsi, registers->rdx,
                         registers->rcx, registers->r8);
  case SYS_FILL_VIDEO_MEMORY:
    return sys_fill_video_memory(registers->rdi);

  case SYS_EXEC:
    return sys_exec((int (*)(void))registers->rdi);

  case SYS_REGISTER_KEY:
    return sys_register_key((uint8_t)registers->rdi,
                            (SpecialKeyHandler)registers->rsi);

  case SYS_WINDOW_WIDTH:
    return sys_window_width();
  case SYS_WINDOW_HEIGHT:
    return sys_window_height();

  case SYS_SLEEP_MILIS:
    return sys_sleep_milis(registers->rdi);

  case SYS_GET_REGISTER_SNAPSHOT:
    return sys_get_register_snapshot((int64_t *)registers->rdi);

  case SYS_GET_CHARACTER_NO_DISPLAY:
    return sys_get_character_without_display();
  case SYS_MALLOC:
    return (uint64_t)sys_malloc(registers->rdi);
  case SYS_FREE:
    sys_free((void *)registers->rdi);
    return 0;
  case SYS_MM_STATS:
    sys_get_memory_stats((int *)registers->rdi, (int *)registers->rsi,
                         (int *)registers->rdx);
    return 0;
  case SYS_CREATE_PROCESS:
    return sys_create_process((int)registers->rdi, (char **)registers->rsi,
                              (ProcessEntryPoint)registers->rdx,
                              (int)registers->rcx, (int *)registers->r8,
                              (int)registers->r9);
  case SYS_GET_PID:
    return sys_get_pid();
  case SYS_KILL_PROCESS:
    return sys_kill((int)registers->rdi);
  case SYS_MODIFY_PRIORITY:
    sys_modify_priority((int)registers->rdi, registers->rsi);
    return 0;
  case SYS_PS:
    return sys_ps((ProcessInfo *)registers->rdi);
  case SYS_BLOCK_PROCESS:
    return sys_block_process((int)registers->rdi);
  case SYS_UNBLOCK_PROCESS:
    return sys_unblock_process((int)registers->rdi);
  case SYS_YIELD:
    sys_yield();
    return 0;
  case SYS_WAIT_PID:
    return sys_wait_pid((int)registers->rdi);
  case SYS_WAIT_FOR_CHILDREN:
    return sys_wait_for_children();
  case SYS_PIPE_OPEN:
    return sys_pipe_open();
  case SYS_PIPE_CLOSE:
    return sys_pipe_close((uint8_t)registers->rdi);
  case SYS_SET_READ_TARGET:
    return sys_set_read_target_sys((uint8_t)registers->rdi);
  case SYS_SET_WRITE_TARGET:
    return sys_set_write_target_sys((uint8_t)registers->rdi);
  case SYS_SEM_OPEN:
    return (int64_t)sys_sem_open((const char *)registers->rdi,
                                 (uint16_t)registers->rsi);
  case SYS_SEM_CLOSE:
    return sys_sem_close((Sem)registers->rdi);
  case SYS_SEM_WAIT:
    return sys_sem_wait((Sem)registers->rdi);
  case SYS_SEM_POST:
    return sys_sem_post((Sem)registers->rdi);
  case SYS_SHUTDOWN:
    extern void outw(uint16_t port, uint16_t val);
    outw(0x604, 0x2000); // QEMU ACPI shutdown
    return 0;
  case SYS_SET_MVAR_VALUE:
    sys_set_mvar_value((char)registers->rdi);
    return 0;
  case SYS_GET_MVAR_VALUE:
    return sys_get_mvar_value();
  default:
    return 0;
  }
}

// ======================
// Linux syscalls
// ======================

int32_t sys_write(int32_t fd, char *__user_buf, int32_t count) {
  // Route through FD layer for transparent pipe/terminal behavior
  return fd_write(fd, (const uint8_t *)__user_buf, count);
}

int32_t sys_read(int32_t fd, signed char *__user_buf, int32_t count) {
  // Route through FD layer: reads from STDIN (keyboard) or pipe depending on
  // target
  return fd_read(fd, (uint8_t *)__user_buf, count);
}

// ======================
// Custom system calls
// ======================

int32_t sys_start_beep(uint32_t nFrequence) {
  play_sound(nFrequence);
  return 0;
}

int32_t sys_stop_beep(void) {
  setSpeaker(SPEAKER_OFF);
  return 0;
}

int32_t sys_fonts_text_color(uint32_t color) {
  setTextColor(color);
  return 0;
}

int32_t sys_fonts_background_color(uint32_t color) {
  setBackgroundColor(color);
  return 0;
}

int32_t sys_fonts_decrease_size(void) { return decreaseFontSize(); }

int32_t sys_fonts_increase_size(void) { return increaseFontSize(); }

int32_t sys_fonts_set_size(uint8_t size) { return setFontSize(size); }

int32_t sys_clear_screen(void) {
  clear();
  return 0;
}

int32_t sys_clear_input_buffer(void) {
  while (clearBuffer() != 0)
    ;
  return 0;
}

uint16_t sys_window_width(void) { return getWindowWidth(); }

uint16_t sys_window_height(void) { return getWindowHeight(); }

// ======================
// Date system calls
// ======================

int32_t sys_hour(int *hour) {
  *hour = getHour();
  return 0;
}

int32_t sys_minute(int *minute) {
  *minute = getMinute();
  return 0;
}

int32_t sys_second(int *second) {
  *second = getSecond();
  return 0;
}

// ======================
// Draw system calls
// ======================

int32_t sys_circle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY,
                   uint64_t diameter) {
  drawCircle(hexColor, topLeftX, topLeftY, diameter);
  return 0;
}

int32_t sys_rectangle(uint32_t color, uint64_t width_pixels,
                      uint64_t height_pixels, uint64_t initial_pos_x,
                      uint64_t initial_pos_y) {
  drawRectangle(color, width_pixels, height_pixels, initial_pos_x,
                initial_pos_y);
  return 0;
}

int32_t sys_fill_video_memory(uint32_t hexColor) {
  fillVideoMemory(hexColor);
  return 0;
}

// ======================
// Custom exec system call
// ======================

int32_t sys_exec(int32_t (*fnPtr)(void)) {
  clear();

  uint8_t fontSize = getFontSize();                 // preserve font size
  uint32_t text_color = getTextColor();             // preserve text color
  uint32_t background_color = getBackgroundColor(); // preserve background color

  SpecialKeyHandler map[F12_KEY - ESCAPE_KEY + 1] = {0};
  clearKeyFnMapNonKernel(
      map); // avoid """processes/threads/apps""" registering keys across each
            // other over time. reset the map every time

  int32_t aux = fnPtr();

  restoreKeyFnMapNonKernel(map);
  setFontSize(fontSize);
  setTextColor(text_color);
  setBackgroundColor(background_color);

  clear();
  return aux;
}

// ======================
// Custom keyboard system calls
// ======================

int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn) {
  registerSpecialKey(scancode, fn, 0);
  return 0;
}

// ======================
// Sleep system calls
// ======================
int32_t sys_sleep_milis(uint32_t milis) {
  sleepTicks((milis * SECONDS_TO_TICKS) / 1000);
  return 0;
}

// ======================
// Register snapshot system calls
// ======================
int32_t sys_get_register_snapshot(int64_t *registers) {
  if (register_snapshot_taken == 0)
    return 0;

  uint8_t i = 0;

  while (i < 18) {
    *(registers++) = register_snapshot[i++];
  }

  return 1;
}

int32_t sys_get_character_without_display(void) {
  return getKeyboardCharacter(0);
}
// ======================
// Memory management system calls
// ======================

void *sys_malloc(size_t size) { return (void *)mm_alloc(size); }

void sys_free(void *ap) { mm_free(ap); }

void sys_get_memory_stats(int *total, int *available, int *used) {
  MemoryStats aux = mm_get_stats();
  *total = aux.total_memory;
  *available = aux.free_memory;
  *used = aux.occupied_memory;
}

// ======================
// Process system calls
// ======================

int sys_create_process(int argc, char **argv, ProcessEntryPoint entryPoint,
                       int priority, int targets[], int hasForeground) {
  Process *p =
      create_process(argc, argv, entryPoint, priority, targets, hasForeground);
  if (p == NULL) {
    return -1;
  }
  return get_pid(p);
}

int sys_get_pid(void) { return get_running_pid(); }

int sys_kill(int pid) { return kill_process(pid); }

void sys_modify_priority(int pid, int new_priority) {
  set_priority(pid, new_priority);
}

int sys_ps(ProcessInfo *process_info) { return ps(process_info); }

int sys_block_process(int pid) {
  Process *p = get_process(pid);
  if (p == NULL || p == idle_proc || p->state == TERMINATED) {
    return -1;
  }

  if (p->state == BLOCKED) {
    unblock_process(p);
  } else if (p->state == RUNNING || p->state == READY) {
    block_process(p);
  }
  return 0;
}


int sys_unblock_process(int pid) {
  Process *p = get_process(pid);
  if (p == NULL) {
    return -1;
  }
  return unblock_process(p);
}

void sys_yield() { yield_cpu(); }

int sys_wait_pid(int pid) { return wait_child(pid); }

int sys_wait_for_children() { return wait_all_children(); }

int sys_get_process_info(ProcessInfo *info, int pid) {
  return get_process_info(info, pid);
}

// ======================
// Pipe syscalls
// ======================
int sys_pipe_open(void) { return openPipe(); }


int sys_pipe_close(uint8_t id) { return closePipe(id); }

int sys_set_read_target_sys(uint8_t id) {
  Process *p = get_current_process();
  if (!p)
    return -1;
  uint8_t old = p->targetByFd[READ_FD];
  if (old != STDIN && old != STDOUT) {
    detachReader(old, p->pid);
  }
  if (id != STDIN && id != STDOUT) {
    if (attachReader(id) < 0)
      return -1;
  }
  return setReadTarget(p->targetByFd, id);
}

int sys_set_write_target_sys(uint8_t id) {
  Process *p = get_current_process();
  if (!p)
    return -1;
  uint8_t old = p->targetByFd[WRITE_FD];
  if (old != STDIN && old != STDOUT) {
    detachWriter(old, p->pid);
  }
  if (id != STDIN && id != STDOUT) {
    if (attachWriter(id) < 0)
      return -1;
  }
  return setWriteTarget(p->targetByFd, id);
}

// ======================
// Semaphore syscalls
// ====================== 
Sem sys_sem_open(const char *name, uint16_t value) {
  return semOpen(name, value);
}

int sys_sem_close(Sem sem) { return semClose(sem); }

int sys_sem_wait(Sem sem) { return semWait(sem); }

int sys_sem_post(Sem sem) { return semPost(sem); }

// ======================
// Shared MVar syscalls
// ======================
void sys_set_mvar_value(char value) { set_mvar_value(value); }

char sys_get_mvar_value(void) { return get_mvar_value(); }