/*! Devices - common interface implementation */
#define _K_DEVICE_C_

#include "device.h"

#include "memory.h"
#include "sched.h"
#include <arch/interrupt.h>
#include <arch/processor.h>
#include <kernel/errno.h> /* shares errno with arch layer */
#include <lib/string.h>

static list_t devices;
static int kfifo_open_error = FALSE;

extern int kernel_scheduler_debug_mode;

static void k_device_interrupt_handler ( unsigned int inum, void *device );

/*! Initialize initial device as console for system boot messages */
void kdevice_set_initial_stdout ()
{
	static kdevice_t k_initial_stdout;
	extern device_t K_INITIAL_STDOUT;
	extern void *k_stdout; /* console for kernel messages */

	k_initial_stdout.dev = K_INITIAL_STDOUT;
	k_initial_stdout.dev.init ( 0, NULL, &k_initial_stdout.dev );
	k_stdout = &k_initial_stdout;
}

/*! Initialize 'device' subsystem */
int k_devices_init ()
{
	extern device_t DEVICES_DEV; /* defined in arch/devices, Makefile */
	device_t *dev[] = { DEVICES_DEV_PTRS, NULL };
	kdevice_t *kdev;
	int iter;

	list_init ( &devices );

	for ( iter = 0; dev[iter] != NULL; iter++ )
	{
		kdev = k_device_add ( dev[iter] );
		k_device_init ( kdev, 0, NULL, NULL );
	}

	return 0;
}

/*! Add new device to system */
kdevice_t *k_device_add ( device_t *dev )
{
	kdevice_t *kdev;
	kxdevice_t *kxdev;

	ASSERT ( dev );

	if(dev->flags & DEV_TYPE_FIFO){
		kxdev = (kxdevice_t*) kmalloc(sizeof(kxdevice_t));

		kthreadq_init(&kxdev->blocked_opening);
		kthreadq_init(&kxdev->blocked_reading);
		kthreadq_init(&kxdev->blocked_writing);

		kdev = (kdevice_t*) kxdev;
	} else
		kdev = kmalloc ( sizeof (kdevice_t) );

	ASSERT ( kdev );

	kdev->dev = *dev;
	kdev->id = k_new_id ();
	kdev->flags = 0;

	list_append ( &devices, kdev, &kdev->list );

	return kdev;
}

/*! Initialize device (and call its initializer, if set) */
int k_device_init ( kdevice_t *kdev, int flags, void *params, void *callback )
{
	int retval = 0;

	ASSERT ( kdev );

	if ( flags )
		kdev->dev.flags = flags;

	if ( params )
		kdev->dev.params = params;

	list_init ( &kdev->descriptors );

	if ( kdev->dev.init )
		retval = kdev->dev.init ( flags, params, &kdev->dev );

	if ( retval == EXIT_SUCCESS && kdev->dev.irq_handler )
	{
		(void) arch_register_interrupt_handler ( kdev->dev.irq_num,
							 k_device_interrupt_handler,
							 kdev );
		arch_irq_enable ( kdev->dev.irq_num );
	}

	if ( callback )
		kdev->dev.callback = callback;

	return retval;
}

/*! Remove device from list of devices */
int k_device_remove ( kdevice_t *kdev )
{
#ifdef DEBUG
	kdevice_t *test;
#endif
	ASSERT ( kdev );

	if ( kdev->dev.irq_num != -1 )
		arch_irq_disable ( kdev->dev.irq_num );

	if ( kdev->dev.irq_handler )
		arch_unregister_interrupt_handler ( kdev->dev.irq_num,
						    kdev->dev.irq_handler,
						    &kdev->dev );
	if ( kdev->dev.destroy )
		kdev->dev.destroy ( kdev->dev.flags, kdev->dev.params,
				    &kdev->dev );
#ifdef DEBUG
	test = list_find_and_remove ( &devices, &kdev->list );
	ASSERT ( test == kdev );
#else
	(void) list_remove ( &devices, 0, &kdev->list );
#endif

	k_free_id ( kdev->id );

	kfree ( kdev );

	return 0;
}

