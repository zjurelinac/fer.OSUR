/*! Lab 1 - 1. memloc() funkcija */

#include <stdio.h>
#include <api/prog_info.h>

int ml_global_var = 7;

void memloc(){
    int local_var = 3;
    printf("memloc() ::\n\t&global = %x,\tglobal = %d\n\t&local = %x,\tlocal = %d\n\t&memloc() = %x\n", &ml_global_var, ml_global_var, &local_var, local_var, memloc);
}