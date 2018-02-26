ENTRY(arch_startup)
SECTIONS {
 .code 0x100000 :
 {
  kernel_code_addr = .;
  *?/boot/startup.asm.o ( .text* )
  *( .text* )
 }
 .data :
 {
  kernel_data_addr = .;
  * ( .rodata* .data* )
 }
 .bss :
 {
  *( .bss* COMMON* )
  . = ALIGN (4096);
 }
 kernel_end_addr = .;
}
