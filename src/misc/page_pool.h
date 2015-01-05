/*
   Paged Memory Pool
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

#ifndef _PAGE_POOL_H_
#define _PAGE_POOL_H_

#include <stdio.h>

#define PAGE_SIZE_DEFAULT (4096)
#define FALL_BACK_SIZE_LIMIT (1024*1014*100) /* 100 MB */

/* Clang */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wpadded"
#endif

/* A block contains PAGE_SIZE * page_count */
struct paged_mem_block
{
    unsigned char *data; /* data area, contains 'page_count' pages of memory */
    unsigned char *bitmap; /* bitmap to data area, takes (page_count / 8) bytes */
    size_t bitmap_size; /* size of space bitmap takes */
    size_t page_count; /* number of page in this block */

    size_t size_used;
    size_t size_free;
    size_t size_total;
};

/* Memory Pool data structure */
struct paged_mem_pool
{
    struct paged_mem_block *blocks;
    size_t block_count;
    size_t page_size;
    int fallback; /* allocate from C Standard Library when no appropriate block */

    size_t size_used;
    size_t size_free;
    size_t size_total;
};

typedef struct paged_mem_block paged_mem_block_t;
typedef struct paged_mem_pool paged_mem_pool_t;

/* Default (4K, Fall back) */
paged_mem_pool_t *paged_mem_pool_new(size_t size, int fill_with_zero);
/* 64 Bytes */
paged_mem_pool_t *paged_mem_pool_new_64b(size_t size, int fill_with_zero);
/* 128 Bytes */
paged_mem_pool_t *paged_mem_pool_new_128b(size_t size, int fill_with_zero);

int paged_mem_pool_destroy(paged_mem_pool_t *pool);

void *paged_mem_pool_malloc(paged_mem_pool_t *pool, size_t size);
int paged_mem_pool_free(paged_mem_pool_t *pool, void *ptr);

#endif

