#include <types.h>

#define PIT_BASE_FREQUENCY 1193180
#define TIMER_HZ           100
#define DEFAULT_QUANTUM    10
#define TICKS_PER_SECOND   TIMER_HZ

struct trap_frame;

void timer_set_cycle(int hz);
void timer_handle(struct trap_frame *frame);
