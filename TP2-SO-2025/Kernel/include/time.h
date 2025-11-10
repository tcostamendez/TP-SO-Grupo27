#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>

#define SECONDS_TO_TICKS 18

/**
 * @brief Timer interrupt handler (irq0).
 */
void timer_handler();
/**
 * @brief Get ticks elapsed since boot.
 * @return Number of timer ticks.
 */
int ticks_elapsed();
/**
 * @brief Get seconds elapsed since boot.
 * @return Seconds since boot.
 */
int seconds_elapsed();
/**
 * @brief Busy-wait sleep for a number of seconds.
 * @param seconds Seconds to sleep.
 */
void sleep(int seconds);
/**
 * @brief Busy-wait sleep for a number of ticks.
 * @param sleep_t Ticks to sleep.
 */
void sleepTicks(uint64_t sleep_t);

#endif
