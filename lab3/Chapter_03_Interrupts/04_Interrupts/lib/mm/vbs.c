#define _VBS_C_
#include <lib/vbs.h>
#include <kernel/kprint.h>
#include <kernel/errno.h>

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

void *m_init(void *segment_start, size_t segment_size){
    int i;
    vbs_head_t *tmp;

    assert(segment_start && segment_size > sizeof(vbs_mempool_t) + 2 * OCCUP_META_SIZE);

    //LOG("INFO", "VBS datastructures size :: head_t = %u,foot_t = %u, list_t = %u, mempool_t = %u\n",
    //     sizeof(vbs_head_t), sizeof(vbs_foot_t), sizeof(vbs_list_t), sizeof(vbs_mempool_t));


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

    //LOG("INFO", "Memory heap :: START = %x, END = %x, SIZE = %u", segment_start, segment_start + segment_size, segment_size);

    for(i = 0; i < 16; ++i){
        mpool->lists[i].min_size = (i + 1) << 5;
        mpool->lists[i].max_size = (i + 1) << 5;
        mpool->lists[i].first = NULL;
        mpool->lists[i].last = NULL;
    }
    for(i = 16; i < 32; ++i){
        mpool->lists[i].min_size = (1 << (i - 7)) + 1;
        mpool->lists[i].max_size = 1 << (i - 6);
        mpool->lists[i].first = NULL;
        mpool->lists[i].last = NULL;
    }

    // Create free blocks by subtracting as much memory as possible from the
    // remaining free space

    segment_size &= -32;    // Align remaining segment size to 32 bytes
    //LOG("INFO", "Aligned segment size = %u", segment_size);

    //kprintf("\nInitial block creation\n====================");

    vbs_mpool_create_space(segment_start, segment_size);

    //vbs_mpool_print();
    return mpool;
}

void* m_alloc(size_t block_size){
    vbs_head_t *block, *rest;
    char *ptr;
    int list_id, remaining, i;
    //kprintf("\n\nm_alloc :: Trying to allocate block of size %u (actual %u)\n",
    //       block_size, ALIGN(block_size + OCCUP_META_SIZE));

    block_size = ALIGN(block_size + OCCUP_META_SIZE);
    block = vbs_mpool_find_block(block_size, &list_id);
    assert(block);

    LOG(INFO, "Found block (*%x [%u]) in list %d", (void*) block, block->size, list_id);

    vbs_list_remove_block(list_id, block);

    remaining = block->size - block_size;
    rest = (void*) block + block_size;

    block->size = block_size;
    MARK_BLOCK_OCCUP(block);
    COPY_SIZE_TO_FOOTER(block);

    if(remaining > 0)
        vbs_mpool_create_block(rest, remaining);

    ptr = (void*) block + sizeof(size_t);
    for(i = 0; i < block_size - OCCUP_META_SIZE; ++i)
        *(ptr++) = 0;

    vbs_mpool_print();
    kprintf("m_alloc() :: allocated block of size %u at %x\n", block_size, (void*) block);

    return (void*) block + sizeof(size_t);
}

int m_free(void *block){
    vbs_head_t *this = block - sizeof(size_t), *prev, *next;

    assert(block);

    kprintf("m_free() :: freeing a block of size %u at %x\n", BLOCK_SIZE(this), this);
    //kprintf("\n\nm_free :: Trying to free a block at %x\n", block);

    prev = (vbs_head_t*) ((void*) this - sizeof(size_t));
    if(IS_BLOCK_FREE(prev)){
        prev = HEADER_OF(prev);
        vbs_mpool_remove_block(prev);
        prev->size += BLOCK_SIZE(this);
        this = prev;
        //LOG(INFO, "Merging with previous block %x", prev);
    }

    next = (vbs_head_t*) NEXT_OF(this);
    if(IS_BLOCK_FREE(next)){
        vbs_mpool_remove_block(next);
        this->size += BLOCK_SIZE(next);
        //LOG(INFO, "Merging with next block %x", next);
    }

    LOG(INFO, "Returning space to mpool :: *%x [%u]", this, BLOCK_SIZE(this));
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
        kprintf("(*%x, [%u]) -> ", (void*) tmp, tmp->size);
        tmp = tmp->next;
    }
    kprintf("NULL\n");
}

static void vbs_mpool_print(){
    int i;
    return;
    kprintf("\nMpool status:\n====================");
    for(i = 0; i < 32; ++i){
        kprintf("#%d :: ", i);
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

    //LOG("INFO", "Creating segment of size %u in list %d - <%x, %x>\n", tmp->size, i, tmp, NEXT_OF(tmp));

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