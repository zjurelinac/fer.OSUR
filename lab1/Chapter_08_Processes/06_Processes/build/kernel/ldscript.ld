ENTRY(arch_startup)
SECTIONS {
 .kernel 0x100000 :
 {
  kernel_code_addr = .;
  *?/boot/startup.asm.o ( .text* )
  *( .text* )
  kernel_data_addr = .;
  *( .rodata* .data* )
 }
 .kernel_bss :
 {
  *( .bss* COMMON* )
  . = ALIGN (4096);
 }
 kernel_end_addr = .;
 /DISCARD/ : { *(.comment*) }
 /DISCARD/ : { *(.eh_frame*) }
}
