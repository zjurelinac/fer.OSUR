int hello_world ( char *args[] );
int timer ( char *args[] );
int keyboard ( char *args[] );
int shell ( char *args[] );
int arguments ( char *args[] );
int user_threads ( char *args[] );
int threads ( char *args[] );
int semaphores ( char *args[] );
int monitors ( char *args[] );
int messages ( char *args[] );
int signals ( char *args[] );
int sse_test ( char *args[] );
int segm_fault ( char *args[] );
int round_robin ( char *args[] );
int run_all ( char *args[] );
int rwlock ( char *args[] );
#define PROGRAMS_FOR_SHELL { \
{ hello_world, "hello", " " }, \
{ timer, "timer", " " }, \
{ keyboard, "keyboard", " " }, \
{ shell, "shell", " " }, \
{ arguments, "args", " " }, \
{ user_threads, "uthreads", " " }, \
{ threads, "threads", " " }, \
{ semaphores, "semaphores", " " }, \
{ monitors, "monitors", " " }, \
{ messages, "messages", " " }, \
{ signals, "signals", " " }, \
{ sse_test, "sse_test", " " }, \
{ segm_fault, "segm_fault", " " }, \
{ round_robin, "rr", " " }, \
{ run_all, "run_all", " " }, \
{ rwlock, "rwlock", " " }, \
{NULL,NULL,NULL} }
