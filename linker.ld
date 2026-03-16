ENTRY(_start)

PHDRS {
    text    PT_LOAD    FLAGS(0x05);
    rodata  PT_LOAD    FLAGS(0x04);
    data    PT_LOAD    FLAGS(0x06);
}

SECTIONS {
    . = 0x00100000;
    .text : {
        *(.multiboot2_header)
        *(.text*)
    } :text
    .rodata : { *(.rodata*) } :rodata
    .data : { *(.data*) } :data
    .bss : { *(COMMON) *(.bss*) } :data
}