/*! Send data to device */
int k_device_send ( void *data, size_t size, int flags, kdevice_t *kdev )
{
	int retval;

	if ( kdev->dev.send )
		retval = kdev->dev.send ( data, size, flags, &kdev->dev );
	else
		retval = EXIT_FAILURE;

	return retval;
}

/*! Read data from device */
int k_device_recv ( void *data, size_t size, int flags, kdevice_t *kdev )
{
	int retval;

	if ( kdev->dev.recv )
		retval = kdev->dev.recv ( data, size, flags, &kdev->dev );
	else
		retval = EXIT_FAILURE;

	return retval;
}

/*! Open device with 'name' (for exclusive use, if defined) */
kdevice_t *k_device_open ( char *name, int flags )
{
	kdevice_t *kdev;
	kxdevice_t *kxdev;
	kfifo_t *fifo_params;

	kfifo_open_error = FALSE;

	kdev = list_get ( &devices, FIRST );
	while ( kdev ){
		if ( !strcmp ( name, kdev->dev.dev_name ) ){
			if ((kdev->dev.flags & DEV_TYPE_NOTSHARED) && (kdev->flags & DEV_OPEN) )
				return NULL;

			if (kdev->dev.flags & DEV_TYPE_FIFO){
				fifo_params = (kfifo_t*) kdev->dev.params;
				kxdev = (kxdevice_t*) kdev;

				if(flags & O_WRONLY){
					LOG(INFO, " Trying to open FIFO `%s` for writing", kdev->dev.dev_name);

					if(kxdev->flags & DEV_WRITE_OPENED){
						LOG(ERROR, " FIFO `%s` already opened for writing.", kdev->dev.dev_name);
						kfifo_open_error = TRUE;
						return NULL;
					}

					kxdev->flags |= DEV_WRITE_OPENED;
					fifo_params->writer = kthread_get_active();

					if(kxdev->flags & DEV_READ_OPENED){
						LOG(INFO, " Releasing thread blocked on opening for read.");
						kthreadq_release(&kxdev->blocked_opening);
					} else {
						LOG(INFO, " Not yet opened for writing, blocking this thread");
						kthread_enqueue(NULL, &kxdev->blocked_opening, 0, NULL, NULL);
					}

					LOG(INFO, " Scheduling after WRITE open");
					//kernel_scheduler_debug_mode = TRUE;
					kthreads_schedule();
					LOG(INFO, " Proceeding here (WR)");
					//halt();

				} else if(flags & O_RDONLY){
					LOG(INFO, " Trying to open FIFO `%s` for reading", kdev->dev.dev_name);

					if(kxdev->flags & DEV_READ_OPENED){
						LOG(ERROR, " FIFO `%s` already opened for reading.", kdev->dev.dev_name);
						kfifo_open_error = TRUE;
						return NULL;
					}

					kxdev->flags |= DEV_READ_OPENED;
					fifo_params->reader = kthread_get_active();

					if(kxdev->flags & DEV_WRITE_OPENED){
						LOG(INFO, " Releasing thread blocked on opening for write.");
						kthreadq_release(&kxdev->blocked_opening);
					} else {
						LOG(INFO, " Not yet opened for reading, blocking this thread");
						kthread_enqueue(NULL, &kxdev->blocked_opening, 0, NULL, NULL);
					}

					LOG(INFO, " Scheduling after READ open");
					//kernel_scheduler_debug_mode = TRUE;
					kthreads_schedule();
					LOG(INFO, " Proceeding here (RD)");
					//halt();

				} else if(flags & O_RDWR){
					LOG(ERROR, " Cannot open FIFO for reading and writing at the same time.");
					kfifo_open_error = TRUE;
					return NULL;
				} else {
					LOG(ERROR, " Unknown operation requested.");
					kfifo_open_error = TRUE;
					return NULL;
				}
			}

			/* FIXME: check read/write/exclusive open conflicts */

			kdev->flags |= DEV_OPEN | flags;
			kdev->ref_cnt++;

			return kdev;
		}

		kdev = list_get_next ( &kdev->list );
	}

	return NULL;
}

