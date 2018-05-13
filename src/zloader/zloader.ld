
MEMORY
{
    /* start at 32MB from start of memory, size = 128MB */
    //ram : org = 0x8000, len = 0x8000000
    ram : org = 0x2000000, len = 0x8000000  
}

__reloc_base__ = 0x8000;
__reloc_target__ = ORIGIN(ram);

__stack_top__ = ORIGIN(ram)+LENGTH(ram);

SECTIONS
{
    . = 0;

    .text : ALIGN(16) SUBALIGN(16)
    {
        _text = .;
        KEEP(*(vectors))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } > ram


    . = ALIGN(4);

    .data :
    {
        _data = .;
        *(.data)
        . = ALIGN(4);
        *(.data.*)
        . = ALIGN(4);
    } > ram 
   
}

_end = .;
__reloc_end__ = __reloc_base__ + _end - ORIGIN(ram);

