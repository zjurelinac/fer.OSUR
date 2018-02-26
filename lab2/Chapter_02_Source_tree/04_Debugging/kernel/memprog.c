/*! Lab 1 - 2. memprog() funkcija */

#include <stdio.h>
#include <lib/string.h>
#include <api/prog_info.h>

int mp_global_var = 8;

void move_programs(){
    extern char program_code_start, program_code_store_start, program_data_store_end;
    unsigned int size = &program_data_store_end - &program_code_store_start;
    printf("program_code_start=%x; program_code_store_start=%x; program_data_store_end=%x; size=%u\n",
           &program_code_start, &program_code_store_start, &program_data_store_end, size);
    memcpy((void*) &program_code_start, (void*) &program_code_store_start, size);
}

void memprog(){
    int local_var = 4;
    printf("memprog() ::\n\t&global = %x,\tglobal = %d\n\t&local = %x,\tlocal = %d\n\t&memloc() = %x\n",
           &mp_global_var, mp_global_var, &local_var, local_var, memprog);
    memloc();
}
