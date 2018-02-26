/*! Hello world program */

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#include <malloc.h>
#include <lib/queue.h>

#define READERS 4
#define WRITERS 2
#define ITERS 5

#define RDPRINT(fmt, ...) printf("Reader %d :: " fmt "\n", id, ## __VA_ARGS__);
#define WRPRINT(fmt, ...) printf("Writer %d :: " fmt "\n", id, ## __VA_ARGS__);

#define TRY(ACTION)                                                 \
    do{                                                             \
        int __rv__ = ACTION;                                        \
        if(__rv__ != 0)                                             \
            LOG(ERROR, "Unexpected return value :: %d", __rv__);    \
    } while(0)

char PROG_HELP[] = "An example pthread_rwlock program.";

static pthread_rwlock_t lock;
static int shared_value = 0;

static timespec_t critical_time;

queue_t Q;

void *reader_thread(void* param){
    int id = (int) param, i;
    timespec_t sleeptime;

    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 100000000 * (5 - id);

    RDPRINT("I'm <Thread %d>", pthread_self());

    for(i = 0; i < ITERS; ++i){
        RDPRINT("Trying to readlock");
        TRY(pthread_rwlock_rdlock(&lock));

        nanosleep(&critical_time, NULL);
        RDPRINT("Read %d", shared_value);

        RDPRINT("Read-unlocking");
        TRY(pthread_rwlock_unlock(&lock));

        nanosleep(&sleeptime, NULL);
    }

    return NULL;
}

void *writer_thread(void* param){
    int id = (int) param, i;
    timespec_t sleeptime;

    sleeptime.tv_nsec = 200000000 * (id + 0.5);
    sleeptime.tv_sec = 0;

    WRPRINT("I'm <Thread %d>", pthread_self());

    for(i = 0; i < ITERS; ++i){
        WRPRINT("Trying to writelock");
        TRY(pthread_rwlock_wrlock(&lock));

        nanosleep(&critical_time, NULL);
        shared_value += (id + 1);
        WRPRINT("Writing %d", shared_value);

        WRPRINT("Write-unlocking");
        TRY(pthread_rwlock_unlock(&lock));

        nanosleep(&sleeptime, NULL);
    }

    WRPRINT("OVER");
    return NULL;
}

int rwlock ( char *args[] ){
    int i;
    //int *ptr, *q;
    //item_t *elem;
    pthread_t readers[READERS], writers[WRITERS];

	printf ( "Example program: [%s:%s]\n%s\n\n", __FILE__, __FUNCTION__,
		 PROG_HELP );

    /*queue_init(&Q);

    for(i = 0; i < 15; ++i){
        ptr = (int*) malloc(sizeof(int));
        *ptr = i*i;
        if(i == 5)
            q = ptr;
        queue_push(&Q, (void*) ptr);
    }

    printf("Did I found %d? -> %x\n", *q, (elem = queue_find(&Q, q)));
    printf("Did I remove %d? -> %d\n", *q, queue_remove(&Q, elem->value));

    while(!queue_empty(&Q))
        printf("Popping %d\n", *((int*) queue_pop(&Q)));*/

    // resource initialization
    pthread_rwlock_init(&lock, NULL);
    critical_time.tv_sec = 0;
    critical_time.tv_nsec = 100000000;

    // thread creation

    for(i = 0; i < READERS; ++i)
        pthread_create(&readers[i], NULL, reader_thread, (void*) (i + 1));

    for(i = 0; i < WRITERS; ++i)
        pthread_create(&writers[i], NULL, writer_thread, (void*) (i + 1));


    // thread waiting

    for(i = 0; i < READERS; ++i)
		pthread_join(readers[i], NULL);

    for(i = 0; i < WRITERS; ++i)
		pthread_join(writers[i], NULL);

    // program end

    printf("Finishing up.\n");
    pthread_rwlock_destroy(&lock);

	return 0;
}
