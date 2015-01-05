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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "page_pool.h"

/* Had better not to change there */
#define PAGE_PER_BLOCK (32)
#define BIT_PER_BITMAP (8)
#define BLOCK_BITMAP_SIZE (PAGE_PER_BLOCK / BIT_PER_BITMAP)

#define BLOCK_DATA_SIZE(page_size) ((page_size) * PAGE_PER_BLOCK)
#define POOL_BLOCK_COUNT(size, page_size) (((size) / BLOCK_DATA_SIZE(page_size)) + (((size) % BLOCK_DATA_SIZE(page_size) != 0) ? 1 : 0))

/* Block header for rapid locating the record in pool */
struct paged_mem_block_header
{
    uint32_t block_idx;
    uint32_t bitmap_idx;
    uint32_t bit_idx;
    uint32_t in_pool;
    void *ptr; /* The position of the real allocated block */
};
typedef struct paged_mem_block_header paged_mem_block_header_t;
#define BLOCK_HEADER_SIZE 20

static void paged_mem_block_header_write(void *ptr_header, \
        uint32_t block_idx, uint32_t bitmap_idx, uint32_t bit_idx, \
        uint32_t in_pool)
{
    uint32_t *ptr_header_p = ptr_header;

    /* Signature */
    *ptr_header_p++ = 0xDEADBEEF;
    /* Block Index */
    *ptr_header_p++ = block_idx;
    /* Bitmap Index & Bit Index */
    *ptr_header_p++ = ((bitmap_idx & 0xFFFF) << 16) | (bit_idx & 0xFFFF);
    /* Bit Index */
    *ptr_header_p++ = in_pool;
}

#define PAGED_MEM_BLOCK_HEADER_NOT_IN_POOL 0
#define PAGED_MEM_BLOCK_HEADER_IN_POOL 1

#define PAGED_MEM_BLOCK_HEADER_READ_OK 0
#define PAGED_MEM_BLOCK_HEADER_READ_INVALID_SIGNATURE 1

static int paged_mem_block_header_read(void *ptr_header, paged_mem_block_header_t *block_header)
{
    uint32_t *ptr_header_p = ptr_header;

    /* Check Signature */
    if (*ptr_header_p != 0xDEADBEEF) return PAGED_MEM_BLOCK_HEADER_READ_INVALID_SIGNATURE;
    /* Skip Signature */
    ptr_header_p++;
    /* Block Index */
    block_header->block_idx = *ptr_header_p++;
    /* Bitmap Index & Bit Index */
    block_header->bitmap_idx = (*ptr_header_p >> 16);
    block_header->bit_idx= (*ptr_header_p & 0xFFFF);
    ptr_header_p++;
    /* In pool */
    block_header->in_pool = *ptr_header_p++;

    return PAGED_MEM_BLOCK_HEADER_READ_OK;
}

/* Create a new Memory Block */
static int paged_mem_block_init(paged_mem_block_t *block, size_t page_size, int fill_with_zero)
{
    if (block == NULL) return -1;

    block->page_count = PAGE_PER_BLOCK;
    block->data = NULL;
    block->bitmap = NULL;

    block->data = (unsigned char *)malloc(sizeof(unsigned char) * BLOCK_DATA_SIZE(page_size));
    if (block->data == NULL) goto fail;
    if (fill_with_zero) memset(block->data, 0, BLOCK_DATA_SIZE(page_size));
    block->bitmap_size = BLOCK_BITMAP_SIZE;
    block->bitmap = (unsigned char *)malloc(sizeof(unsigned char) * BLOCK_BITMAP_SIZE);
    if (block->bitmap == NULL) goto fail;
    memset(block->bitmap, 0, BLOCK_BITMAP_SIZE);

    block->size_used = 0;
    block->size_free = BLOCK_DATA_SIZE(page_size);
    block->size_total = BLOCK_DATA_SIZE(page_size);
    return 0;
fail:
    if (block->bitmap != NULL) { free(block->bitmap); block->bitmap = NULL; }
    if (block->data != NULL) { free(block->data); block->data = NULL; }
    return -1;
}

