/*! Interrupt handling - 'arch' layer (only basic operations) */

#define _ARCH_INTERRUPTS_C_
#include "interrupt.h"

#include <arch/processor.h>
#include <kernel/errno.h>
#include <lib/list.h>
#include <kernel/memory.h>

/*! Interrupt controller device */
extern arch_ic_t IC_DEV;
static arch_ic_t *icdev = &IC_DEV;

/*! interrupt handlers */
static list_t ihandlers[INTERRUPTS];

struct ihndlr
{
	int (*ihandler) ( unsigned int );

	list_h list;
};

/*! Simple FIFO queue */

typedef struct _item_t_ {
	struct _item_t_ *next;
	int elem;
} item_t;

typedef struct _queue_t_ {
	item_t *first, *last;
} queue_t;

static void queue_push(queue_t *queue, int elem){
	item_t *new = kmalloc(sizeof(item_t));

	new->elem = elem;
	new->next = NULL;

	if(queue->first == NULL)
		queue->first = new;

	if(queue->last != NULL)
		queue->last->next = new;
	queue->last = new;
}

static int queue_pop(queue_t *queue){

	ASSERT(queue->first != NULL);

	item_t *first = queue->first;
	int elem = first->elem;

	queue->first = first->next;

	if(queue->first == NULL)
		queue->last = NULL;

	kfree(first);

	return elem;
}

static int queue_empty(queue_t *queue){
	return queue->first == NULL;
}

/*! Interrupt handling queue */

static queue_t interrupt_queue;
int processing_interrupts;

static void add_interrupt_to_queue(int irq_num){
	queue_push(&interrupt_queue, irq_num);
}

static void process_single_interrupt(int irq_num){
	struct ihndlr *ih;

	//LOG(INFO, "Will process interrupt %d", irq_num);

	if(irq_num < INTERRUPTS && (ih = list_get (&ihandlers[irq_num], FIRST))){
		/* Call registered handlers */
		while ( ih )
		{
			ih->ihandler ( irq_num );

			ih = list_get_next ( &ih->list );
		}
	} else {
		LOG ( ERROR, "Unregistered interrupt: %d - %s!\n",
		      irq_num, icdev->int_descr ( irq_num ) );
		halt ();
	}
}

static void process_interrupts(){
	if(processing_interrupts) return;
	processing_interrupts = 1;

	while(!queue_empty(&interrupt_queue))
		process_single_interrupt(queue_pop(&interrupt_queue));

	processing_interrupts = 0;
}


/********************************************************/

/*! Initialize interrupt subsystem (in 'arch' layer) */
void arch_init_interrupts ()
{
	int i;

	interrupt_queue.first = interrupt_queue.last = NULL;
	processing_interrupts = 0;

	icdev->init ();

	for ( i = 0; i < INTERRUPTS; i++ )
		list_init ( &ihandlers[i] );
}

/*!
 * enable and disable interrupts generated outside processor, controller by
 * interrupt controller (PIC or APIC or ...)
 */
void arch_irq_enable ( unsigned int irq )
{
	icdev->enable_irq ( irq );
}
void arch_irq_disable ( unsigned int irq )
{
	icdev->disable_irq ( irq );
}

/*! Register handler function for particular interrupt number */
void arch_register_interrupt_handler ( unsigned int inum, void *handler )
{
	struct ihndlr *ih;

	if ( inum < INTERRUPTS )
	{
		ih = kmalloc ( sizeof (struct ihndlr) );
		ASSERT ( ih );

		ih->ihandler = handler;

		list_append ( &ihandlers[inum], ih, &ih->list );
	}
	else {
		LOG ( ERROR, "Interrupt %d can't be used!\n", inum );
		halt ();
	}
}

/*! Unregister handler function for particular interrupt number */
void arch_unregister_interrupt_handler ( unsigned int irq_num, void *handler )
{
	struct ihndlr *ih, *next;

	ASSERT ( irq_num >= 0 && irq_num < INTERRUPTS );

	ih = list_get ( &ihandlers[irq_num], FIRST );

	while ( ih )
	{
		next = list_get_next ( &ih->list );

		if ( ih->ihandler == handler )
			list_remove ( &ihandlers[irq_num], FIRST, &ih->list );

		ih = next;
	}
}

/*!
 * "Forward" interrupt handling to registered handler
 * (called from interrupts.S)
 */
void arch_interrupt_handler ( int irq_num )
{
	LOG(INFO, "Received an interrupt %d\n", irq_num);

	if(irq_num < INTERRUPTS)
	{
		add_interrupt_to_queue(irq_num);

		/* enable interrupts on PIC immediately since program may not
		 * return here immediately */
		if ( icdev->at_exit )
			icdev->at_exit ( irq_num );

		arch_enable_interrupts();

		process_interrupts();
	}
	else {
		LOG ( ERROR, "Unregistered interrupt: %d !\n", irq_num );
		halt ();
	}
}
