ENTRY(_start)

SECTIONS {
    . = 1M;

    .multiboot2 ALIGN(8) : {
        *(.multiboot2)
    }

	. += 0xC0000000;

    PROVIDE(kernel_start = .);

    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000) {
        *(.rodata*)
    }

    .ctors ALIGN(4K) : AT(ADDR(.ctors) - 0xC0000000) {
        PROVIDE(__ctors_start = .);
        KEEP(*(.ctors))
        KEEP(*(SORT_BY_NAME(.ctors.*)))
        PROVIDE(__ctors_end = .);
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
