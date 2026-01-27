#include <arch/acpi.h>
#include <arch/io.h>
#include <arch/trap_frame.h>
#include <arch/x86.h>
#include <drivers/keyboard.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/memory.h>
#include <types.h>

typedef struct __attribute__((packed)) {
	uint16_t offset_1;
	uint16_t selector;
	uint8_t  zero;
	uint8_t  type_attributes;
	uint16_t offset_2;
} idt_entry;

enum IDTTypeAttributes {
	TaskGate    = 0x05,
	IntGate_16  = 0x06,
	TrapGate_16 = 0x07,
	IntGate_32  = 0x0E,
	TrapGate_32 = 0x0F,

	CPU_Ring0 = 0x00 << 5,
	CPU_Ring1 = 0x01 << 5,
	CPU_Ring2 = 0x02 << 5,
	CPU_Ring3 = 0x03 << 5,

	PresentBit = 0x01 << 7
};

typedef void (*syscallHandler)(trap_frame_t *frame);

#define IDT_SIZE        256
#define IDT_ENTRY(indx) (idt_entries + (indx))

#define PIC1         0x20 // IO base address for master PIC
#define PIC2         0xA0 // IO base address for slave PIC
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2 + 1)

#define PIC_EOI 0x20 // End-of-interrupt command code

#define ICW1_ICW4      0x01 // Indicates that ICW4 will be present
#define ICW1_SINGLE    0x02 // Single (cascade) mode
#define ICW1_INTERVAL4 0x04 // Call address interval 4 (8)
#define ICW1_LEVEL     0x08 // Level triggered (edge) mode
#define ICW1_INIT      0x10 // Initialization - required!

#define ICW4_8086       0x01 // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO       0x02 // Auto (normal) EOI
#define ICW4_BUF_SLAVE  0x08 // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C // Buffered mode/master
#define ICW4_SFNM       0x10 // Special fully nested (not)

extern void isr_0(void);
extern void isr_1(void);
extern void isr_2(void);
extern void isr_3(void);
extern void isr_4(void);
extern void isr_5(void);
extern void isr_6(void);
extern void isr_7(void);
extern void isr_8(void);
extern void isr_9(void);
extern void isr_10(void);
extern void isr_11(void);
extern void isr_12(void);
extern void isr_13(void);
extern void isr_14(void);
extern void isr_15(void);
extern void isr_16(void);
extern void isr_17(void);
extern void isr_18(void);
extern void isr_19(void);
extern void isr_20(void);
extern void isr_21(void);
extern void isr_22(void);
extern void isr_23(void);
extern void isr_24(void);
extern void isr_25(void);
extern void isr_26(void);
extern void isr_27(void);
extern void isr_28(void);
extern void isr_29(void);
extern void isr_30(void);
extern void isr_31(void);

extern void irq_32(void);
extern void irq_33(void);
extern void irq_34(void);
extern void irq_35(void);
extern void irq_36(void);
extern void irq_37(void);
extern void irq_38(void);
extern void irq_39(void);
extern void irq_40(void);
extern void irq_41(void);
extern void irq_42(void);
extern void irq_43(void);
extern void irq_44(void);
extern void irq_45(void);
extern void irq_46(void);
extern void irq_47(void);
// [...]
extern void irq_128(void);

extern void syscall_dispatcher(trap_frame_t *frame);

idt_entry idt_entries[IDT_SIZE];
idtr_t    idtr = {.limit = sizeof(idt_entry) * IDT_SIZE - 1, .base = (uintptr_t)&idt_entries};

irqHandler interrupt_handlers[256] = {
    [0x80] = syscall_dispatcher,
};

syscallHandler syscall_handlers[256] = {};

const char *interrupt_names[] = {"Divide Error",
                                 "Debug Exception",
                                 "NMI Interrupt",
                                 "Breakpoint",
                                 "Overflow",
                                 "BOUND Range Exceeded",
                                 "Invalid Opcode (Undefined Opcode)",
                                 "Device Not Available (Math Coprocessor)",
                                 "Double Fault",
                                 "Coprocessor Segment Overrun (reserved)",
                                 "Invalid TSS",
                                 "Segment Not Present",
                                 "Stack-Segment Fault",
                                 "General Protection",
                                 "Page Fault",
                                 "Reserved",
                                 "x87 FPU Floating-Point Error (Math Fault)",
                                 "Alignment Check",
                                 "Machine Check",
                                 "SIMD Floating-Point Exception",
                                 "Virtualization Exception",
                                 "Control Protection Exception",
                                 "Reserved",
                                 "Reserved",
                                 "Reserved",
                                 "Reserved",
                                 "Reserved",
                                 "Reserved",
                                 "Hypervisor Injection Exception",
                                 "VMM Communication Exception",
                                 "Security Exception",
                                 "Reserved"};

void exception_handler(trap_frame_t *frame)
{
	if (interrupt_handlers[frame->int_no] != NULL)
		interrupt_handlers[frame->int_no](frame);
	else if (frame->int_no < sizeof(interrupt_names) / sizeof(interrupt_names[0]))
		kpanic(interrupt_names[frame->int_no]);
	else
		kpanic("Unknown Exception");
}

