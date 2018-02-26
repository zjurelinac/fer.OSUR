#define _K_PRINT_C_

#include <kernel/kprint.h>

#include <lib/string.h>
#include <types/io.h>

console_t *k_stdout;


int kprintf ( char *format, ... )
{
	size_t size;
	char buffer[CONSOLE_MAXLEN];

	k_stdout->print ( "\x1b[31m" );

	size = vssprintf ( buffer, CONSOLE_MAXLEN, &format );
	k_stdout->print ( buffer );

	k_stdout->print ( "\x1b[39m" );

	return size;
}
