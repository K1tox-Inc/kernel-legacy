.section .text

.global user_cafe_start
.global user_cafe_end
.global user_dead_start
.global user_dead_end
.global kernel_task_start
.global kernel_task_end

.extern hello_kproc


# --- USER : CAFEBABE ---
.align 4
user_cafe_start:
    mov $0, %eax
    mov $0x41, %ebx
    int $0x80
    
    mov $0xCAFEBABE, %eax
1:
    jmp 1b
user_cafe_end:


# --- USER : DEADBEEF ---
.align 4
user_dead_start:
    mov $0xDEADBEEF, %ebx
2:
    jmp 2b
user_dead_end:


# --- KERNEL : HELLO ---
.align 4
kernel_task_start:
    call hello_kproc
1:
    hlt
    jmp 1b
kernel_task_end: