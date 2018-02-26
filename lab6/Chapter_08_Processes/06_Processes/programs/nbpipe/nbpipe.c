/*! Process pipes example */

#include <stdio.h>
#include <lib/string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <types/io.h>

char PROG_HELP[] = "Demonstrate nonblocking FIFO pipes.";

#define MSG(src, msg, ...) LOG(INFO, " <" # src " = %d> :: " # msg, pthread_self().id, ## __VA_ARGS__)

void master_proc_thread(){
    int fd;
    char msg[32];
    timespec_t delay = {1, 0};

    MSG(MASTER, "Started");

    if((fd = open("/nbfifo-pipe", O_CREAT | O_WRONLY | O_NONBLOCK, S_IFIFO)) == -1){
        MSG(MASTER, "Error - cannot open pipe");
        return;
    }

    strcpy(msg, "Hello my slave!");
    while(write(fd, msg, 32) < 0)
        nanosleep(&delay, NULL);

    MSG(MASTER, "Sent message, terminating.");
    close(fd);
}

void slave_proc_thread(){
    int fd;
    char msg[32];
    timespec_t delay = {1, 0};

    MSG(SLAVE, "Started");

    if((fd = open("/nbfifo-pipe", O_CREAT | O_RDONLY, S_IFIFO)) == -1){
        MSG(SLAVE, "Error - cannot open pipe");
        return;
    }

    while(read(fd, msg, 32) < 0)
        nanosleep(&delay, NULL);

    MSG(SLAVE, "Received message <%s>, terminating.", msg);
    close(fd);
}

int nbpipe( char *args[] ){
    char id = args[1][0];

    LOG(INFO, " `%s` process -> %c started.", args[0], id);

    if(id == 'W'){
        master_proc_thread();
    } else
        slave_proc_thread();

	return 0;
}