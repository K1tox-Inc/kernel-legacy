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
    jmp 1b

user_cafe_end:


# --- USER : DEADBEEF ---
.align 4
user_dead_start:

    mov ebx, 0xDEADBEEF

2:
    jmp 2b

user_dead_end:

# --- USER : SYS_GET ---
.align 4
mok_sys_get_start:
    mov eax, 39
    int 0x80
    mov edi, eax 
1:
    jmp 1b
mok_sys_get_end: