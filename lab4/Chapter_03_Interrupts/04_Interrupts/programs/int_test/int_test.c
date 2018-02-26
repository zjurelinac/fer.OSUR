#include <stdio.h>
#include <arch/interrupt.h>
#include <arch/processor.h>
#include <kernel/errno.h>
#include <api/prog_info.h>

#define INT_N 10
#define INT_D 50000000
static int idx = 0;

static void other_handler(){
    int id = ++idx, i;
    printf("Started processing irq %d\n", id);
    for(i = 0; i < INT_D; ++i) ;
    printf("Finished processing irq %d\n", id);
}

static void initial_handler(){
    int i;
    arch_unregister_interrupt_handler(SOFTWARE_INTERRUPT, initial_handler);
    arch_register_interrupt_handler(SOFTWARE_INTERRUPT, other_handler);
    printf("Started raising interrupts\n");
    for(i = 0; i < INT_N; ++i)
        raise_interrupt(SOFTWARE_INTERRUPT);
}

void int_test(){
    printf("\n=======================================\n\n");
    arch_register_interrupt_handler(SOFTWARE_INTERRUPT, initial_handler);
    printf("Raising first interrupt!\n");
    raise_interrupt(SOFTWARE_INTERRUPT);
    printf("Job done!\n");
}
