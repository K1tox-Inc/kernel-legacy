#pragma once

#include <types.h>

typedef volatile uint8_t spin_lock_t;

extern volatile uint32_t lock_counter;

void spin_lock(spin_lock_t *lock);
void spin_unlock(spin_lock_t *lock);

static inline void lock_irq(void)
{
	__asm__ volatile("cli");
	lock_counter++;
};

static inline void unlock_irq(void)
{
	if (lock_counter)
		lock_counter--;
	if (!lock_counter)
		__asm__ volatile("sti");
};
