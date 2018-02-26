/*! Lab 1 - 2. memprog() funkcija */

#include <stdio.h>
#include <api/prog_info.h>

static int mp_global_var = 8;

void memprog(){
    int local_var = 4;
    printf("memprog() ::\n\t&global = %x,\tglobal = %d\n\t&local = %x,\tlocal = %d\n\t&memloc() = %x\n", &mp_global_var, mp_global_var, &local_var, local_var, memprog);
    memloc();
}