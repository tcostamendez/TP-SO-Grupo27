#include <libsys.h>
#include <stddef.h>
#include <stdint.h>

void startBeep(uint32_t nFrequence) { sys_start_beep(nFrequence); }

void stopBeep(void) { sys_stop_beep(); }

void setTextColor(uint32_t color) { sys_fonts_text_color(color); }

void setBackgroundColor(uint32_t color) { sys_fonts_background_color(color); }

uint8_t increaseFontSize(void) { return sys_fonts_increase_size(); }

uint8_t decreaseFontSize(void) { return sys_fonts_decrease_size(); }

uint8_t setFontSize(uint8_t size) { return sys_fonts_set_size(size); }

void getDate(int *hour, int *minute, int *second) {
  sys_hour(hour);
  sys_minute(minute);
  sys_second(second);
}

void clearScreen(void) { sys_clear_screen(); }

// void clearInputBuffer(void) { sys_clear_input_buffer(); }

void drawCircle(uint32_t color, long long int topleftX, long long int topLefyY,
                long long int diameter) {
  sys_circle(color, topleftX, topLefyY, diameter);
}

void drawRectangle(uint32_t color, long long int width_pixels,
                   long long int height_pixels, long long int initial_pos_x,
                   long long int initial_pos_y) {
  sys_rectangle(color, width_pixels, height_pixels, initial_pos_x,
                initial_pos_y);
}

void fillVideoMemory(uint32_t hexColor) { sys_fill_video_memory(hexColor); }

int32_t exec(int32_t (*fnPtr)(void)) { return sys_exec(fnPtr); }

void registerKey(enum REGISTERABLE_KEYS scancode,
                 void (*fn)(enum REGISTERABLE_KEYS scancode)) {
  sys_register_key(scancode, fn);
}

int getWindowWidth(void) { return sys_window_width(); }

int getWindowHeight(void) { return sys_window_height(); }

void sleep(uint32_t miliseconds) { sys_sleep_milis(miliseconds); }

int32_t getRegisterSnapshot(int64_t *registers) {
  return sys_get_register_snapshot(registers);
}

int32_t getCharacterWithoutDisplay(void) {
  return sys_get_character_without_display();
}

void* allocMemory(size_t size) {
  return sys_malloc(size);
}

void freeMemory(void* ptr) {
  sys_free(ptr);
}

void memoryStats(int * total, int * available, int * used){
  sys_get_memory_stats(total, available, used);
}

int createProcess(int argc, char** argv, void (*entry_point)(int, char**), int priority, int * targets, int hasForeground) {
  return sys_create_process(argc, argv, entry_point, priority, targets, hasForeground);
}

int getMyPid(void) {
  return sys_get_pid();
}

int killProcess(int pid) {
  return sys_kill_process(pid);
}

void setProcessPriority(int pid, int priority) {
  sys_modify_priority(pid, priority);
}

int ps(ProcessInfo* process_info) {
  return sys_ps(process_info);
}

void blockProcess(int pid) {
  sys_block_process(pid);
}


void yieldCPU(void) {
  sys_yield();
}

int waitPid(int pid) {
  return sys_wait_pid(pid);
}

int waitForChildren(void) {
  return sys_wait_for_children();
}

int getProcessInfo(ProcessInfo* info, int pid){
  return sys_get_process_info(info, pid);
}

// ==========================
// Pipe wrappers
// ==========================
int pipeOpen(void) {
  return sys_pipe_open();
}

int pipeAttach(uint8_t id) {
  return sys_pipe_attach(id);
}

int pipeClose(uint8_t id) {
  return sys_pipe_close(id);
}

int setReadTarget(uint8_t id) {
  return sys_set_read_target(id);
}

int setWriteTarget(uint8_t id) {
  return sys_set_write_target(id);
}

// ==========================
// Semaphore wrappers
// ==========================
sem_t semOpen(const char *name, uint16_t initialValue) {
  return (sem_t)sys_sem_open(name, initialValue);
}

int semClose(sem_t s) { return sys_sem_close(s); }

int semWait(sem_t s) { return sys_sem_wait(s); }

int semPost(sem_t s) { return sys_sem_post(s); }

void shutdown(void) { sys_shutdown(); }