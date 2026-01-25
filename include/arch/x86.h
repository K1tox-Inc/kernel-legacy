#pragma once

#include <arch/trap_frame.h>
#include <types.h>

typedef struct {
	uint16_t  limit;
	uintptr_t base;
} __attribute__((packed)) gdtr_t;

typedef struct {
	uint16_t  limit;
	uintptr_t base;
} __attribute__((packed)) idtr_t;

typedef void (*irqHandler)(trap_frame_t *frame);

void          x86_init(void);
const gdtr_t *get_gdtr(void);
const idtr_t *get_idtr(void);
void          gdt_init(void);
void          idt_init(void);
void          idt_register_interrupt_handlers(uint8_t num, irqHandler handler);