/*! Close device (close exclusive use, if defined) */
void k_device_close ( kdevice_t *kdev )
{
	kdev->ref_cnt--;
	if ( !kdev->ref_cnt )
		kdev->flags &= ~DEV_OPEN;

	if(kdev->dev.flags & DEV_TYPE_FIFO){
		kfifo_t *fifo_params = (kfifo_t*) kdev->dev.params;
		kthread_t *active = kthread_get_active();

		if(active == fifo_params->reader){
			kdev->flags &= ~DEV_READ_OPENED;
			fifo_params->reader = NULL;
			LOG(INFO, " Closing FIFO read end");
		} else if(active == fifo_params->writer){
			kdev->flags &= ~DEV_WRITE_OPENED;
			fifo_params->writer = NULL;
			LOG(INFO, " Closing FIFO write end");
		} else {
			LOG(INFO, " Error closing FIFO - unknown end");
		}
	}

	/* FIXME: restore flags; use list kdev->descriptors? */
}

/* common device interrupt handler wrapper */
static void k_device_interrupt_handler ( unsigned int inum, void *device )
{
	kdevice_t *kdev = device;
	int status = EXIT_SUCCESS;

	ASSERT ( inum && device && kdev->dev.irq_num == inum );
	/* TODO: check if kdev is in "devices" list */

	if ( kdev->dev.irq_handler )
		status = kdev->dev.irq_handler ( inum, &kdev->dev );

	if ( status ) {} /* handle return status if required */
}

static int k_device_status ( int flags, kdevice_t *kdev )
{
	ASSERT ( kdev );

	if ( kdev->dev.status )
		return kdev->dev.status ( flags, &kdev->dev );
	else
		return -1;
}

/* /dev/null emulation */
static int do_nothing ()
{
	return 0;
}

device_t dev_null = (device_t)
{
	.dev_name = "dev_null",

	.irq_num = 	-1,
	.irq_handler =	NULL,

	.init =		NULL,
	.destroy =	NULL,
	.send =		do_nothing,
	.recv =		do_nothing,
	.status =	do_nothing,

	.flags = 	DEV_TYPE_SHARED,
	.params = 	NULL,
};

/*! FIFO pipes */

device_t fifo_template_dev = (device_t) {
	.dev_name = "\0",

	.irq_num = 	-1,
	.irq_handler =	NULL,

	.init =		NULL,
	.destroy =	NULL,
	.send =		k_fifo_send,
	.recv =		k_fifo_recv,
	.status =	do_nothing,

	.flags = 	DEV_TYPE_SHARED | DEV_TYPE_FIFO,
	.params = 	NULL,
};

int k_make_fifo(char *name){
	device_t *new_fifo_dev;
	kfifo_t *new_fifo_data = kmalloc(sizeof(kfifo_t));

	new_fifo_dev = (device_t*) kmalloc(sizeof(device_t));
	memcpy(new_fifo_dev, &fifo_template_dev, sizeof(device_t));

	strcpy(new_fifo_dev->dev_name, name);

	new_fifo_data->reader = NULL;
	new_fifo_data->writer = NULL;
	new_fifo_data->read_pos = 0;
	new_fifo_data->write_pos = 0;
	new_fifo_data->avail_size = FIFO_BUFFSIZE;

	new_fifo_dev->params = new_fifo_data;

	k_device_add(new_fifo_dev);

	return EXIT_SUCCESS;
}

