.intel_syntax noprefix
.code32

.section .text
	.global syscall_dispatcher
	.global interrupt_dispatcher
	.extern syscall_handlers
	.extern interrupt_handlers

syscall_dispatcher:
	mov eax, [esp+4]
	mov eax, [eax+48]
	shl eax, 2
	mov eax, [syscall_handlers+eax]
	test eax, eax
	jz .no_sys_handler
	jmp eax
.no_sys_handler:
	ret

interrupt_dispatcher:
	mov eax, [esp+4]
	mov eax, [eax+48]
	shl eax, 2
	mov eax, [interrupt_handlers+eax]
	test eax, eax
	jz .no_int_handler
	jmp eax
.no_int_handler:
	ret

interrupt_routine:
	pusha

	mov ax, ds
	push eax
	mov ax, es
	push eax
	mov ax, fs
	push eax
	mov ax, gs
	push eax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov eax, [esp+48]
	cmp eax, 40
	jl .skip_pic2
	mov al, 0x20
	out 0xA0, al
.skip_pic2:
	mov al, 0x20
	out 0x20, al

	lea eax, [esp]
	push eax
	call interrupt_dispatcher
	add esp, 4

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
irq_stub 128
