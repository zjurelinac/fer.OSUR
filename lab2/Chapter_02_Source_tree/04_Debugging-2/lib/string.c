#include <lib/string.h>
void *memset ( void *s, int c, size_t n )
{
	size_t p;
	char *m = (char *) s;

	for ( p = 0; p < n; p++, m++ )
	 *m = (char) c;

	return s;
}
void *memsetw (void *s, int c, size_t n)
{
	size_t p;
	short *m = (short *) s;

	for ( p = 0; p < n; p++, m++ )
	 *m = (short) c;

	return s;
}
void *memcpy ( void *dest, const void *src, size_t n )
{
	char *d = (char *) dest, *s = (char *) src;
	size_t p;

	for ( p = 0; p < n; p++, d++, s++ )
	 *d = *s;

	return dest;
}
void *memmove ( void *dest, const void *src, size_t n )
{
	char *d, *s;
	size_t p;

	if ( dest < src )
	{
	 d = (char *) dest;
	 s = (char *) src;
	 for ( p = 0; p < n; p++, d++, s++ )
	  *d = *s;
	}
	else {
	 d = ((char *) dest) + n - 1;
	 s = ((char *) src) + n - 1;
	 for ( p = 0; p < n; p++, d--, s-- )
	  *d = *s;
	}

	return dest;
}
void *memmovew ( void *dest, const void *src, size_t n )
{
	short int *d, *s;
	size_t p;

	if ( dest < src )
	{
	 d = (short int *) dest;
	 s = (short int *) src;
	 for ( p = 0; p < n; p++, d++, s++ )
	  *d = *s;
	}
	else {
	 d = ((short int *) dest) + n - 1;
	 s = ((short int *) src) + n - 1;
	 for ( p = 0; p < n; p++, d--, s-- )
	  *d = *s;
	}

	return dest;
}
int memcmp ( const void *m1, const void *m2, size_t size )
{
	unsigned char *a = (unsigned char *) m1;
	unsigned char *b = (unsigned char *) m2;

	for ( ; size > 0; a++, b++, size-- )
	{
	 if ( *a < *b )
	  return -1;
	 else if ( *a > *b )
	  return 1;
	}

	return 0;
}


size_t strlen ( const char *s )
{
	size_t i;

	for ( i = 0; s[i]; i++ )
	 ;

	return i;
}


int strcmp ( const char *s1, const char *s2 )
{
	size_t i;

	for ( i = 0; s1[i] || s2[i]; i++ )
	{
	 if ( s1[i] < s2[i] )
	  return -1;
	 else if ( s1[i] > s2[i] )
	  return 1;
	}
	return 0;
}


int strncmp ( const char *s1, const char *s2, size_t n )
{
	size_t i;

	for ( i = 0; i < n && ( s1[i] || s2[i] ); i++ )
	{
	 if ( s1[i] < s2[i] )
	  return -1;
	 else if ( s1[i] > s2[i] )
	  return 1;
	}
	return 0;
}







char *strcpy ( char *dest, const char *src )
{
	int i;

	for ( i = 0; src[i]; i++ )
	 dest[i] = src[i];

	dest[i] = 0;

	return dest;
}







char *strcat ( char *dest, const char *src )
{
	int i;

	for ( i = 0; dest[i]; i++ )
	 ;

	strcpy ( &dest[i], src );

	return dest;
}







char *strchr (const char *s, int c)
{
	int i;

	for ( i = 0; s[i]; i++ )
	 if ( s[i] == (char) c )
	  return (char *) &s[i];
	return NULL;
}







char *strstr (const char *s1, const char *s2)
{
	int j;

	for (; s1 && s2 ;)
	{
	 for ( j = 0; s1[j] && s2[j] && s1[j] == s2[j]; j++ )
	  ;

	 if ( !s2[j] )
	  return (char *) s1;

	 if ( !s1[j] )
	  return NULL;

	 s1++;
	}

	return (char *) s1;
}







void itoa ( char *buffer, int base, int d )
{
	char *p = buffer;
	char *p1, *p2, firsthexchar;
	unsigned long ud = d;
	int divisor = 10;
	int digits = 0;


	if ( base == 'd' && d < 0 )
	{
	 *p++ = '-';
	  buffer++;
	 ud = -d;
	}
	else if ( base == 'x' || base == 'X' )
	{
	 divisor = 16;
	}

	firsthexchar = (base == 'x' ? 'a' : 'A');


	do {
	 int remainder = ud % divisor;

	 *p++ = (remainder < 10) ? remainder + '0' :
	    remainder + firsthexchar - 10;
	 digits++;
	}
	while ( ud /= divisor );


	if ( base == 'x' || base == 'X' )
	{
	 while ( digits < 8 )
	 {
	  digits++;
	  *p++ = '0';
	 }
	 *p++ = 'x';
	 *p++ = '0';
	}

	*p = 0;


	p1 = buffer;
	p2 = p - 1;
	while ( p1 < p2 )
	{
	 char tmp = *p1;
	 *p1 = *p2;
	 *p2 = tmp;
	 p1++;
	 p2--;
	}
}



int vssprintf ( char *str, size_t size, char **arg )
{
	char *format = *arg, buffer[20], *p;
	int c, i = 0;

	if ( !format )
	 return 0;

	arg++;

	while ( (c = *format++) != 0 && i < size - 1 )
	{
	 if ( c != '%' )
	 {
	  str[i++] = (char) c;
	 }
	 else {
	  c = *format++;
	  switch ( c ) {
	  case 'd':
	  case 'u':
	  case 'x':
	  case 'X':
	   itoa ( buffer, c, *((int *) arg++) );
	   p = buffer;
	   if ( i + strlen (p) < size - 1 )
	    while ( *p )
	     str[i++] = *p++;
	   else
	    goto too_long;
	   break;

	  case 's':
	   p = *arg++;
	   if ( !p )
	    p = "(null)";

	   if ( i + strlen (p) < size - 1 )
	    while ( *p )
	     str[i++] = *p++;
	   else
	    goto too_long;
	   break;

	  default:
	   str[i++] = *( (int *) arg++ );
	   break;
	  }
	 }
	}

too_long:

	str[i++] = 0;

	return i;
}
char *strtok ( char *s, const char *delim )
{
	static char *last;

	return strtok_r ( s, delim, &last );
}

char *strtok_r ( char *s, const char *delim, char **last )
{
	int i, j, found_delim;

	if ( s == NULL )
	 s = *last;
	if ( s == NULL)
	 return NULL;


	for ( i = 0; s[i] != 0; i++ )
	{
	 found_delim = 0;
	 for ( j = 0; delim[j] != 0; j++ )
	 {
	  if ( s[i] == delim[j] )
	  {
	   found_delim = 1;
	   break;
	  }
	 }

	 if ( found_delim == 0 )
	  break;
	}

	if ( s[i] == 0 )
	{
	 *last = NULL;
	 return NULL;
	}

	s = &s[i];


	for ( i = 1; s[i] != 0; i++ )
	{
	 found_delim = 0;
	 for ( j = 0; delim[j] != 0; j++ )
	 {
	  if ( s[i] == delim[j] )
	  {
	   found_delim = 1;
	   break;
	  }
	 }

	 if ( found_delim == 1 )
	  break;
	}

	if ( s[i] == 0 )
	 *last = NULL;
	else
	 *last = &s[i+1];
	s[i] = 0;

	return s;
}

