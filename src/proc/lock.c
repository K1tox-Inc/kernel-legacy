#include <proc/lock.h>

volatile uint32_t lock_counter = 0;

void spin_lock(spin_lock_t *lock)
{
	lock_irq(); // Contains the memory barrier via CLI clobber
	*lock = 1;
	/* On Single-Core, disabling IRQs ensures mutual exclusion
	 * For Multi-Core, need an atomic Test-And-Set loop here :
	 *
	 * while(*lock)
	 *  Attomic ops;
	 *
	 *
	 */
}

// Also the memory barrier currently inside lock_irq and unlock_irq
// they will be move inside the functions spins if multi core is implemented

void spin_unlock(spin_lock_t *lock)
{
	*lock = 0;
	unlock_irq(); // Contains the memory barrier via STI clobber
}
