.intel_syntax noprefix
.code32

.section .text

.extern exception_handler

.macro isr_err_stub num
.global isr_\num
isr_\num:
	cli
	push \num
	jmp exception_routine
.endm

.macro isr_no_err_stub num
.global isr_\num
isr_\num:
	cli
	push 0
	push \num
	jmp exception_routine
.endm

exception_routine:
	pusha

	# Push all segment registers (order: ds, es, fs, gs)
	mov ax, ds
	push eax
	mov ax, es
	push eax
	mov ax, fs
	push eax
	mov ax, gs
	push eax

	# Load kernel data segment
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	# Pass pointer to trap_frame as argument
	lea eax, [esp]
	push eax
	
	call exception_handler
	
	add esp, 4  # Clean up pushed pointer

	# Restore segment registers (reverse order: gs, fs, es, ds)
	pop eax
	mov gs, ax
	pop eax
	mov fs, ax
	pop eax
	mov es, ax
	pop eax
	mov ds, ax

	popa

	add esp, 8

	iret

isr_no_err_stub  0  # Division by Zero
isr_no_err_stub  1  # Debug Exception
isr_no_err_stub  2  # Non-Maskable Interrupt
isr_no_err_stub  3  # Breakpoint
isr_no_err_stub  4  # Overflow
isr_no_err_stub  5  # Bounds Check
isr_no_err_stub  6  # Invalid Opcode
isr_no_err_stub  7  # Device Not Available
isr_err_stub     8  # Double Fault
isr_no_err_stub  9  # Coprocessor Segment Overrun
isr_err_stub    10  # Invalid TSS
isr_err_stub    11  # Segment Not Present
isr_err_stub    12  # Stack Fault
isr_err_stub    13  # General Protection Fault
isr_err_stub    14  # Page Fault
isr_no_err_stub 15  # Reserved
isr_no_err_stub 16  # x87 Floating-Point
isr_err_stub    17  # Alignment Check
isr_no_err_stub 18  # Machine Check
isr_no_err_stub 19  # SIMD Floating-Point
isr_no_err_stub 20  # Virtualization
isr_no_err_stub 21  # Control Protection
isr_no_err_stub 22  # Reserved
isr_no_err_stub 23  # Reserved
isr_no_err_stub 24  # Reserved
isr_no_err_stub 25  # Reserved
isr_no_err_stub 26  # Reserved
isr_no_err_stub 27  # Reserved
isr_no_err_stub 28  # Reserved
isr_no_err_stub 29  # Reserved
isr_err_stub    30  # Security Exception
isr_no_err_stub 31  # Reserved
