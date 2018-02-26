OUTPUT_FORMAT("binary")
ENTRY(prog_init)
SECTIONS {
 .user 0:
 {
  user_code = .;
  * ( *.program_header* )
  * (.text*)
  user_data = .;
  * ( .rodata* .data* )
  user_bss = .;
  * ( .bss* COMMON* )
  . = ALIGN (4096);
  user_end = .;
 }
 /DISCARD/ : { *(.comment*) }
 /DISCARD/ : { *(.eh_frame*) }
}
