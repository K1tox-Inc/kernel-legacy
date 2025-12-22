#pragma once

#include <register.h>
#include <types.h>

typedef struct {
	uint32_t  gs;
	uint32_t  fs;
	uint32_t  es;
	REGISTERS to_remove_quickly_as_soon_possible;

	uint32_t int_no;
	uint32_t err_code;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

	uint32_t user_esp;
	uint32_t user_ss;

} __attribute__((packed)) trap_frame_t;
