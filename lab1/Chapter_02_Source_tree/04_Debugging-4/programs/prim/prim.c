/*! Prim numbers example */

#include <stdio.h>
#include <api/prog_info.h>

static int test_prim ( unsigned long n ) {
	unsigned long i, max;

	if ( ( n & 1 ) == 0 ) /* je li paran? */
		return 0;

	max = n/2;
	for ( i = 3; i <= max; i += 2 )
		if ( ( n % i ) == 0 )
			return 0;

	return 1; /* prim number */
}

int prim ()
{
	unsigned long i;

	printf ( "Example program: [%s:%s]\n%s\n\n", __FILE__, __FUNCTION__,
		 prim_PROG_HELP );

	printf ( "Printing a few prim numbers:\n" );

	for ( i = 0 ; i < 50; i++ )
		if ( test_prim (i) )
			printf ( "%d ", i );

	printf ( "\n" );

	return 0;
}
