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

///////////////////////////////////////////////////
// Gdt flags

enum Gdt_Access_Byte {
	ACCESS_BIT      = 0b00000001, // Bit 0: Indique si le segment a été accédé par le CPU
	RW_BIT          = 0b00000010, // Bit 1: Lecture/écriture pour données, lecture pour code
	DC_BIT          = 0b00000100, // Bit 2: Direction pour données, Conforming pour code
	EXECUTABLE_BIT  = 0b00001000, // Bit 3: 1=segment de code, 0=segment de données
	DESCRIPTOR_TYPE = 0b00010000, // Bit 4: 1=segment code/données, 0=segment système
	DPL_RING0       = 0b00000000, // Bits 5-6: Niveau noyau, privilèges maximaux
	DPL_RING1       = 0b00100000, // Bits 5-6: Niveau intermédiaire, rarement utilisé
	DPL_RING2       = 0b01000000, // Bits 5-6: Niveau intermédiaire, rarement utilisé
	DPL_RING3       = 0b01100000, // Bits 5-6: Niveau utilisateur, privilèges minimaux
	PRESENT_BIT     = 0b10000000  // Bit 7: Indique si le segment est présent en mémoire
};

#define GDT_IDX_NULL        0
#define GDT_IDX_KERNEL_CODE 1
#define GDT_IDX_KERNEL_DATA 2
#define GDT_IDX_USER_CODE   3
#define GDT_IDX_USER_DATA   4
#define GDT_IDX_TSS         5

#define GET_RING(dpl_bitmask) ((dpl_bitmask) >> 5)

#define USER_RING   GET_RING(DPL_RING3)
#define KERNEL_RING GET_RING(DPL_RING0)

#define GDT_SELECTOR(idx, ring) (((idx) << 3) | (ring))

#define USER_CS GDT_SELECTOR(GDT_IDX_USER_CODE, USER_RING) // 0x1B
#define USER_DS GDT_SELECTOR(GDT_IDX_USER_DATA, USER_RING) // 0x23

///////////////////////////////////////////////////
// Eflags

// look here https://wiki.osdev.org/CPU_Registers_x86 to get more info
// Status Flags
#define EFLAGS_CF    (1 << 0) // Carry Flag
#define EFLAGS_FIXED (1 << 1) // Reserved Bit
#define EFLAGS_PF    (1 << 2) // Parity Flag
#define EFLAGS_AF    (1 << 4) // Auxiliary
#define EFLAGS_ZF    (1 << 6) // Zero Flag
#define EFLAGS_SF    (1 << 7) // Sign Flag

// Control Flags
#define EFLAGS_TF (1 << 8)  // Trap Flag
#define EFLAGS_IF (1 << 9)  // Interrupt Flag
#define EFLAGS_DF (1 << 10) // Direction Flag
#define EFLAGS_OF (1 << 11) // Overflow Flag

// System Flags
#define EFLAGS_IOPL(x) ((x & 3) << 12) // I/O Privilege Level (must be 0)
#define EFLAGS_NT      (1 << 14)       // Nested Task
#define EFLAGS_RF      (1 << 16)       // Resume Flag
#define EFLAGS_VM      (1 << 17)       // Virtual 8086 Mode
#define EFLAGS_AC      (1 << 18)       // Alignment Check
#define EFLAGS_VIF     (1 << 19)       // Virtual Interrupt Flag
#define EFLAGS_VIP     (1 << 20)       // Virtual Interrupt Pending
#define EFLAGS_ID      (1 << 21)       // ID Flag

#define EFLAGS_USER_DEFAULT   (EFLAGS_FIXED | EFLAGS_IF)
#define EFLAGS_KERNEL_DEFAULT (EFLAGS_FIXED | EFLAGS_IF)

///////////////////////////////////////////////////
// Others

typedef void (*irqHandler)(trap_frame_t *frame);

void          x86_init(void);
const gdtr_t *get_gdtr(void);
const idtr_t *get_idtr(void);
void          gdt_init(void);
void          idt_init(void);
void          set_tss_to_kstack_top(uintptr_t stack_top);
void          idt_register_interrupt_handlers(uint8_t num, irqHandler handler);
