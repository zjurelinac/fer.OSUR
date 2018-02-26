#pragma once

typedef struct _item_t_ {
    struct _item_t_ *prev, *next;
    void *value;
} item_t;

typedef struct _queue_t_ {
    item_t *first, *last;
} queue_t;

void queue_init(queue_t *queue);
int queue_empty(queue_t *queue);
void queue_push(queue_t *queue, void *value);
void *queue_pop(queue_t *queue);
item_t *queue_find(queue_t *queue, void *value);
int queue_remove(queue_t *queue, void *value);
