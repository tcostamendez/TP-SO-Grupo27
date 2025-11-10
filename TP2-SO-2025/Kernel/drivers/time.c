// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <interrupts.h>
#include <time.h>

#include <cursor.h>
#include <fonts.h>
#include <stdint.h>

extern uint8_t soft_irq0;

static unsigned long ticks = 0;

void timer_handler(void) {
  if (soft_irq0) {
    soft_irq0 = 0; 
    return;
  }
  ticks++;
  // toggleCursor();
}

int ticks_elapsed(void) { return ticks; }

int seconds_elapsed(void) { return ticks / SECONDS_TO_TICKS; }

void sleepTicks(uint64_t sleep_t) {
  unsigned long start = ticks;
  while (ticks < start + sleep_t)
    _hlt();
  return;
}

void sleep(int seconds) {
  sleepTicks(seconds * SECONDS_TO_TICKS);
  return;
}