int k_fifo_send( void *data, size_t size, uint flags, device_t *dev ){
	kfifo_t *fifo_params = (kfifo_t*) dev->params;
	char *bytedata = (char*) data;
	int amount = 0;

	if(fifo_params->avail_size > 0){
		amount = size < fifo_params->avail_size ? size : fifo_params->avail_size;
		fifo_params->avail_size -= amount;
		size -= amount;

		LOG(INFO, " Writing %d bytes to buffer, %d remaining.", amount, size);
		while(amount--){
			fifo_params->buff[fifo_params->write_pos] = *(bytedata++);
			fifo_params->write_pos = (fifo_params->write_pos + 1) % FIFO_BUFFSIZE;
		}
	}

	return -size;
}

int k_fifo_recv( void *data, size_t size, uint flags, device_t *dev ){
	kfifo_t *fifo_params = (kfifo_t*) dev->params;
	char *bytedata = (char*) data;
	int data_size = FIFO_BUFFSIZE - fifo_params->avail_size, amount = 0;

	if(data_size > 0){
		amount = size < data_size ? size : data_size;
		fifo_params->avail_size += amount;
		size -= amount;

		LOG(INFO, " Reading %d bytes from buffer, %d remaining.", amount, size);
		while(amount--){
			*(bytedata++) = fifo_params->buff[fifo_params->read_pos];
			fifo_params->read_pos = (fifo_params->read_pos + 1) % FIFO_BUFFSIZE;
		}
	}

	return -size;
}



/*! syscall wrappers -------------------------------------------------------- */

int sys__open ( void *p )
{
	char *pathname;
	int flags;
	mode_t mode;
	descriptor_t *desc;

	kdevice_t *kdev;
	kobject_t *kobj;
	kprocess_t *proc;


	pathname =	*( (char **) p );		p += sizeof (char *);
	flags =		*( (int *) p );			p += sizeof (int);
	mode =		*( (mode_t *) p );		p += sizeof (mode_t);
	desc =		*( (descriptor_t **) p );

	proc = kthread_get_process (NULL);

	ASSERT_ERRNO_AND_EXIT ( pathname, EINVAL );
	pathname = U2K_GET_ADR ( pathname, proc );
	ASSERT_ERRNO_AND_EXIT ( pathname, EINVAL );

	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );
	desc = U2K_GET_ADR ( desc, proc );
	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );

	kdev = k_device_open ( pathname, flags );

	if ( !kdev ){
		if(kfifo_open_error){
			LOG(ERROR, " Cannot open FIFO, giving up!");
			return EXIT_FAILURE;
		} else if( (flags & O_CREAT) && (mode & S_IFIFO)){
			LOG(INFO, " Creating new fifo `%s`", pathname);

			k_make_fifo(pathname);
			kdev = k_device_open(pathname, flags);

		} else
			return EXIT_FAILURE;
	}

	kobj = kmalloc_kobject ( proc, 0 );
	ASSERT_ERRNO_AND_EXIT ( kobj, ENOMEM );

	kobj->kobject = kdev;
	kobj->flags = flags;

	desc->ptr = kobj;
	desc->id = kdev->id;

	/* add descriptor to device list */
	list_append ( &kdev->descriptors, kobj, &kobj->spec );

	EXIT2 ( EXIT_SUCCESS, EXIT_SUCCESS );
}

int sys__close ( void *p )
{
	descriptor_t *desc;

	kdevice_t *kdev;
	kobject_t *kobj;
	kprocess_t *proc;

	desc = *( (descriptor_t **) p );

	proc = kthread_get_process (NULL);

	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );
	desc = U2K_GET_ADR ( desc, proc );
	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );

	kobj = desc->ptr;
	ASSERT_ERRNO_AND_EXIT ( kobj, EINVAL );
	ASSERT_ERRNO_AND_EXIT ( list_find ( &proc->kobjects, &kobj->list ),
				EINVAL );
	kdev = kobj->kobject;
	ASSERT_ERRNO_AND_EXIT ( kdev && kdev->id == desc->id, EINVAL );

	kfree_kobject ( proc, kobj );

	/* remove descriptor from device list */
	list_remove ( &kdev->descriptors, 0, &kobj->spec );

	k_device_close ( kdev );

	EXIT2 ( EXIT_SUCCESS, EXIT_SUCCESS );
}

