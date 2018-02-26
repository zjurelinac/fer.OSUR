#include "lib/queue.h"
#include <kernel/memory.h>
#include <api/errno.h>

#define malloc kmalloc
#define free kfree

void queue_init(queue_t *queue){
    ASSERT(queue);
    queue->first = queue->last = NULL;
}

int queue_empty(queue_t *queue){
    ASSERT(queue);
    return queue->first == NULL;
}

void queue_push(queue_t *queue, void *value){
    item_t *new = malloc(sizeof(item_t));

    ASSERT(queue && value);

    new->value = value;
    new->prev = NULL;
    new->next = queue->first;

    if(queue->first == NULL)
        queue->last = new;
    else
        queue->first->prev = new;

    queue->first = new;
}

void *queue_pop(queue_t *queue){
    void *value;
    item_t *tmp;

    ASSERT(!queue_empty(queue));

    value = queue->last->value;
    tmp = queue->last;

    if(queue->first == queue->last)
        queue->first = queue->last = NULL;
    else{
        queue->last->prev->next = NULL;
        queue->last = queue->last->prev;
    }

    free(tmp);

    return value;
}

item_t *queue_find(queue_t *queue, void *value){
    item_t *tmp = queue->first;
    ASSERT(queue && value);

    while(tmp != NULL){
        if(tmp->value == value) return tmp;
        tmp = tmp->next;
    }

    return NULL;
}

int queue_remove(queue_t *queue, void *value){
    item_t *tmp = queue->first, *ttmp;
    ASSERT(queue && value );

    if(queue_empty(queue)) return 0;

    if(queue->last->value == value){
        queue_pop(queue);
        return 1;
    }

    if(queue->first->value == value){
        tmp = queue->first;
        queue->first->next->prev = NULL;
        queue->first = queue->first->next;

        free(tmp);

        return 1;
    }

    while(tmp->next != NULL){
        if(tmp->next->value == value){
            ttmp = tmp->next;
            tmp->next = tmp->next->next;
            tmp->next->prev = tmp;

            free(ttmp);
            return 1;
        }
        tmp = tmp->next;
    }

    return 0;
}