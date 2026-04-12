#pragma once

#include <types.h>

#define PIT_BASE_FREQUENCY 1193180
#define TIMER_HZ           100
#define DEFAULT_QUANTUM    10
#define TICKS_PER_SECOND   TIMER_HZ

#define TICKS_TO_SECONDS(ticks)   (ticks / TICKS_PER_SECOND)
#define SECONDS_TO_TICKS(seconds) (seconds * TICKS_PER_SECOND)

struct trap_frame;

void     timer_set_cycle(int hz);
void     timer_handle(struct trap_frame *frame);
void     timer_ksleep(uint32_t seconds);
uint32_t timer_get_global_ticks(void);
