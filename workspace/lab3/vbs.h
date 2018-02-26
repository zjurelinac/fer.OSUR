// Dynamic memory allocation - Variable Block Sizes

#pragma once

#define LOCAL_TEST

//#include <types/basic.h>
#ifdef LOCAL_TEST
    #include <stdlib.h>
#else
    typedef unsigned int size_t;
#endif

#include <assert.h>

#ifndef _VBS_C_

int m_init(void *segment_start, size_t segment_size);
void *m_alloc(size_t block_size);
int m_free(void *block);

#else

typedef struct _vbs_head_t_{
    size_t size;
    struct _vbs_head_t_ *next, *prev;
} vbs_head_t;

typedef struct _vbs_foot_t_{
    size_t size;
} vbs_foot_t;

typedef struct _vbs_list_t_{
    size_t min_size, max_size;
    vbs_head_t *first, *last;
} vbs_list_t;

typedef struct _vbs_mempool_t_{
    vbs_list_t lists[32];
} vbs_mempool_t;

#define DEBUG(fmt, ...) printf("[DBG] %s:%d :: " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

#define BASIC_SIZE 32

#define FREE_HEADER_SIZE (sizeof(vbs_head_t))
#define FREE_FOOTER_SIZE (sizeof(vbs_foot_t))
#define FREE_META_SIZE (FREE_HEADER_SIZE + FREE_FOOTER_SIZE)
#define OCCUP_META_SIZE (2 * sizeof(size_t))

#define MARK_BLOCK_FREE(header) do { (header)->size &= -2; } while(0)
#define MARK_BLOCK_OCCUP(header) do { (header)->size |= 1; } while(0)

#define IS_BLOCK_OCCUP(header) ((header)->size & 1)
#define IS_BLOCK_FREE(header) !IS_BLOCK_OCCUP(header)

#define BLOCK_SIZE(header) ((header)->size & -2)

#define NEXT_OF(header) ((void*) (header) + BLOCK_SIZE(header))
#define FOOTER_OF(header) (NEXT_OF(header) - sizeof(vbs_foot_t))
#define HEADER_OF(footer) ((void*) (footer) - BLOCK_SIZE(footer) + sizeof(vbs_foot_t))

#define COPY_SIZE_TO_FOOTER(header) do { ((vbs_foot_t*) FOOTER_OF(header))->size = (header)->size; } while(0)

#define ALIGN(size) (((size) + BASIC_SIZE - 1) & ~(BASIC_SIZE - 1))

#endif