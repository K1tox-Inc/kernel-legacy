ENTRY(_start)

SECTIONS {
    . = 1M;

    .multiboot2_header ALIGN(8) : {
        *(.multiboot2_header)
    }

	. += 0xC0000000;
    PROVIDE(kernel_start = .);

    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000) {
        *(.rodata*)
    }

    .init_array ALIGN(4K) : AT(ADDR(.init_array) - 0xC0000000) {
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT_BY_NAME(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);
    }
    
    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000) {
        *(.data*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000) {
        *(COMMON)
        *(.bss)
    }
    .stack ALIGN(16) : AT(ADDR(.stack) - 0xC0000000) {
        kernel_stack = .;
        *(.stack)
        kernel_stack_top = .;
    }
    PROVIDE(kernel_end = .);
}
