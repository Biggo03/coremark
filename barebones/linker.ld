ENTRY(_start)

MEMORY {
  IMEM (rx)  : ORIGIN = 0x00000000, LENGTH = 32K
  DMEM (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
}

__stack_top = ORIGIN(DMEM) + LENGTH(DMEM);

SECTIONS {
    .text : ALIGN(4) {
        KEEP(*(.text._start))
        *(.init)
        *(.text .text.*)
    } > IMEM

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
    } > DMEM

    .data : ALIGN(4) {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } > DMEM

    .sdata : ALIGN(4) {
        __sdata_start = .;
        *(.sdata .sdata.*)
        __sdata_end = .;
    } > DMEM

    .sbss (NOLOAD) : ALIGN(4) {
        __sbss_start = .;
        *(.sbss .sbss.*)
        __sbss_end = .;
    } > DMEM

    __global_pointer$ = (__sdata_start + __sbss_end) / 2;

    .bss (NOLOAD) : ALIGN(4) {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } > DMEM

    . = ALIGN(8);
    __heap_start = .;
    __heap_end   = __stack_top - 0x100;
}
