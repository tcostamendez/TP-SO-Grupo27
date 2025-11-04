#include <fonts.h>
#include <keyboard.h>
#include <lib.h>
#include <sound.h>
#include <stddef.h>
#include <syscallDispatcher.h>
#include <time.h>
#include <video.h>
#include <process.h>
//#include "first_fit_mm.h"
#include "buddy_system_mm.h"
#include "fd.h"
#include "pipe.h"
#include "sem.h"
#ifndef SECONDS_TO_TICKS
#include "time.h"
#ifndef SECONDS_TO_TICKS
#define SECONDS_TO_TICKS 18
#endif
#endif
extern int64_t register_snapshot[18];
extern int64_t register_snapshot_taken;

// @todo Note: Technically.. registers on the stack are modifiable (since its a
// struct pointer, not struct).
int32_t syscallDispatcher(Registers *registers) {
  switch (registers->rax) {
  case 3:
    return sys_read(registers->rdi, (signed char *)registers->rsi,
                    registers->rdx);
  // Note: Register parameters are 64-bit
  case 4:
    return sys_write(registers->rdi, (char *)registers->rsi, registers->rdx);

  case 0x80000000:
    return sys_start_beep(registers->rdi);
  case 0x80000001:
    return sys_stop_beep();
  case 0x80000002:
    return sys_fonts_text_color(registers->rdi);
  case 0x80000003:
    return sys_fonts_background_color(registers->rdi);
  case 0x80000004: /* Reserved for sys_set_italics */
  case 0x80000005: /* Reserved for sys_set_bold */
  case 0x80000006: /* Reserved for sys_set_underline */
    return -1;
  case 0x80000007:
    return sys_fonts_decrease_size();
  case 0x80000008:
    return sys_fonts_increase_size();
  case 0x80000009:
    return sys_fonts_set_size((uint8_t)registers->rdi);
  case 0x8000000A:
    return sys_clear_screen();
  case 0x8000000B:
    return sys_clear_input_buffer();

  case 0x80000010:
    return sys_hour((int *)registers->rdi);
  case 0x80000011:
    return sys_minute((int *)registers->rdi);
  case 0x80000012:
    return sys_second((int *)registers->rdi);

  case 0x80000019:
    return sys_circle(registers->rdi, registers->rsi, registers->rdx,
                      registers->rcx);
  case 0x80000020:
    return sys_rectangle(registers->rdi, registers->rsi, registers->rdx,
                         registers->rcx, registers->r8);
  case 0x80000021:
    return sys_fill_video_memory(registers->rdi);

  case 0x800000A0:
    return sys_exec((int (*)(void))registers->rdi);

  case 0x800000B0:
    return sys_register_key((uint8_t)registers->rdi,
                            (SpecialKeyHandler)registers->rsi);

  case 0x800000C0:
    return sys_window_width();
  case 0x800000C1:
    return sys_window_height();

  case 0x800000D0:
    return sys_sleep_milis(registers->rdi);

  case 0x800000E0:
    return sys_get_register_snapshot((int64_t *)registers->rdi);

  case 0x800000F0:
    return sys_get_character_without_display();
  
  /* ----------------------------------------------------------------------------------------------------------- */
  case 0x80000100:
      return (uint32_t) sys_malloc(registers->rdi);
  case 0x80000101:
      sys_free((void*)registers->rdi);
      return 0;
  case 0x80000102:
      return sys_create_process((int)registers->rdi, (char**)registers->rsi, (ProcessEntryPoint)registers->rdx, (int)registers->rcx, (int*)registers->r8, (int)registers->r9);
  case 0x80000103:
      return sys_get_pid();
  case 0x80000104:
      return sys_kill((int)registers->rdi);
  case 0x80000105:
      sys_modify_priority((int)registers->rdi, registers->rsi);
      return 0;
  case 0x80000106:
      sys_print_processes();
      return 0;
  case 0x80000107:
      sys_block_process((int)registers->rdi);
      return 0;
  case 0x80000108:
      sys_unblock_process((int)registers->rdi);
      return 0;
  case 0x80000109:
      sys_yield();
      return 0;
  case 0x8000010A:
      return sys_wait_pid((int)registers->rdi);
  case 0x8000010B:
      return sys_wait_for_children();
  case 0x8000010C:
      return sys_get_process_info((ProcessInfo*)registers->rdi, (int)registers->rsi);    
  case 0x80000110:
    return sys_pipe_open();
  case 0x80000111:
    return sys_pipe_attach((uint8_t)registers->rdi);
  case 0x80000112:
    return sys_pipe_close((uint8_t)registers->rdi);
  case 0x80000113:
    return sys_set_read_target_sys((uint8_t)registers->rdi);
  case 0x80000114:
    return sys_set_write_target_sys((uint8_t)registers->rdi);
  case 0x80000120:
    return (int64_t)sys_sem_open((const char*)registers->rdi, (uint16_t)registers->rsi);
  case 0x80000121:
    return sys_sem_close((Sem)registers->rdi);
  case 0x80000122:
    return sys_sem_wait((Sem)registers->rdi);
  case 0x80000123:
    return sys_sem_post((Sem)registers->rdi);
  default:
    return 0;
  }
}

// ==================================================================
// Linux syscalls
// ==================================================================

int32_t sys_write(int32_t fd, char *__user_buf, int32_t count) {
  // Route through FD layer for transparent pipe/terminal behavior
  return fd_write(fd, (const uint8_t *)__user_buf, count);
}

