#pragma once

#include <arch/register.h>
#include <types.h>

typedef struct {
	REGISTERS regs;

	uint32_t int_no;
	uint32_t err_code;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

	uint32_t user_esp;
	uint32_t user_ss;

} __attribute__((packed)) trap_frame_t;
