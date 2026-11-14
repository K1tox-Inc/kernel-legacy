.intel_syntax noprefix
.code32

.macro SYSCALL_MMAP addr, length, prot, flags
    mov eax, 90
    mov ebx, \addr
    mov ecx, \length
    mov edx, \prot
    mov esi, \flags
    mov edi, -1
    xor ebp, ebp
    int 0x80
.endm

.macro SYSCALL_MUNMAP addr, length
    mov eax, 91
    mov ebx, \addr
    mov ecx, \length
    int 0x80
.endm

.macro SYSCALL_WRITE fd, buf, len
    mov eax, 4
    mov ebx, \fd
    mov ecx, \buf
    mov edx, \len
    int 0x80
.endm

.macro SYSCALL_WAITPID pid, status, options
    mov eax, 7
    mov ebx, \pid
    mov ecx, \status
    mov edx, \options
    int 0x80
.endm

.macro SYSCALL_EXIT status
    mov eax, 1
    mov ebx, \status
    int 0x80
.endm

.macro SYSCALL_SLEEP seconds
    mov eax, 162
    mov ebx, \seconds
    int 0x80
.endm

.macro SYSCALL_FORK
    mov eax, 2
    int 0x80
.endm

.macro SYSCALL_EXEC_FN index
    mov eax, 11
    mov ebx, \index
    int 0x80
.endm

.macro SYSCALL_SIGNAL sig, handler
    mov eax, 48
    mov ebx, \sig
    mov ecx, \handler
    int 0x80
.endm

.section .text

.global kitoxD_start
.global kitoxD_end

.align 4
kitoxD_start:

    .kitox_hang:
        SYSCALL_WAITPID -1, 0, 0
        jmp .kitox_hang

kitoxD_end:
