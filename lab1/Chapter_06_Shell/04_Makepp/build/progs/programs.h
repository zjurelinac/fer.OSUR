int hello_world ( char *args[] );
int timer ( char *args[] );
int keyboard ( char *args[] );
int shell ( char *args[] );
int arguments ( char *args[] );
int segm_fault ( char *args[] );
int run_all ( char *args[] );
#define PROGRAMS_FOR_SHELL { \
{ hello_world, "hello", " " }, \
{ timer, "timer", " " }, \
{ keyboard, "keyboard", " " }, \
{ shell, "shell", " " }, \
{ arguments, "args", " " }, \
{ segm_fault, "segm_fault", " " }, \
{ run_all, "run_all", " " }, \
{NULL,NULL,NULL} }
