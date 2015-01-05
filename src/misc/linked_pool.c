/*
   Linked Memory Pool
   Copyright (c) 2013-2014 Cheryl Natsu 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
/* Conversion from 'void *' to 'uint64_t' is sign-extended */
#pragma warning(disable:4826)
/* 'type cast' : truncation from 'uint64_t' to 'void *' */
#pragma warning(disable:4305)
/* Potential comparison of a constant with another constant */
#pragma warning(disable:6326)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linked_pool.h"
#include "spinlock.h"

#define UPPER_BOUND_32BIT 0xFFFFFFFFUL
#define MASK_32BIT UPPER_BOUND_32BIT
#define POOL_BLOCK_COUNT(size, page_size) (((size) / (page_size)) + (((size) % (page_size) != 0) ? 1 : 0))
#define SOP (sizeof(void*))
#define BLOCK_HEADER_SIZE (SOP*2)

/* Declarations */
static struct linked_mem_pool_chain *linked_mem_pool_chain_new(void);
static int linked_mem_pool_chain_destroy(struct linked_mem_pool_chain *chain);

static int linked_mem_pool_chain_append(struct linked_mem_pool_chain *chain, \
        struct linked_mem_pool_chain_node *new_node);
static struct linked_mem_pool_chain_node *linked_mem_pool_chain_pick( \
        struct linked_mem_pool_chain *chain);

static struct linked_mem_pool_core *linked_mem_pool_core_new(void *core, void *chain);
static int linked_mem_pool_core_destroy(struct linked_mem_pool_core *core);
static struct linked_mem_pool_core_list *linked_mem_pool_core_list_new(void);
static int linked_mem_pool_core_list_destroy(struct linked_mem_pool_core_list *list);
static int linked_mem_pool_core_list_append(struct linked_mem_pool_core_list *list, \
        struct linked_mem_pool_core *new_core);


/* Chain */

static struct linked_mem_pool_chain *linked_mem_pool_chain_new(void)
{
    struct linked_mem_pool_chain *new_chain = NULL;

    new_chain = (struct linked_mem_pool_chain *)malloc(sizeof(struct linked_mem_pool_chain));
    if (new_chain == NULL) { goto fail; }
    new_chain->begin = NULL;
    new_chain->end = NULL;
    new_chain->size = 0;

fail:
    return new_chain;
}

static int linked_mem_pool_chain_destroy(struct linked_mem_pool_chain *chain)
{
    free(chain);

    return 0;
}

static int linked_mem_pool_chain_append(struct linked_mem_pool_chain *chain, \
        struct linked_mem_pool_chain_node *new_node)
{
    new_node->next = NULL;

    if (chain->begin == NULL)
    {
        chain->begin = chain->end = new_node;
    }
    else
    {
        chain->end->next = new_node;
        chain->end = new_node;
    }
    chain->size += 1;

    return 0;
}

static struct linked_mem_pool_chain_node *linked_mem_pool_chain_pick( \
        struct linked_mem_pool_chain *chain)
{
    struct linked_mem_pool_chain_node *node;

    if (chain->begin == NULL)
    {
        return NULL;
    }
    else if (chain->begin == chain->end)
    {
        node = chain->begin;
        chain->begin = chain->end = NULL;
        chain->size -= 1;
        return node;
    }
    else
    {
        node = chain->begin;
        chain->begin = chain->begin->next;
        chain->size -= 1;
        return node;
    }
}


/* Core */

static struct linked_mem_pool_core *linked_mem_pool_core_new(void *core, void *chain)
{
    struct linked_mem_pool_core *new_linked_mem_pool_core = NULL;

    new_linked_mem_pool_core = (struct linked_mem_pool_core *)malloc(sizeof(struct linked_mem_pool_core));
    if (new_linked_mem_pool_core == NULL) { goto fail; } 
    new_linked_mem_pool_core->core = core;
    new_linked_mem_pool_core->chain = chain;
    new_linked_mem_pool_core->next = NULL;

fail:
    return new_linked_mem_pool_core;
}

static int linked_mem_pool_core_destroy(struct linked_mem_pool_core *core)
{
    free(core->core);
    free(core->chain);
    free(core);

    return 0;
}