inline void idt_register_interrupt_handlers(uint8_t num, irqHandler handler)
{
	interrupt_handlers[num] = handler;
}

static void init_pic(void)
{
	/* Initialization of ICW1 */
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

	/* Initialization of ICW2 */
	outb(PIC1_DATA, 0x20);
	outb(PIC2_DATA, 0x28);

	/* Initialization of ICW3 */
	outb(PIC1_DATA, 0x04);
	outb(PIC2_DATA, 0x02);

	/* Initialization of ICW4 */
	outb(PIC1_DATA, ICW4_8086);
	outb(PIC2_DATA, ICW4_8086);

	/* mask interrupts */
	outb(PIC1_DATA, 0x00);
	outb(PIC2_DATA, 0x00);
}

static inline void idt_set_entry(idt_entry *ptr, uint16_t selector, uint8_t type, uint32_t offset)
{
	ptr->offset_1        = offset & 0xffff;
	ptr->offset_2        = (offset & 0xffff0000) >> 16;
	ptr->selector        = selector;
	ptr->type_attributes = type;
}

const idtr_t *get_idtr(void) { return &idtr; }

void idt_init(void)
{
	init_pic();
	ft_bzero((void *)&idt_entries, sizeof(idt_entry) * IDT_SIZE);

	idt_set_entry(IDT_ENTRY(0x00), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_0);
	idt_set_entry(IDT_ENTRY(0x01), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_1);
	idt_set_entry(IDT_ENTRY(0x02), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_2);
	idt_set_entry(IDT_ENTRY(0x03), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_3);
	idt_set_entry(IDT_ENTRY(0x04), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_4);
	idt_set_entry(IDT_ENTRY(0x05), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_5);
	idt_set_entry(IDT_ENTRY(0x06), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_6);
	idt_set_entry(IDT_ENTRY(0x07), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_7);
	idt_set_entry(IDT_ENTRY(0x08), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_8);
	idt_set_entry(IDT_ENTRY(0x09), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_9);
	idt_set_entry(IDT_ENTRY(0x0a), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_10);
	idt_set_entry(IDT_ENTRY(0x0b), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_11);
	idt_set_entry(IDT_ENTRY(0x0c), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_12);
	idt_set_entry(IDT_ENTRY(0x0d), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_13);
	idt_set_entry(IDT_ENTRY(0x0e), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_14);
	idt_set_entry(IDT_ENTRY(0x0f), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_15);
	idt_set_entry(IDT_ENTRY(0x10), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_16);
	idt_set_entry(IDT_ENTRY(0x11), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_17);
	idt_set_entry(IDT_ENTRY(0x12), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_18);
	idt_set_entry(IDT_ENTRY(0x13), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_19);
	idt_set_entry(IDT_ENTRY(0x14), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_20);
	idt_set_entry(IDT_ENTRY(0x15), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_21);
	idt_set_entry(IDT_ENTRY(0x16), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_22);
	idt_set_entry(IDT_ENTRY(0x17), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_23);
	idt_set_entry(IDT_ENTRY(0x18), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_24);
	idt_set_entry(IDT_ENTRY(0x19), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_25);
	idt_set_entry(IDT_ENTRY(0x1a), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_26);
	idt_set_entry(IDT_ENTRY(0x1b), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_27);
	idt_set_entry(IDT_ENTRY(0x1c), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_28);
	idt_set_entry(IDT_ENTRY(0x1d), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_29);
	idt_set_entry(IDT_ENTRY(0x1e), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_30);
	idt_set_entry(IDT_ENTRY(0x1f), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)isr_31);

	idt_set_entry(IDT_ENTRY(0x20), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_32);
	idt_set_entry(IDT_ENTRY(0x21), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_33);
	idt_set_entry(IDT_ENTRY(0x22), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_34);
	idt_set_entry(IDT_ENTRY(0x23), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_35);
	idt_set_entry(IDT_ENTRY(0x24), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_36);
	idt_set_entry(IDT_ENTRY(0x25), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_37);
	idt_set_entry(IDT_ENTRY(0x26), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_38);
	idt_set_entry(IDT_ENTRY(0x27), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_39);
	idt_set_entry(IDT_ENTRY(0x28), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_40);
	idt_set_entry(IDT_ENTRY(0x29), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_41);
	idt_set_entry(IDT_ENTRY(0x2a), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_42);
	idt_set_entry(IDT_ENTRY(0x2b), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_43);
	idt_set_entry(IDT_ENTRY(0x2c), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_44);
	idt_set_entry(IDT_ENTRY(0x2d), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_45);
	idt_set_entry(IDT_ENTRY(0x2e), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_46);
	idt_set_entry(IDT_ENTRY(0x2f), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_47);
	// [...]
	idt_set_entry(IDT_ENTRY(0x80), 0x08, PresentBit | IntGate_32 | CPU_Ring0, (uint32_t)irq_128);

	__asm__ volatile("lidt %0; sti" : : "m"(idtr));
}
