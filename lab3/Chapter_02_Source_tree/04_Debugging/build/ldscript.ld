ENTRY ( arch_startup )
SECTIONS {
    .kernel_code 0x0100000 :
    {
        *?/boot/startup.asm.o ( .text* )
        *kernel/?*( .text* )
        *arch/?*( .text* )
        *api/?*( .text* )
        *lib/?*( .text* )
        *kernel/?*( .rodata* )
        *arch/?*( .rodata* )
        *api/?*( .rodata* )
        *lib/?*( .rodata* )
    }
    .kernel_data . :
    {
        kernel_data_addr = .;
        *kernel/?*( .data* )
        *arch/?*( .data* )
        *api/?*( .data* )
        *lib/?*( .data* )
        *kernel/?*( .bss* COMMON* )
        *arch/?*( .bss* COMMON* )
        *api/?*( .bss* COMMON* )
        *lib/?*( .bss* COMMON* )
        . = ALIGN (4096);
        kernel_end_addr = .;
    }
    program_code_start = 0x0200000;
    program_code_store_start = kernel_end_addr;
    .program_code program_code_start : AT ( kernel_end_addr )
    {
        *programs/?*( .text* )
        *programs/?*( .rodata* )
        . = ALIGN (4096);
    }
    program_code_store_end = program_code_store_start + SIZEOF( .program_code );
    program_data_store_start = program_code_store_end;
    program_data_start = 0x0200000 + SIZEOF ( .program_code );
    .program_data ( program_data_start ) : AT ( program_code_store_end )
    {
        *programs/?*( .data* )
        *programs/?*( .bss* )
        . = ALIGN (4096);
    }
    program_data_store_end = program_data_store_start + SIZEOF( .program_data );
    /DISCARD/ : { *(.debug*) }
    /DISCARD/ : { *(.note*) }
    /DISCARD/ : { *(.comment*) }
    /DISCARD/ : { *(.eh_frame*) }
}
