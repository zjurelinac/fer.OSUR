/*! Process pipes example */

#include <stdio.h>
#include <lib/string.h>
#include <pthread.h>
#include <errno.h>
#include <types/io.h>

char PROG_HELP[] = "Demonstrate FIFO pipes.";

#define MSG(src, msg, ...) LOG(INFO, " <" # src " = %d> :: " # msg, pthread_self().id, ## __VA_ARGS__)
#define MESSAGE_COUNT 5
//#define

typedef struct _message_t_{
    char text[32];
} message_t;

void master_proc_thread(){
    int fd;
    message_t master_msg;

    MSG(MASTER, "Started");

    if((fd = open("/fifo-pipe", O_CREAT | O_WRONLY, S_IFIFO)) == -1){
        MSG(MASTER, "Error - cannot open pipe");
        return;
    }
    MSG(MASTER, "Opened pipe -> fd = %d", fd);

    strcpy(master_msg.text, "Hello!");
    MSG(MASTER, "Sending message to slave: <%s>", master_msg.text);
    write(fd, (void*) &master_msg, sizeof(message_t));

    close(fd);
    MSG(MASTER, "Closed pipe, done.");
}

void slave_proc_thread(){
    int fd;
    message_t slave_msg;
    MSG(SLAVE, "Started");

    if((fd = open("/fifo-pipe", O_CREAT | O_RDONLY, S_IFIFO)) == -1){
        MSG(SLAVE, "Error - cannot open pipe");
        return;
    }
    MSG(SLAVE, "Opened pipe -> fd = %d", fd);

    read(fd, (void*) &slave_msg, sizeof(message_t));
    MSG(SLAVE, "Received message from master: <%s>", slave_msg.text);

    close(fd);
    MSG(SLAVE, "Closed pipe, done.");
}

int piping( char *args[] ){
    char id = args[1][0];

    LOG(INFO, " `%s` process -> %c started.", args[0], id);

    if(id == 'W'){
        master_proc_thread();
    } else
        slave_proc_thread();

	return 0;
}