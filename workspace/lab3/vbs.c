#define _VBS_C_
#include "vbs.h"
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////////////////

static vbs_mempool_t *mpool;

static void vbs_list_print(vbs_list_t*);
static void vbs_mpool_print();

static vbs_head_t* vbs_mpool_find_block(int, int*);
static void vbs_mpool_remove_block(vbs_head_t*);
static int vbs_mpool_create_block(void*, int);
static void vbs_mpool_create_space(void*, int);

static void vbs_list_insert_block(int, vbs_head_t*);
static void vbs_list_remove_block(int, vbs_head_t*);

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

void m_init(void *segment_start, size_t segment_size){
    int i, size;
    vbs_head_t *tmp;

    assert(segment_start && segment_size > sizeof(vbs_mempool_t) + 2 * OCCUP_META_SIZE);

    DEBUG("VBS datastructures size :: head_t = %lu,foot_t = %lu, list_t = %lu, mempool_t = %lu\n",
           sizeof(vbs_head_t), sizeof(vbs_foot_t), sizeof(vbs_list_t), sizeof(vbs_mempool_t));


    mpool = segment_start;
    segment_start += sizeof(vbs_mempool_t);
    segment_size -= sizeof(vbs_mempool_t);

    // Create sentinels
    tmp = segment_start;
    tmp->size = OCCUP_META_SIZE;
    MARK_BLOCK_OCCUP(tmp);
    COPY_SIZE_TO_FOOTER(tmp);

    segment_start += OCCUP_META_SIZE;

    tmp = segment_start + segment_size - 2 * OCCUP_META_SIZE;
    tmp->size = OCCUP_META_SIZE;
    MARK_BLOCK_OCCUP(tmp);
    COPY_SIZE_TO_FOOTER(tmp);

    segment_size -= 2 * OCCUP_META_SIZE;

    DEBUG("Memory heap :: START = %p, END = %p, SIZE = %lu\n", segment_start, segment_start + segment_size, segment_size);

    //puts("\nBlock stats\n====================");
    for(i = 0; i < 16; ++i){
        mpool->lists[i].min_size = (i + 1) << 5;
        mpool->lists[i].max_size = (i + 1) << 5;
        mpool->lists[i].first = NULL;
        mpool->lists[i].last = NULL;
        //printf("Block %d :: min = %u, max = %u\n", i, (i + 1) << 5, (i + 1) << 5);
    }
    for(i = 16; i < 32; ++i){
        mpool->lists[i].min_size = (1 << (i - 7)) + 1;
        mpool->lists[i].max_size = 1 << (i - 6);
        mpool->lists[i].first = NULL;
        mpool->lists[i].last = NULL;
        //printf("Block %d :: min = %u, max = %u\n", i, (1 << (i - 7)) + 1, 1 << (i - 6));
    }

    // Create free blocks by subtracting as much memory as possible from the
    // remaining free space

    segment_size &= -32;    // Align remaining segment size to 32 bytes
    DEBUG("Aligned segment size = %lu\n", segment_size);

    puts("\nInitial block creation\n====================");

    vbs_mpool_create_space(segment_start, segment_size);

    vbs_mpool_print();
}

void* m_alloc(size_t block_size){
    vbs_head_t *block, *rest;
    int list_id, remaining;
    printf("\n\nm_alloc :: Trying to allocate block of size %lu (actual %lu)\n",
           block_size, ALIGN(block_size + OCCUP_META_SIZE));

    block_size = ALIGN(block_size + OCCUP_META_SIZE);
    block = vbs_mpool_find_block(block_size, &list_id);
    assert(block);

    DEBUG("Found block (*%p [%lu]) in list %d\n", (void*) block, block->size, list_id);

    vbs_list_remove_block(list_id, block);

    remaining = block->size - block_size;
    rest = (void*) block + block_size;

    block->size = block_size;
    MARK_BLOCK_OCCUP(block);
    COPY_SIZE_TO_FOOTER(block);

    if(remaining > 0)
        vbs_mpool_create_block(rest, remaining);

    vbs_mpool_print();

    return (void*) block + sizeof(size_t);
}

