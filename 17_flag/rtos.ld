MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

SECTIONS
{
    .text :
    {
        _text = .;
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
    }

    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        _data = .;
        *(vtable)
        *(.data*)
        _edata = .;
    } > SRAM

    .bss :
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM

    . = ALIGN(32);           /*Not sure if this needs to be done, but why not.*/
    _p_stack_bottom = .;
    . = . + 0x4000;
    _p_stack_top = 0x20008000;
    . = . + 0x4000;          /*Allocate 4K for the Stack.*/
    _stack_top = 0x2000c000;  /*Address of the top of the heap, also end of RAM.*/
}