static int paged_mem_block_uninit(paged_mem_block_t *block)
{
    if (block->bitmap != NULL) free(block->bitmap);
    if (block->data != NULL) free(block->data);
    return 0;
}

/* Create a new Memory Pool  */
static paged_mem_pool_t *paged_mem_pool_new_raw(size_t size, size_t page_size, int fallback, int fill_with_zero)
{
    size_t idx = 0;
    paged_mem_pool_t *pool = NULL;
    pool = (paged_mem_pool_t *)malloc(sizeof(paged_mem_pool_t));
    if (pool == NULL) goto fail;
	pool->blocks = NULL;
	pool->page_size = page_size;
	pool->fallback = fallback;
    pool->block_count = POOL_BLOCK_COUNT(size, page_size);
	pool->size_used = 0;
	size = pool->block_count * BLOCK_DATA_SIZE(page_size);
	pool->size_free = size;
	pool->size_total = size;
    pool->blocks = (paged_mem_block_t *)malloc(sizeof(paged_mem_block_t) * (pool->block_count));
    if (pool->blocks == NULL) goto fail;
    for (idx = 0; idx < pool->block_count; idx++)
    {
        pool->blocks[idx].bitmap = NULL;
        pool->blocks[idx].data = NULL;
    }
    for (idx = 0; idx < pool->block_count; idx++)
        if (paged_mem_block_init(&pool->blocks[idx], page_size, fill_with_zero) != 0) goto fail;
    return pool;
fail:
    if (pool != NULL)
    {
        if (idx != 0)
        {
            if (pool->blocks != NULL)
            {
                while (idx > 0)
                {
                    paged_mem_block_uninit(&pool->blocks[idx]);
                    idx--;
                }
                free(pool->blocks);
            }
        }
        free(pool);
    }
    return NULL;
}

#define PAGED_POOL_FALLBACK_ON 1
/*#define PAGED_POOL_FALLBACK_OFF 0*/

paged_mem_pool_t *paged_mem_pool_new(size_t size, int fill_with_zero)
{
    return paged_mem_pool_new_raw(size, PAGE_SIZE_DEFAULT, PAGED_POOL_FALLBACK_ON, fill_with_zero);
}

paged_mem_pool_t *paged_mem_pool_new_64b(size_t size, int fill_with_zero)
{
    return paged_mem_pool_new_raw(size, 64, PAGED_POOL_FALLBACK_ON, fill_with_zero);
}

paged_mem_pool_t *paged_mem_pool_new_128b(size_t size, int fill_with_zero)
{
    return paged_mem_pool_new_raw(size, 128, PAGED_POOL_FALLBACK_ON, fill_with_zero);
}

int paged_mem_pool_destroy(paged_mem_pool_t *pool)
{
    size_t idx;
    for (idx = 0; idx < pool->block_count; idx++)
    {
        paged_mem_block_uninit(&pool->blocks[idx]);
    }
    free(pool->blocks);
    free(pool);
    return 0;
}

