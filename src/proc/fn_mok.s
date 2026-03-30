.intel_syntax noprefix
.code32

.section .data

hello_from_cafe:
    .ascii "Hello from cafe!\n"

.section .text

.global user_cafe_start
.global user_cafe_end
.global user_dead_start
.global user_dead_end
.global mok_sys_get_start
.global mok_sys_get_end
.global kitoxD_start
.global kitoxD_end

# --- USER : CAFEBABE ---
.align 4
user_cafe_start:

    # 1. Call sys_fork (syscall index 2)
    mov eax, 2
    int 0x80

    # 2. Call sys_write (syscall index 4)
    mov eax, 4          # syscall index: sys_write
    mov ebx, 1          # argument 1: file descriptor 1 (stdout)
    lea ecx, [hello_from_cafe]
    mov edx, 17         # argument 3: string length (17 bytes)
    int 0x80            # Trigger sys_write

    mov eax, 0xCAFEBABE

1:
    hlt
    jmp 1b

user_cafe_end:


# --- USER : DEADBEEF ---
.align 4
user_dead_start:

    mov ebx, 0xDEADBEEF

2:
    hlt
    jmp 2b

user_dead_end:

# --- USER : SYS_GET ---
.align 4
mok_sys_get_start:
    mov eax, 20
    int 0x80
    mov edi, eax 
3:
    hlt
    jmp 3b
mok_sys_get_end:

# ============================================================
# SYSCALL WAIT MACROS
# ============================================================
# SYSCALL_WAIT4(pid, status, options, rusage)  — primitive
# SYSCALL_WAITPID(pid, status, options)        — wrapper wait4
# SYSCALL_WAIT(status)                         — wrapper wait4
# ============================================================

.macro SYSCALL_WAIT4 pid, status, options, rusage
    mov eax, 114            # sys_wait4 — Linux x86_32 ABI
    mov ebx, \pid
    mov ecx, \status
    mov edx, \options
    mov esi, \rusage
    int 0x80
.endm

.macro SYSCALL_WAITPID pid, status, options
    SYSCALL_WAIT4 \pid, \status, \options, 0
.endm

.macro SYSCALL_WAIT status
    SYSCALL_WAIT4 -1, \status, 0, 0
.endm

# --- USER : K1toxD ---
.align 4
kitoxD_start:
    # SYSCALL_WAIT 0
2:
    hlt
    jmp 2b
kitoxD_end: