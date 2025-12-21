#include <proc/lock.h>

volatile uint32_t lock_counter = 0;

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
