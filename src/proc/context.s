.intel_syntax noprefix
.code32

.section .text

.extern g_tss
.extern current_task
.global switch_to
.global task_launcher
.global task_user_launcher

# =============================================================================
# STRUCT TASK LAYOUT (Memory Map)
# =============================================================================
# Offset | Size | Field
# -------|------|--------------------------------------------------------------
# 0      | 4    | pid_t pid
# 4      | 4    | uid_t uid
# 8      | 4    | gid_t gid
# 12     | 4    | struct task *real_parent
# 16     | 4    | struct task *parent
# 20     | 8    | struct list_head children (prev=20, next=24)
# 28     | 8    | struct list_head siblings (prev=28, next=32)
# 36     | 4    | uintptr_t esp (CPU Context Save)
# 40     | 4    | uintptr_t cr3 (Page Directory Pointer)
# 44     | 4    | uintptr_t kernel_stack_pointer (Bottom / Canary)
# 48     | 4    | uintptr_t kernel_stack_base (Top / TSS.esp0)
# 52     | 4    | section_t *text_sec
# 56     | 4    | section_t *data_sec
# 60     | 4    | section_t *stack_sec
# 64     | 4    | section_t *heap_sec
# 68     | 4    | struct task *next (Scheduler)
# 72     | 4    | struct task *prev
# 76     | 4    | enum process_states state
# 80     | 12   | struct signal_queue signals
# 92     | 4    | char *name
# 96     | 4    | size_t ring
# =============================================================================

# void switch_to(struct task *current, struct task *next);
switch_to:
  push ebp
  mov ebp, esp

  push ebx
  push esi
  push edi

  mov ebx, [ebp + 8]
  mov [ebx + 36], esp

  mov ebx, [ebp + 12]
  mov esp, [ebx + 36]

  mov eax, [ebx + 40]
  mov cr3, eax

  mov eax, [ebx + 48]
  mov [g_tss + 4], eax

  pop edi
  pop esi
  pop ebx

  pop ebp

  ret

# void task_launcher(struct task *next);
task_launcher:
  # get task *next into ebx
  mov ebx, [esp + 4]
  mov current_task, ebx

  # reload cr3
  mov eax, [ebx + 40]
  mov cr3, eax

  # update g_tss
  mov eax, [ebx + 48]
  mov [g_tss + 4], eax

  # jump to new process stack
  mov esp, [ebx + 36]

  # pop flushed registers
  pop edi
  pop esi
  pop ebx
  pop ebp

  ret

task_user_launcher:
    mov ebx, [esp + 4]
    mov current_task, ebx

    mov eax, [ebx + 40]
    mov cr3, eax

    mov eax, [ebx + 48]
    mov [g_tss + 4], eax

    mov esp, [ebx + 36]

    # set segs as UserDS
    mov ax, 0x23  # UserDS = 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop edi
    pop esi
    pop ebx
    pop ebp

    iret
