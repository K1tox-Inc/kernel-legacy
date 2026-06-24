#pragma once

#include <arch/register.h>
#include <types.h>
#include <utils/assert.h>

struct trap_frame {
	struct registers regs;

	uint32_t int_no;
	uint32_t err_code;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

	uint32_t user_esp;
	uint32_t user_ss;
} __packed;

// See `src/arch/x86/irq.s`
static_assert(offsetof(struct trap_frame, int_no) == 48,
              "Offset of `int_no` in `struct trap_frame` must be 48.");
