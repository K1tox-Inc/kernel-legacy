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

.global user_cafe_start
.global user_cafe_end
.global user_dead_start
.global user_dead_end
.global kitoxD_start
.global kitoxD_end

.align 4
kitoxD_start:
    xor esi, esi

.kitox_spawn_loop:

    cmp esi, 2
    jge .kitox_hang

    SYSCALL_FORK
    test eax, eax
    jz .kitox_child

    mov ecx, 0xbeef

    inc esi
    jmp .kitox_spawn_loop

.kitox_child:

    mov ecx, 0xdead

    SYSCALL_EXEC_FN esi
    SYSCALL_EXIT 1

.kitox_hang:

    SYSCALL_WAITPID -1, 0, 0
    jmp .kitox_hang

kitoxD_end:

.align 4
user_cafe_start:

    call .write

.exit:
    SYSCALL_EXIT eax

.msg:
.ascii "Hello from forked!\n"

.write:
    mov ecx, [esp]
    add ecx, (.msg - .exit)
    SYSCALL_WRITE 1, ecx, (.write - .msg)
    ret

user_cafe_end:

.align 4
user_dead_start:
    jmp .dead_after_msg
.dead_msg:
    .ascii "Hello from dead!\n"
.dead_after_msg:
    call .dead_getpc
.dead_getpc:
    pop ecx
    sub ecx, (.dead_getpc - .dead_msg)
    SYSCALL_WRITE 1, ecx, 17

    SYSCALL_MMAP 0, 4096, 3, 0x20
    mov esi, eax

    mov dword ptr [esi], 0x44414544
    mov byte ptr [esi + 4], 0x0A

    SYSCALL_WRITE 1, esi, 5

    mov eax, 0xDEADBEEF
    SYSCALL_EXIT 1

user_dead_end:
