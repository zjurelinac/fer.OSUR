#define _K_STARTUP_C_

#include <arch/processor.h>
#include <api/stdio.h>
#include <api/prog_info.h>
#include <types/io.h>
#include <kernel/errno.h>


uint8 system_stack [ STACK_SIZE ];

char system_info[] = OS_NAME ": " NAME_MAJOR ":" NAME_MINOR ", "
	  "Version: " VERSION " (" ARCH ")";




void k_startup ()
{
	extern console_t K_INITIAL_STDOUT, K_STDOUT;
	extern console_t *k_stdout;


	k_stdout = &K_INITIAL_STDOUT;
	k_stdout->init (0);




	k_stdout = &K_STDOUT;
	k_stdout->init (0);

	kprintf ( "%s\n", system_info );

	stdio_init ();


	hello_world ();
	debug ();

	kprintf ( "\nSystem halted!\n" );
	halt ();
}