static int read_write ( void *p, int op );

int sys__read ( void *p )
{
	return read_write ( p, TRUE );
}
int sys__write ( void *p )
{
	return read_write ( p, FALSE );
}

static inline int call_rw_op(int op, void *buffer, size_t size, kobject_t *kobj, kdevice_t *kdev){
	return op ? k_device_recv ( buffer, size, kobj->flags, kdev )
			  : k_device_send ( buffer, size, kobj->flags, kdev );
}

static int read_write ( void *p, int op )
{
	descriptor_t *desc;
	void *buffer;
	size_t size;

	kdevice_t *kdev;
	kxdevice_t *kxdev;
	kobject_t *kobj;
	int retval;
	kprocess_t *proc;

	desc =  *( (descriptor_t **) p );	p += sizeof (descriptor_t *);
	buffer = *( (char **) p );			p += sizeof (char *);
	size = *( (size_t *) p );

	proc = kthread_get_process (NULL);

	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );
	desc = U2K_GET_ADR ( desc, proc );
	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );
	ASSERT_ERRNO_AND_EXIT ( buffer, EINVAL );
	buffer = U2K_GET_ADR ( buffer, proc );
	ASSERT_ERRNO_AND_EXIT ( buffer, EINVAL );
	ASSERT_ERRNO_AND_EXIT ( size > 0, EINVAL );

	kobj = desc->ptr;
	ASSERT_ERRNO_AND_EXIT ( kobj, EINVAL );
	ASSERT_ERRNO_AND_EXIT ( list_find ( &proc->kobjects, &kobj->list ),
				EINVAL );
	kdev = kobj->kobject;
	ASSERT_ERRNO_AND_EXIT ( kdev && kdev->id == desc->id, EINVAL );

	/* TODO check permission for requested operation from opening flags */
	if( kdev->dev.flags & DEV_TYPE_FIFO ){
		kxdev = (kxdevice_t*) kdev;

		LOG(INFO, " IO operation (%s) on FIFO `%s`", op ? "RECV" : "SEND", kxdev->dev.dev_name);

		if( kdev->flags & O_NONBLOCK ){

			retval = call_rw_op(op, buffer, size, kobj, kdev);

			LOG(INFO, " Nonblocking op ended with %d retval.", retval);

			if ( retval >= 0 )
				EXIT2 ( EXIT_SUCCESS, retval );
			else
				EXIT2 ( EIO, EXIT_FAILURE );
		} else {

			while(TRUE){
				retval = call_rw_op(op, buffer, size, kobj, kdev);  // retval = -(remaining size);

				if(op)
					kthreadq_release(&kxdev->blocked_writing);
				else
					kthreadq_release(&kxdev->blocked_reading);

				if(retval >= 0)  	// no more data remaining to be read/written
					break; 			// escape from the loop, do not reschedule threads yet.

				retval = -retval;
				LOG(INFO, " IO operation partially done, moved buffer ahead %d bytes, remaining %d.", size - retval, retval);
				buffer = (void*) (((char*) buffer) + (size - retval));  // move buffer ahead by (size - retval) bytes - that much was consumed in last operation
				size = retval;

				if(op){
					LOG(INFO, " Blocking reader; previously released writer");
					kthread_enqueue(NULL, &kxdev->blocked_reading, 0, NULL, NULL);
				} else {
					LOG(INFO, " Blocking writer; previously released reader");
					kthread_enqueue(NULL, &kxdev->blocked_writing, 0, NULL, NULL);
				}

				LOG(INFO, " Scheduling after partial IO");
				kthreads_schedule();
				kprintf("\n");
			};

		}

		return EXIT_SUCCESS;

	} else {

		retval = call_rw_op(op, buffer, size, kobj, kdev);

		if ( retval >= 0 )
			EXIT2 ( EXIT_SUCCESS, retval );
		else
			EXIT2 ( EIO, EXIT_FAILURE );
	}
}

