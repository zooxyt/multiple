/* Virtual Machine : Resource Management : Page Pool
 * Copyright(C) 2013-2014 Cheryl Natsu

 * This file is part of multiple - Multiple Paradigm Language Emulator

 * multiple is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * multiple is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#include <stdlib.h>
#include <stdio.h>

#include "page_pool.h"

#include "vm_res_pool_page.h"

/* Initialize and release pool */
void *virtual_machine_resource_pool_page_init(size_t size)
{
    return paged_mem_pool_new(size, 1);
}

void *virtual_machine_resource_pool_page_64b_init(size_t size)
{
    return paged_mem_pool_new_64b(size, 1);
}

void *virtual_machine_resource_pool_page_128b_init(size_t size)
{
    return paged_mem_pool_new_128b(size, 1);
}

int virtual_machine_resource_pool_page_uninit(void *pool_ptr)
{
    return paged_mem_pool_destroy(pool_ptr);
}

/* Allocate and deallocate memory */
void *virtual_machine_resource_pool_page_malloc(void *pool_ptr, size_t size)
{
    return paged_mem_pool_malloc(pool_ptr, size);
}

int virtual_machine_resource_pool_page_free(void *pool_ptr, void *ptr)
{
    return paged_mem_pool_free(pool_ptr, ptr);
}

/* Lack */
int virtual_machine_resource_pool_page_lack(void *pool_ptr)
{
    struct paged_mem_pool *paged_mem_pool_ptr = pool_ptr;

    /* Memory pool usage */
    /*
    printf("%u of %u = %.2f\n", \
            (unsigned int)paged_mem_pool_ptr->size_used, \
            (unsigned int)paged_mem_pool_ptr->size_total, \
            paged_mem_pool_ptr->size_used / (float)paged_mem_pool_ptr->size_total);
    fflush(stdout);
    */

    return (paged_mem_pool_ptr->size_used > (paged_mem_pool_ptr->size_total >> 1)) ? 1 : 0;
}

