#pragma once

#include <types.h>

typedef volatile uint8_t preempt_lock;

void lock_preempt(preempt_lock *lock);
void unlock_preempt(preempt_lock *lock);

static __always_inline void lock_irq(void) { __asm__ volatile("cli" ::: "memory"); }

static __always_inline void unlock_irq(void) { __asm__ volatile("sti" ::: "memory"); }
