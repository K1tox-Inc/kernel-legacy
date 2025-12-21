#include <types.h>

typedef volatile uint8_t spin_lock_t;

static volatile uint32_t lock_counter = 0;

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

void spin_lock(spin_lock_t *lock)
{
	lock_irq();
	*lock = 1;
	/* On Single-Core, disabling IRQs ensures mutual exclusion
	 * For Multi-Core, need an atomic Test-And-Set loop here :
	 *
	 * while(*lock)
	 *  Attomic ops;
	 */
}

void spin_unlock(spin_lock_t *lock)
{
	*lock = 0;
	unlock_irq();
}
