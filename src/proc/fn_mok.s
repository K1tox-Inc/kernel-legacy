.intel_syntax noprefix
.code32

# ============================================================
# SYSCALL MACROS
# ============================================================

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
    jmp .cafe_after_msg
.cafe_msg:
    .ascii "Hello from cafe!\n"
.cafe_after_msg:
    call .cafe_getpc
.cafe_getpc:
    pop ecx
    sub ecx, (.cafe_getpc - .cafe_msg)
    SYSCALL_WRITE 1, ecx, 17
    SYSCALL_EXIT 1

2:
    SYSCALL_SLEEP 2
    jmp 2b
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

    mov eax, 0xDEADBEEF
    SYSCALL_EXIT 1
3:
    jmp 2b
user_dead_end: