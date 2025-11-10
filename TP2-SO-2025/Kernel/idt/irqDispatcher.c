// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <keyboard.h>
#include <stdint.h>
#include <time.h>
#include <video.h>
static uint8_t int_20(void);
static uint8_t int_21(void);

static uint8_t (*interruptions[])(void) = {int_20, int_21};

uint8_t irqDispatcher(uint64_t irq) {
	if (irq < 2) {
		return interruptions[irq]();
	}
	return 0;
}

static uint8_t int_20(void) {
	timer_handler();
	return 0;
}

static uint8_t int_21(void) { return keyboardHandler(); }