int kdevice_status ( descriptor_t *desc, int flags, kprocess_t *proc )
{
	kdevice_t *kdev;
	kobject_t *kobj;
	int status, rflags = 0;

	ASSERT_AND_RETURN_ERRNO ( desc, EINVAL );

	kobj = desc->ptr;
	ASSERT_AND_RETURN_ERRNO ( kobj, EINVAL );
	ASSERT_AND_RETURN_ERRNO ( list_find ( &proc->kobjects, &kobj->list ),
				EINVAL );
	kdev = kobj->kobject;
	ASSERT_AND_RETURN_ERRNO ( kdev && kdev->id == desc->id, EINVAL );

	status = k_device_status ( flags, kdev );

	if ( status == -1 )
		return -1;

	/* TODO only DEV_IN_READY and DEV_OUT_READY are set in device drivers */
	if ( ( flags & ( POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI ) ) )
		if ( ( status & DEV_IN_READY ) )
			rflags |= POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
	if ( ( flags & ( POLLOUT | POLLWRNORM | POLLWRBAND | POLLPRI ) ) )
		if ( ( status & DEV_OUT_READY ) )
			rflags |= POLLOUT | POLLWRNORM | POLLWRBAND | POLLPRI;

	return rflags;
}

int sys__device_status ( void *p )
{
	descriptor_t *desc;
	int flags;
	kprocess_t *proc;

	int status;

	desc =  *( (descriptor_t **) p );	p += sizeof (descriptor_t *);
	flags = *( (int *) p );

	proc = kthread_get_process (NULL);

	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );
	desc = U2K_GET_ADR ( desc, proc );
	ASSERT_ERRNO_AND_EXIT ( desc, EINVAL );

	status = kdevice_status ( desc, flags, proc );
	if ( status == -1 )
		EXIT2 ( EXIT_FAILURE, status );
	else
		EXIT2 ( EXIT_SUCCESS, status );
}

/*!
 * poll - input/output multiplexing
 * \param fds set of file descriptors
 * \param nfds number of file descriptors in fds
 * \param timeout minimum time in ms to wait for any event defined by fds
 *        (in current implementation this parameter is ignored)
 * \param std_desc address of file descriptor array from user space
 * \return number of file descriptors with changes in revents, -1 on errors
 */
int sys__poll ( void *p )
{
	struct pollfd *fds;
	nfds_t nfds;
	int timeout __attribute__ ((unused));
	descriptor_t *std_desc;

	int changes = 0, i;
	short revents;
	kprocess_t *proc;

	fds =       *( (struct pollfd **) p );	p += sizeof (struct pollfd *);
	nfds =      *( (nfds_t *) p );		p += sizeof (nfds_t);
	timeout =   *( (int *) p );		p += sizeof (int);
	std_desc =  *( (descriptor_t **) p );

	proc = kthread_get_process (NULL);

	ASSERT_ERRNO_AND_EXIT ( fds && nfds > 0 && std_desc, EINVAL );
	fds = U2K_GET_ADR ( fds, proc );
	ASSERT_ERRNO_AND_EXIT ( fds, EINVAL );
	std_desc = U2K_GET_ADR ( std_desc, proc );
	ASSERT_ERRNO_AND_EXIT ( std_desc, EINVAL );

	for ( i = 0; i < nfds; i++ )
	{
		revents = kdevice_status (
			&std_desc[fds[i].fd], fds[i].events, proc
		);
		ASSERT_ERRNO_AND_EXIT ( revents != -1, EINVAL );

		fds[i].revents = revents;
		if ( revents )
			changes++;
	}

	EXIT2 ( EXIT_SUCCESS, changes );
}