int32_t sys_read(int32_t fd, signed char *__user_buf, int32_t count) {
  // Route through FD layer: reads from STDIN (keyboard) or pipe depending on target
  return fd_read(fd, (uint8_t *)__user_buf, count);
}

// ==================================================================
// Custom system calls
// ==================================================================

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

// ==================================================================
// Date system calls
// ==================================================================

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

// ==================================================================
// Draw system calls
// ==================================================================

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

// ==================================================================
// Custom exec system call
// ==================================================================

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

// ==================================================================
// Custom keyboard system calls
// ==================================================================

int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn) {
  registerSpecialKey(scancode, fn, 0);
  return 0;
}

// ==================================================================
// Sleep system calls
// ==================================================================
int32_t sys_sleep_milis(uint32_t milis) {
  sleepTicks((milis * SECONDS_TO_TICKS) / 1000);
  return 0;
}

// ==================================================================
// Register snapshot system calls
// ==================================================================
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
// ==================================================================
// Memory managment system calls
// ==================================================================

void * sys_malloc(size_t size) {
  //! REVISAR CASTEO
  return (void *) mm_alloc(size);
}

void sys_free(void* ap){
  mm_free(ap);
}

// ==================================================================
// Process system calls
// ==================================================================

int sys_create_process(int argc, char ** argv, ProcessEntryPoint entryPoint, int priority, int targets[], int hasForeground){
  Process* p = create_process(argc, argv, entryPoint, priority, targets, hasForeground);
  if(p == NULL){
    return -1;
  }
  return get_pid(p);
}

int sys_get_pid() {
  return get_running_pid();
}

int sys_kill(int pid) {
  return kill_process(pid);
}

void sys_modify_priority(int pid, int new_priority){
  set_priority(pid, new_priority);
}

void sys_print_processes() {
    print("NAME         | PID | PRIO | FG/BG | STACK_PTR (DEC)      | BASE_PTR (DEC)\n");
    print("----------------------------------------------------------------------------\n");
    for (int j = 0; j < MAX_PROCESSES; j++) {
        Process* p = get_process(j); 
        if (p != NULL) {
            char* pid_str    = num_to_str(p->pid);
            char* prio_str   = num_to_str(p->priority);
            char* ground_str = num_to_str(p->ground);
            char* rsp_str    = num_to_str(p->rsp);
            char* rbp_str    = num_to_str(p->rbp);
            if (pid_str == NULL || prio_str == NULL || ground_str == NULL || rsp_str == NULL || rbp_str == NULL) {
                print("Error: Fallo de memoria al listar proceso ID: ");
                if (pid_str) print(pid_str); else print("?");
                print("\n");
                if (pid_str) mm_free(pid_str);
                if (prio_str) mm_free(prio_str);
                if (ground_str) mm_free(ground_str);
                if (rsp_str) mm_free(rsp_str);
                if (rbp_str) mm_free(rbp_str);
                
                continue; // Saltar al siguiente proceso
            }
            print("Process ");   print(pid_str); print("   | "); // NAME
            print(pid_str);      print(" | ");                   // PID
            print(prio_str);     print("  | ");                   // PRIO
            print(ground_str);       print("     | ");                   // FG/BG
            print(rsp_str);      print(" | ");                   // STACK
            print(rbp_str);                                       // BASE
            print("\n");                                          // Fin de línea
            mm_free(pid_str);
            mm_free(prio_str);
            mm_free(ground_str);
            mm_free(rsp_str);
            mm_free(rbp_str);
        }
    }
}

void sys_block_process(int pid){
  Process* p = get_process(pid);
  if (p != NULL) {
    block_process(p);
  }
}

void sys_unblock_process(int pid){
  Process* p = get_process(pid);
  if (p != NULL) {
    unblock_process(p);
  }
}

void sys_yield(){
  yield_cpu();
}

int sys_wait_pid(int pid){
  return wait_child(pid);
}

int sys_wait_for_children(){
  return wait_all_children();
}

int sys_get_process_info(ProcessInfo * info, pid){
  return get_process_info(info, pid);
}
// ==================================================================
// Pipe syscalls
// ==================================================================
int sys_pipe_open(void) {
  return openPipe();
}

int sys_pipe_attach(uint8_t id) {
  return attach(id);
}

int sys_pipe_close(uint8_t id) {
  return closePipe(id);
}

static void detach_if_pipe(uint8_t id, int pid) {
  if (id != STDIN && id != STDOUT) {
    removeAttached(id, pid);
  }
}

int sys_set_read_target_sys(uint8_t id) {
  Process *p = get_current_process();
  if (!p) return -1;
  uint8_t old = p->targetByFd[READ_FD];
  // detach old if it was a pipe
  detach_if_pipe(old, p->pid);
  // attach new if it is a pipe
  if (id != STDIN && id != STDOUT) {
    if (attach(id) < 0) return -1;
  }
  return setReadTarget(p->targetByFd, id);
}

int sys_set_write_target_sys(uint8_t id) {
  Process *p = get_current_process();
  if (!p) return -1;
  uint8_t old = p->targetByFd[WRITE_FD];
  detach_if_pipe(old, p->pid);
  if (id != STDIN && id != STDOUT) {
    if (attach(id) < 0) return -1;
  }
  return setWriteTarget(p->targetByFd, id);
}

// ==================================================================
// Semaphore syscalls
// ==================================================================
Sem sys_sem_open(const char *name, uint16_t value) {
  return semOpen(name, value);
}

int sys_sem_close(Sem sem) { return semClose(sem); }

int sys_sem_wait(Sem sem) { return semWait(sem); }

int sys_sem_post(Sem sem) { return semPost(sem); }