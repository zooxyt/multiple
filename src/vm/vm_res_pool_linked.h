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

#ifndef _VM_RES_POOL_LINKED_H_
#define _VM_RES_POOL_LINKED_H_

#include "vm_res.h"

/* Initialize and release pool */
void *virtual_machine_resource_pool_linked_init(size_t size);
void *virtual_machine_resource_pool_linked_64b_init(size_t size);
void *virtual_machine_resource_pool_linked_128b_init(size_t size);
int virtual_machine_resource_pool_linked_uninit(void *pool_ptr);

/* Allocate and deallocate memory */
void *virtual_machine_resource_pool_linked_malloc(void *pool_ptr, size_t size);
int virtual_machine_resource_pool_linked_free(void *pool_ptr, void *ptr);

/* GC Interface */
int virtual_machine_resource_pool_linked_lack(void *pool_ptr);
int virtual_machine_resource_pool_linked_feedback(void *pool_ptr);

#endif


