.intel_syntax noprefix
.code32

.set MAGIC,         0xE85250D6
.set ARCHITECTURE,  0
.set HEADER_LENGTH, header_end - header_start
.set CHECKSUM,      -(MAGIC + ARCHITECTURE + HEADER_LENGTH)

#--------------------------
# --- Multiboot Section ---
.section .multiboot

.align 8
header_start:
.long MAGIC
.long ARCHITECTURE
.long HEADER_LENGTH
.long CHECKSUM
.short 7, 0
.long  8
.short 0, 0
.long  8
header_end:

#--------------------
# --- BSS Section ---
.section .bss, "aw", @nobits
.align 4096

bootstrap_page_directory:
	.skip 4096

bootstrap_page_table_identity:
	.skip 4096                      # PDE[0] first 4MiB

bootstrap_page_table_higher:
	.skip 4096                      # PDE[768] Higher half first 4MiB

.global mb2_magic
mb2_magic:
    .skip 4

.global mb2_mbi
mb2_mbi:
    .skip 4

#---------------------
# --- Text Section ---
.section .text

.global _start
.extern kernel_init_higher_half
.extern kernel_main
.extern kernel_stack_top


.macro clear_table addr
    push edi
    push ecx
    push eax

    mov edi, \addr
    mov ecx, 1024
    xor eax, eax
    rep stosd

    pop eax
    pop ecx
    pop edi
.endm

_start:
	lea esp, [kernel_stack_top - 0xC0000000]
	mov ebp, esp

    mov [mb2_magic - 0xC0000000], eax
    mov [mb2_mbi - 0xC0000000], ebx

	call boostrap_paging

boostrap_paging:

    #--------------------------------------------------------
    # Setup bootstrap paging tables

    # Prepare paging vars into registers

    mov edi, offset bootstrap_page_directory
    sub edi, 0xC0000000
    mov esi, offset bootstrap_page_table_identity
    sub esi, 0xC0000000
    mov edx, offset bootstrap_page_table_higher
    sub edx, 0xC0000000

    clear_table edi
    clear_table esi
    clear_table edx


    # Set PD[0] flags

    mov eax, esi
    or eax, 3
    mov [edi], eax


    # Setup first page table for identity

    mov edi, esi
    mov ecx, 1024
    mov eax, 0 | 3
fill_identity_loop:
    stosd
    add eax, 4096
    loop fill_identity_loop


    # Set PD[768] flags

    mov edi, offset bootstrap_page_directory
    sub edi, 0xC0000000
    mov eax, edx
    or eax, 3
    mov [edi + 768 * 4], eax


    # Setup first page table for higher half

    mov edi, edx
    mov ecx, 1024
    mov eax, 0 | 3
fill_kernel_loop:
    stosd
    add eax, 4096
    loop fill_kernel_loop


    #--------------------------------------------------------
    # reload cr3 and enable paging

    mov edi, offset bootstrap_page_directory
    sub edi, 0xC0000000
    mov eax, edi
    mov cr3, eax
    mov eax, cr0
    or  eax, 0x80000001
    mov cr0, eax

    lea eax, [reload_eip]
    jmp eax

    #--------------------------------------------------------
    # reload stack and eip in higher half

reload_eip:

    lea esp, [kernel_stack_top]
    mov ebp, esp

    #--------------------------------------------------------


    call kernel_main

    cli
    jmp hang

hang:
    hlt
    jmp hang
