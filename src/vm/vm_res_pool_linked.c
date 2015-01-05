/* Virtual Machine : Resource Management : Linked Pool
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

#include "linked_pool.h"
#include "vm_res_pool_linked.h"

/* Initialize and release pool */
void *virtual_machine_resource_pool_linked_init(size_t size)
{
    return linked_mem_pool_new(size, 1);
}

void *virtual_machine_resource_pool_linked_64b_init(size_t size)
{
    return linked_mem_pool_new_64b(size, 1);
}

void *virtual_machine_resource_pool_linked_128b_init(size_t size)
{
    return linked_mem_pool_new_128b(size, 1);
}

int virtual_machine_resource_pool_linked_uninit(void *pool_ptr)
{
    return linked_mem_pool_destroy(pool_ptr);
}

/* Allocate and deallocate memory */
void *virtual_machine_resource_pool_linked_malloc(void *pool_ptr, size_t size)
{
    return linked_mem_pool_malloc(pool_ptr, size);
}

int virtual_machine_resource_pool_linked_free(void *pool_ptr, void *ptr)
{
    return linked_mem_pool_free(pool_ptr, ptr);
}

/* Lack */
int virtual_machine_resource_pool_linked_lack(void *pool_ptr)
{
    struct linked_mem_pool *linked_mem_pool_ptr = pool_ptr;

    /*if ((linked_mem_pool_ptr->size_used > \*/
    /*((linked_mem_pool_ptr->size_total >> 2) + (linked_mem_pool_ptr->size_total >> 1))))*/
    /*{*/
    /**//* Memory pool usage */
    /*printf("%u of %u + %u = %.2f\n", \*/
    /*(unsigned int)linked_mem_pool_ptr->size_used, \*/
    /*(unsigned int)linked_mem_pool_ptr->size_total, \*/
    /*(unsigned int)linked_mem_pool_ptr->size_used_extra, \*/
    /*(float)linked_mem_pool_ptr->size_used / (float)linked_mem_pool_ptr->size_total);*/
    /*fflush(stdout);*/
    /*}*/

    /*return 1;*/

    return linked_mem_pool_lack(linked_mem_pool_ptr);
}

/* Feedback */
int virtual_machine_resource_pool_linked_feedback(void *pool_ptr)
{
    struct linked_mem_pool *linked_mem_pool_ptr = pool_ptr;
    return linked_mem_pool_feedback(linked_mem_pool_ptr);
}