void *paged_mem_pool_malloc(paged_mem_pool_t *pool, size_t size)
{
    size_t block_idx, page_idx, bitmap_idx;
    unsigned int bit_idx;
    void *p = NULL;
	size_t fall_back_size;

    if (size + BLOCK_HEADER_SIZE > pool->page_size)
    {
        /* Larger than page size */
        if (pool->fallback != 0) 
        {
			fall_back_size = size + BLOCK_HEADER_SIZE;
			if (fall_back_size > FALL_BACK_SIZE_LIMIT)
			{
				return NULL;
			}
            /* Allocate Memory */
			p = malloc(fall_back_size);
            if (p == NULL) return NULL;
            /* Write Header */
            paged_mem_block_header_write(p, 0, 0, 0, PAGED_MEM_BLOCK_HEADER_NOT_IN_POOL);
            /* Return the body part */
            return (char *)p + BLOCK_HEADER_SIZE;
        }
        else
        { return NULL; }
    }
    for (block_idx = 0; block_idx < pool->block_count; block_idx++)
    {
        for (page_idx = 0; page_idx < pool->blocks[block_idx].page_count && pool->blocks[block_idx].size_free >= pool->page_size; page_idx++)
        {
            for (bitmap_idx = 0; bitmap_idx < pool->blocks[block_idx].bitmap_size; bitmap_idx++)
            {
                if (pool->blocks[block_idx].bitmap[bitmap_idx] != 0xFF)
                {
                    bit_idx = (unsigned int)((pool->blocks[block_idx].bitmap[bitmap_idx] ^ (pool->blocks[block_idx].bitmap[bitmap_idx] + 1)) >> 1);
                    bit_idx = (bit_idx & 0x55) + ((bit_idx>>1)&0x55);
                    bit_idx = (bit_idx & 0x33) + ((bit_idx>>2)&0x33);
                    bit_idx = (bit_idx & 0xf) + ((bit_idx>>4)&0xf);
                    
                    p = pool->blocks[block_idx].data + (bitmap_idx * BIT_PER_BITMAP + bit_idx) * pool->page_size;

                    /* Write Header */
                    paged_mem_block_header_write(p, (uint32_t)block_idx, (uint32_t)bitmap_idx, bit_idx, PAGED_MEM_BLOCK_HEADER_IN_POOL);

                    /* Mark bitmap */
                    /* For preventing '-Wconversion'
                     * pool->blocks[block_idx].bitmap[bitmap_idx] |= (1 << (bit_idx)); */
                    pool->blocks[block_idx].bitmap[bitmap_idx] |= (unsigned char)(1 << (bit_idx));
                    pool->blocks[block_idx].size_used += pool->page_size;
                    pool->blocks[block_idx].size_free -= pool->page_size;
                    pool->size_used += pool->page_size;
                    pool->size_free -= pool->page_size;

                    /* Return body */
                    return (char *)p + BLOCK_HEADER_SIZE;
                }
            }
        }
    }
    /* No enough space in pool */
    if (pool->fallback != 0) 
    {
        /* Allocate Memory */
		fall_back_size = size + BLOCK_HEADER_SIZE;
		if (fall_back_size > FALL_BACK_SIZE_LIMIT)
		{
			return NULL;
		}
		p = malloc(fall_back_size);
        if (p == NULL) return NULL;
        /* Write Header */
        paged_mem_block_header_write(p, 0, 0, 0, PAGED_MEM_BLOCK_HEADER_NOT_IN_POOL);
        /* Return the body part */
        return (char *)p + BLOCK_HEADER_SIZE;
    }
    else { return NULL; }
}

int paged_mem_pool_free(paged_mem_pool_t *pool, void *ptr)
{
    paged_mem_block_header_t block_header;
    size_t block_idx, bitmap_idx, bit_idx;

    /* Read Block Header */
    if (paged_mem_block_header_read((char *)ptr - BLOCK_HEADER_SIZE, &block_header) != 0)
    {
        return -1; 
    }
    else
    {
        if (block_header.in_pool != 0)
        {
            /* In pool */ 
			block_idx = (size_t)block_header.block_idx;
            bitmap_idx = (size_t)block_header.bitmap_idx;
			bit_idx = (size_t)block_header.bit_idx;

            /* For preventing '-Wconversion'
             * pool->blocks[block_idx].bitmap[bitmap_idx] &= ~((unsigned char)((1 << (bit_idx)) & 0xFF));*/
            pool->blocks[block_idx].bitmap[bitmap_idx] = (unsigned char) \
                                                         (pool->blocks[block_idx].bitmap[bitmap_idx] & \
                                                          ~((unsigned char)((1 << (bit_idx)) & 0xFF)));
            pool->blocks[block_idx].size_used -= pool->page_size;
            pool->blocks[block_idx].size_free += pool->page_size;
            pool->size_used -= pool->page_size;
            pool->size_free += pool->page_size;

            return 0;
        }
        else
        {
            /* Not in pool */
            free((char *)ptr - BLOCK_HEADER_SIZE);
            return 0;
        }
    }
}


