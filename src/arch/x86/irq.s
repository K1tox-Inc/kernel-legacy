.intel_syntax noprefix
.code32

.section .text
	.global syscall_dispatcher
	.global interrupt_dispatcher
	.extern syscall_handlers
	.extern interrupt_handlers

syscall_dispatcher:
	# Pass pointer to trap_frame as argument
	lea eax, [esp]
	push eax
	
	# Get int_no from trap_frame (offset: 4 segs + 8 GPRs = 48 bytes)
	mov eax, [esp+52]  # +4 because we pushed the pointer
	shl eax, 2
	mov eax, [syscall_handlers+eax]

	test eax, eax
	jz .no_sys_handler

	call eax
	add esp, 4  # Clean up pushed pointer

.no_sys_handler:
	ret

interrupt_dispatcher:
	# Pass pointer to trap_frame as argument
	lea eax, [esp]
	push eax
	
	# Get int_no from trap_frame (offset: 4 segs + 8 GPRs = 48 bytes)
	mov eax, [esp+52]  # +4 because we pushed the pointer
	shl eax, 2
	mov eax, [interrupt_handlers+eax]

	test eax, eax
	jz .no_int_handler

	call eax
	add esp, 4  # Clean up pushed pointer

.no_int_handler:
	ret

interrupt_routine:
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

	# Send EOI to PIC
	# Adjust offset: 4 segments * 4 bytes + 8 GPRs * 4 bytes = 48 bytes
	mov eax, [esp+48]
	cmp eax, 40
	jl .skip_pic2

	mov al, 0x20
	out 0xA0, al

.skip_pic2:
	mov al, 0x20
	out 0x20, al

	# Pass pointer to trap_frame as argument
	lea eax, [esp]
	push eax
	
	call interrupt_dispatcher
	
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
	add esp, 4
	iret

.macro irq_stub num
.global irq_\num
irq_\num:
	cli
	push \num
	jmp interrupt_routine
.endm

irq_stub 32
irq_stub 33
irq_stub 34
irq_stub 35
irq_stub 36
irq_stub 37
irq_stub 38
irq_stub 39
irq_stub 40
irq_stub 41
irq_stub 42
irq_stub 43
irq_stub 44
irq_stub 45
irq_stub 46
irq_stub 47
# [...]
irq_stub 128