int m_free(void *block){
    vbs_head_t *this = block - sizeof(size_t), *prev, *next;

    assert(block);

    printf("\n\nm_free :: Trying to free a block at %p\n", block);

    prev = (vbs_head_t*) ((void*) this - sizeof(size_t));
    //DEBUG("Previous block = *%p [%lu]\n", prev, prev->size);
    if(IS_BLOCK_FREE(prev)){
        prev = HEADER_OF(prev);
        vbs_mpool_remove_block(prev);
        prev->size += BLOCK_SIZE(this);
        this = prev;
        DEBUG("Merging with previous block %p\n", prev);
    }

    next = (vbs_head_t*) NEXT_OF(this);
    //DEBUG("Next block = *%p [%lu]\n", next, next->size);
    if(IS_BLOCK_FREE(next)){
        vbs_mpool_remove_block(next);
        this->size += BLOCK_SIZE(next);
        DEBUG("Merging with next block %p\n", next);
    }

    DEBUG("Returning space to mpool :: *%p [%lu]\n", this, BLOCK_SIZE(this));
    vbs_mpool_create_space(this, BLOCK_SIZE(this));

    vbs_mpool_print();

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Auxiliary functions
////////////////////////////////////////////////////////////////////////////////

static void vbs_list_print(vbs_list_t *list){
    vbs_head_t *tmp = list->first;
    while(tmp != NULL){
        printf("(*%p, [%lu]) -> ", (void*) tmp, tmp->size);
        tmp = tmp->next;
    }
    printf("NULL\n");
}

static void vbs_mpool_print(){
    int i;
    //return;
    puts("\nMpool status:\n====================");
    for(i = 0; i < 32; ++i){
        printf("#%d :: ", i);
        vbs_list_print(&mpool->lists[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////

static vbs_head_t* vbs_mpool_find_block(int size, int *source_list){
    vbs_head_t *tmp;
    int i = 0;

    while(mpool->lists[i].max_size < size)
        ++i;

    while(i < 32){
        tmp = mpool->lists[i].first;
        while(tmp != NULL && tmp->size < size)
            tmp = tmp->next;
        if(tmp != NULL){
            *source_list = i;
            return tmp;
        }
        ++i;
    }

    return NULL;
}

static int vbs_mpool_create_block(void* location, int max_block_size){
    int i = 31;
    vbs_head_t *tmp;

    while(i >= 0 && mpool->lists[i].min_size > max_block_size)
        --i;

    assert(i >= 0);

    tmp = (vbs_head_t*) location;
    tmp->size = (max_block_size > mpool->lists[i].max_size) ?
                 mpool->lists[i].max_size : max_block_size;

    DEBUG("Creating segment of size %lu in list %d - <%x, %x>\n", tmp->size, i, tmp, NEXT_OF(tmp));

    MARK_BLOCK_FREE(tmp);
    COPY_SIZE_TO_FOOTER(tmp);

    vbs_list_insert_block(i, tmp);

    return BLOCK_SIZE(tmp);
}

static void vbs_mpool_create_space(void *location, int size){
    int tsize;
    while(size > 0){
        tsize = vbs_mpool_create_block(location, size);
        location += size;
        size -= tsize;
    }
}

static void vbs_mpool_remove_block(vbs_head_t *block){
    vbs_head_t *tmp = block;
    int i;

    while(tmp->prev != NULL)
        tmp = tmp->prev;

    for(i = 0; i < 32; ++i)
        if(mpool->lists[i].first == tmp)
            break;

    vbs_list_remove_block(i, block);
}

////////////////////////////////////////////////////////////////////////////////

static void vbs_list_insert_block(int list_id, vbs_head_t *elem){
    vbs_list_t *list = &mpool->lists[list_id];
    vbs_head_t *tmp;
    if(list_id < 16){
        elem->next = NULL;
        elem->prev = list->last;
        if(list->last != NULL)
            list->last->next = elem;
        list->last = elem;

        if(list->first == NULL)
            list->first = elem;
    } else {
        if(list->first == NULL || list->first->size >= elem->size){
            elem->next = list->first;
            elem->prev = NULL;
            list->first = elem;
            if(list->last == NULL)
                list->last = elem;
        } else {
            tmp = list->first;
            while(tmp->next != NULL && tmp->next->size < elem->size)
                tmp = tmp->next;
            elem->next = tmp->next;
            elem->prev = tmp;
            if(tmp->next == NULL)
                list->last = elem;
            tmp->next = elem;
        }
    }
}

static void vbs_list_remove_block(int list_id, vbs_head_t *elem){
    if(elem->prev == NULL)
        mpool->lists[list_id].first = elem->next;
    else
        elem->prev->next = elem->next;

    if(elem->next == NULL)
        mpool->lists[list_id].last = elem->prev;
    else
        elem->next->prev = elem->prev;
}