static struct linked_mem_pool_core_list *linked_mem_pool_core_list_new(void)
{
    struct linked_mem_pool_core_list *new_linked_mem_pool_core_list = NULL;

    new_linked_mem_pool_core_list = (struct linked_mem_pool_core_list *)malloc(sizeof(struct linked_mem_pool_core_list));
    if (new_linked_mem_pool_core_list == NULL) { goto fail; }

    new_linked_mem_pool_core_list->begin = new_linked_mem_pool_core_list->end = NULL;

fail:
    return new_linked_mem_pool_core_list;
}

static int linked_mem_pool_core_list_destroy(struct linked_mem_pool_core_list *list)
{
    struct linked_mem_pool_core *linked_mem_pool_core_cur, *linked_mem_pool_core_next; 

    linked_mem_pool_core_cur = list->begin;
    while (linked_mem_pool_core_cur != NULL)
    {
        linked_mem_pool_core_next = linked_mem_pool_core_cur->next;
        linked_mem_pool_core_destroy(linked_mem_pool_core_cur);
        linked_mem_pool_core_cur = linked_mem_pool_core_next;
    }
    free(list);

    return 0;
}

static int linked_mem_pool_core_list_append(struct linked_mem_pool_core_list *list, \
        struct linked_mem_pool_core *new_core)
{
    if (list->begin == NULL)
    {
        list->begin = list->end = new_core;
    }
    else
    {
        list->end->next = new_core;
        list->end = new_core;
    }

    return 0;
}

static int linked_mem_pool_generate_new_core(struct linked_mem_pool_core_list *cores, \
        struct linked_mem_pool_chain *chain, size_t core_size, size_t page_size)
{
    int ret = 0;
    size_t chain_node_count = POOL_BLOCK_COUNT(core_size, page_size);
    size_t idx;
    void *new_core = NULL;
    void *new_chain = NULL;
    char *content_p;
    struct linked_mem_pool_core *new_linked_mem_pool_core = NULL;
    struct linked_mem_pool_chain_node *linked_mem_pool_chain_node_cur = NULL;
    struct linked_mem_pool_chain_node *linked_mem_pool_chain_node_prev = NULL;

    /* Create real part of core */
    new_core = (void *)malloc(core_size);
    if (new_core == NULL) { ret = -1; goto fail; }
    /* Create real part of chain */
    new_chain = (void *)malloc(sizeof(struct linked_mem_pool_chain_node) * chain_node_count);
    if (new_chain == NULL) { ret = -1; goto fail; }

    /* Fill chain */
    content_p = new_core;
    linked_mem_pool_chain_node_cur = new_chain;
    for (idx = 0; idx != chain_node_count; idx++)
    {
        /* Connect prev to cur */
        if (linked_mem_pool_chain_node_prev != NULL)
        { linked_mem_pool_chain_node_prev->next = linked_mem_pool_chain_node_cur; }

        /* Fill node */
        linked_mem_pool_chain_node_cur->content = content_p;
        content_p += page_size;

        /* Added to global chain */
        linked_mem_pool_chain_append(chain, linked_mem_pool_chain_node_cur);

        /* Next node */
        linked_mem_pool_chain_node_prev = linked_mem_pool_chain_node_cur;
        linked_mem_pool_chain_node_cur++;
    }

    new_linked_mem_pool_core = linked_mem_pool_core_new(new_core, new_chain);
    if (new_linked_mem_pool_core == NULL) { ret = -1; goto fail; }
    new_core = NULL;
    new_chain = NULL;

    linked_mem_pool_core_list_append(cores, new_linked_mem_pool_core);
    new_linked_mem_pool_core = NULL;

    goto done;
fail:
    if (new_core != NULL) { free(new_core); }
    if (new_chain != NULL) { free(new_chain); }
    if (new_linked_mem_pool_core != NULL) { linked_mem_pool_core_destroy(new_linked_mem_pool_core); }
done:
    return ret;
}


/* Create a new Memory Pool  */
static linked_mem_pool_t *linked_mem_pool_new_raw(size_t size, size_t page_size, int fallback, int fill_with_zero)
{
    linked_mem_pool_t *pool = NULL;

    (void)fill_with_zero;

    pool = (linked_mem_pool_t *)malloc(sizeof(linked_mem_pool_t));
    if (pool == NULL) { goto fail; }

    thread_mutex_init(&pool->lock);

    pool->cores = NULL;
    pool->chain = NULL;
    pool->cores = linked_mem_pool_core_list_new();
    if (pool->cores == NULL) { goto fail; }
    pool->chain = linked_mem_pool_chain_new();
    if (pool->chain == NULL) { goto fail; }

	pool->block_size = page_size;
    pool->block_count = POOL_BLOCK_COUNT(size, page_size);
	pool->fallback = fallback;
	pool->size_used = 0;
	size = pool->block_count * (page_size);
	pool->size_free = size;
	pool->size_total = size;
    
    pool->size_used_extra = 0;
    pool->size_used_prev = 0;

    /* Initialize core */
    if (linked_mem_pool_generate_new_core( \
                pool->cores, \
                pool->chain, \
                size, page_size) != 0)
    { goto fail; }

    goto done;
fail:
    if (pool != NULL)
    {
        if (pool->chain != NULL) linked_mem_pool_chain_destroy(pool->chain);
        if (pool->cores != NULL) linked_mem_pool_core_list_destroy(pool->cores);
        thread_mutex_uninit(&pool->lock);
        free(pool);
        pool = NULL;
    }
done:
    return pool;
}

#define LINKED_POOL_FALLBACK_ON 1

linked_mem_pool_t *linked_mem_pool_new(size_t size, int fill_with_zero)
{
    return linked_mem_pool_new_raw(size, PAGE_SIZE_DEFAULT, LINKED_POOL_FALLBACK_ON, fill_with_zero);
}

linked_mem_pool_t *linked_mem_pool_new_64b(size_t size, int fill_with_zero)
{
    return linked_mem_pool_new_raw(size, 64, LINKED_POOL_FALLBACK_ON, fill_with_zero);
}

linked_mem_pool_t *linked_mem_pool_new_128b(size_t size, int fill_with_zero)
{
    return linked_mem_pool_new_raw(size, 128, LINKED_POOL_FALLBACK_ON, fill_with_zero);
}

int linked_mem_pool_destroy(linked_mem_pool_t *pool)
{
    linked_mem_pool_chain_destroy(pool->chain);
    linked_mem_pool_core_list_destroy(pool->cores);
    thread_mutex_uninit(&pool->lock);
    free(pool);

    return 0;
}

static void write_pointer(void *dst, void *ptr_to_write)
{
    uint32_t pu32;
    uint64_t pu64;

    switch (SOP)
    {
        case 4:
			pu32 = (uint32_t)((uint64_t)ptr_to_write & MASK_32BIT);
            *((uint32_t*)dst) = pu32;
            break;
        case 8:
            pu64 = (uint64_t)ptr_to_write;
            *((uint64_t*)dst) = pu64;
            break;
    }
}

static void *read_pointer(void *src)
{
    switch (SOP)
    {
        case 4:
            return (void *)((uint64_t)(*((uint32_t *)src)));
        case 8:
            return (void *)(*((uint64_t *)src));
		default:
			return NULL;
    }
}

static int linked_mem_pool_extend(linked_mem_pool_t *pool)
{
    /* Append two new cores */

    /* 1st */
    if (linked_mem_pool_generate_new_core( \
                pool->cores, \
                pool->chain, \
                pool->block_size * pool->block_count, pool->block_size) != 0)
    { return -1; }
    pool->size_total += pool->block_size * pool->block_count;
    pool->size_free += pool->block_size * pool->block_count;
    /* 2nd */
    if (linked_mem_pool_generate_new_core( \
                pool->cores, \
                pool->chain, \
                pool->block_size * pool->block_count, pool->block_size) != 0)
    { return -1; }
    pool->size_total += pool->block_size * pool->block_count;
    pool->size_free += pool->block_size * pool->block_count;

    return 0;
}

void *linked_mem_pool_malloc(linked_mem_pool_t *pool, size_t size)
{
    char *p = NULL;
    struct linked_mem_pool_chain_node *chain_node = NULL;
    void *p_null = NULL, *p_result = NULL;

    thread_mutex_lock(&pool->lock);

    if (size + BLOCK_HEADER_SIZE > pool->block_size)
    {
        /* Larger than page size */
        if (pool->fallback != 0) 
        {
            /* Allocate Memory */
            p = malloc(size + BLOCK_HEADER_SIZE); 
            if (p == NULL) 
            {
                p_result = NULL;
                goto finish;
            }
            /* Write Header */
            write_pointer(p, p_null); /* pool */
            write_pointer(p + SOP, p_null); /* page */
            /* Update usage */
            pool->size_used_extra += size;
            /* Return the body part */
            p_result = (char *)p + BLOCK_HEADER_SIZE;
            goto finish;
        }
        else
        {
            p_result = NULL;
            goto finish;
        }
    }
    else
    {
        chain_node = linked_mem_pool_chain_pick(pool->chain);
        if (chain_node == NULL) 
        {
            /* No enough space in pool */
            if (pool->fallback != 0) 
            {
                /* Allocate Memory */
                if (linked_mem_pool_extend(pool) != 0)
                {
                    p_result = NULL;
                    goto finish;
                }

                /* Pick a block */
                chain_node = linked_mem_pool_chain_pick(pool->chain);
                p = chain_node->content;
                write_pointer(p, pool); /* pool */
                write_pointer(p + SOP, chain_node); /* page */
                /* Update usage */
                pool->size_used += pool->block_size;
                pool->size_free -= pool->block_size;
                p_result = (char *)p + BLOCK_HEADER_SIZE;
                goto finish;
            }
            else
            {
                p_result = NULL;
                goto finish;
            }
        }
        else
        {
            p = chain_node->content;
            write_pointer(p, pool); /* pool */
            write_pointer(p + SOP, chain_node); /* page */
            /* Update usage */
            pool->size_used += pool->block_size;
            pool->size_free -= pool->block_size;
            p_result = (char *)p + BLOCK_HEADER_SIZE;
            goto finish;
        }
    }

finish:
    thread_mutex_unlock(&pool->lock);
    return p_result;
}

int linked_mem_pool_free(linked_mem_pool_t *pool, void *ptr)
{
    void *ptr_block;
    struct linked_mem_pool *ptr_pool;
    struct linked_mem_pool_chain_node *ptr_page;

    thread_mutex_lock(&pool->lock);

    ptr_block = (char *)ptr - BLOCK_HEADER_SIZE;
    ptr_pool = read_pointer(ptr_block);
    ptr_page = read_pointer((char *)ptr_block + SOP);

    if (ptr_pool == NULL)
    {
        free(ptr_block);
    }
    else
    {
        linked_mem_pool_chain_append(ptr_pool->chain, ptr_page);
        pool->size_used -= pool->block_size;
        pool->size_free += pool->block_size;
    }

    thread_mutex_unlock(&pool->lock);

    return 0;
}

/* GC Process 
 * 1. Is Lack? If yes, record memory status 
 * 2. Launch GC
 * 3. Feedback, If collected 25%-, extend the pool */

int linked_mem_pool_lack(linked_mem_pool_t *pool)
{
    int lack = 0;
    
    thread_mutex_lock(&pool->lock);

    /*
    printf("\n(used:%u, total:%u, extra:%u)\n", \
            (unsigned int)pool->size_used, \
            (unsigned int)pool->size_total, \
            (unsigned int)pool->size_used_extra);
            */

    /* If used space larger than 75% of total space */
    lack = (pool->size_used > \
            ((pool->size_total >> 2) + (pool->size_total >> 1))) ? 1 : 0;

    if (lack == 0)
    {
        /* If extra used space larger than 8 times of total space */
        if (pool->size_used_extra > (pool->size_total << 3))
        {
            lack = 1;
        }
    }

    if (lack != 0) pool->size_used_prev = pool->size_used;

    thread_mutex_unlock(&pool->lock);

    return lack;
}

int linked_mem_pool_feedback(linked_mem_pool_t *pool)
{
    thread_mutex_lock(&pool->lock);
    if (pool->size_used_prev - pool->size_used < pool->size_total >> 2)
    {
        linked_mem_pool_extend(pool);
    }
    thread_mutex_unlock(&pool->lock);

    return 0;
}

