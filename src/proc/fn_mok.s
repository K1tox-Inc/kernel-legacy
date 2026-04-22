.intel_syntax noprefix
.code32

# ============================================================
# SYSCALL MACROS
# ============================================================

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

# ============================================================
# GLOBALS
# ============================================================

.section .text

.global user_cafe_start
.global user_cafe_end
.global user_dead_start
.global user_dead_end
.global kitoxD_start
.global kitoxD_end

# ============================================================
# INIT PROCESS — kitoxD (PID 1)
# ============================================================

.align 4
kitoxD_start:
    xor esi, esi                # esi = index = 0

.kitox_spawn_loop:
    cmp esi, 2                  # MOK_SENTINEL
    jge .kitox_reap

    SYSCALL_FORK
    test eax, eax
    jnz .kitox_parent

    # Child: exec mok[esi]
    mov eax, 11
    mov ebx, esi
    int 0x80
    # unreachable

.kitox_parent:
    inc esi
    jmp .kitox_spawn_loop

.kitox_reap:
    jmp .kitox_after_msg
.kitox_msg:
    .ascii "kitoxD: going to sleep\n"
.kitox_after_msg:
    call .kitox_getpc
.kitox_getpc:
    pop ecx
    sub ecx, (.kitox_getpc - .kitox_msg)
    SYSCALL_WRITE 1, ecx, 23

1:
    SYSCALL_WAITPID -1, 0, 0
    jmp 1b
kitoxD_end:

# ============================================================
# MOK — CAFEBABE
# ============================================================

.align 4
user_cafe_start:
    call .cafe_get_handler_addr
.cafe_get_handler_addr:
    pop ecx
    add ecx, (.cafe_handler - .cafe_get_handler_addr)
    SYSCALL_SIGNAL 2, ecx

    jmp .cafe_after_msg
.cafe_msg:
    .ascii "Cafe: waiting for signal...\n"
.cafe_after_msg:
    call .cafe_getpc
.cafe_getpc:
    pop ecx
    sub ecx, (.cafe_getpc - .cafe_msg)
    SYSCALL_WRITE 1, ecx, 28

.cafe_loop:
    SYSCALL_SLEEP 2
    jmp .cafe_loop

.align 4
.cafe_handler:
    jmp .cafe_h_after_msg
.cafe_h_msg:
    .ascii "[SIGINT] caught! returning...\n"
.cafe_h_after_msg:
    call .cafe_h_getpc
.cafe_h_getpc:
    pop ecx
    sub ecx, (.cafe_h_getpc - .cafe_h_msg)
    SYSCALL_WRITE 1, ecx, 30
    ret
user_cafe_end:

# ============================================================
# MOK — DEADBEEF
# ============================================================

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

3:
    SYSCALL_SLEEP 2
    jmp 3b
user_dead_end: