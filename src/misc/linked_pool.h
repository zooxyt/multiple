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

#ifndef _LINKED_POOL_H_
#define _LINKED_POOL_H_

#include <stdint.h>

#include "spinlock.h"


#define PAGE_SIZE_DEFAULT (4096)

/* Clang */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wpadded"
#endif

/* The format of a block of memory 
 * sop = sizeof(void *)
 * -------------------
 * ptr_to_pool : sop
 * ptr_to_page : sop
 * begin
 * content (block_size - sop * 2)
 * end
 * -------------------
 */


/* Chain */

struct linked_mem_pool_chain_node
{
    void *content;

    struct linked_mem_pool_chain_node *next;
};

struct linked_mem_pool_chain
{
    struct linked_mem_pool_chain_node *begin;
    struct linked_mem_pool_chain_node *end;
    size_t size;
};


/* Core */

struct linked_mem_pool_core
{
    void *core;
    void *chain;
    struct linked_mem_pool_core *next;
};

struct linked_mem_pool_core_list
{
    struct linked_mem_pool_core *begin;
    struct linked_mem_pool_core *end;
};


/* Memory Pool data structure */
struct linked_mem_pool
{
    struct linked_mem_pool_core_list *cores;

    struct linked_mem_pool_chain *chain;

    size_t block_size;
    size_t block_count;

    /* In Pool */
    size_t size_used;
    size_t size_free;
    size_t size_total;

    /* Not in Pool */
    size_t size_used_extra;

    /* Memory status when lack called */
    size_t size_used_prev;

	/* allocate from malloc when no appropriate block */
	int fallback;

    /* Lock */
    mutex_t lock;
};

typedef struct linked_mem_block linked_mem_block_t;
typedef struct linked_mem_pool linked_mem_pool_t;

/* Default (4K, Fall back) */
linked_mem_pool_t *linked_mem_pool_new(size_t size, int fill_with_zero);
/* 64 Bytes */
linked_mem_pool_t *linked_mem_pool_new_64b(size_t size, int fill_with_zero);
/* 128 Bytes */
linked_mem_pool_t *linked_mem_pool_new_128b(size_t size, int fill_with_zero);

int linked_mem_pool_destroy(linked_mem_pool_t *pool);

/* Memory Allocation and Unallocation */
void *linked_mem_pool_malloc(linked_mem_pool_t *pool, size_t size);
int linked_mem_pool_free(linked_mem_pool_t *pool, void *ptr);

/* GC Interface */
int linked_mem_pool_lack(linked_mem_pool_t *pool);
int linked_mem_pool_feedback(linked_mem_pool_t *pool);

#endif

