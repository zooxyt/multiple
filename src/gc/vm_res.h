/* Virtual Machine : Resource Management
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

#ifndef _VM_RES_H_
#define _VM_RES_H_

#include <stdint.h>
#include <stdio.h>

#include "vm_startup.h"
#include "vm_infrastructure.h"


struct virtual_machine_resource_source
{
    /* Memory Pool */
    void *pool;
    void *(*init)(size_t size);
    int (*uninit)(void *pool_ptr);

    /* Methods for memory allocation & deallocation */
    void *(*malloc)(void *pool_ptr, size_t size);
    int (*free)(void *pool_ptr, void *ptr);

    /* Lack */
    int (*lack)(void *pool_ptr);
    /* Feedback */
    int (*feedback)(void *pool_ptr);
};
struct virtual_machine_resource_source *virtual_machine_resource_source_new(\
        void *(*init)(size_t size), \
        int (*uninit)(void *pool_ptr), \
        void *(*pool_malloc)(void *pool_ptr, size_t size), \
        int (*pool_free)(void *pool_ptr, void *ptr), \
        int (*lack)(void *pool_ptr), \
        int (*feedback)(void *pool_ptr), \
        size_t size);
int virtual_machine_resource_source_destroy(struct virtual_machine_resource_source *source);


/* Write Barriers */

/* TODO: Optimize this part by replacing the data structure */
struct virtual_machine_write_barrier
{
    struct virtual_machine_object_table_item *eden;
    struct virtual_machine_object_table_item *survivor;

    struct virtual_machine_write_barrier *next;
};
struct virtual_machine_write_barrier *virtual_machine_write_barrier_new(void);
int virtual_machine_write_barrier_destroy(struct virtual_machine_write_barrier *write_barrier);

struct virtual_machine_write_barrier_list
{
    struct virtual_machine_write_barrier *begin;
    struct virtual_machine_write_barrier *end;
};
struct virtual_machine_write_barrier_list *virtual_machine_write_barrier_list_new(void);
int virtual_machine_write_barrier_list_destroy(struct virtual_machine_write_barrier_list *write_barrier_list);
int virtual_machine_write_barrier_list_append(struct virtual_machine_write_barrier_list *write_barrier_list, \
        struct virtual_machine_write_barrier *new_write_barrier);

int virtual_machine_write_barrier_list_update(struct virtual_machine_write_barrier_list *write_barrier_list, \
    struct virtual_machine_object_table_item *eden, \
    struct virtual_machine_object_table_item *survivor);
int virtual_machine_write_barrier_list_eden_is_ref_by_survivor(struct virtual_machine_write_barrier_list *write_barrier_list, \
    struct virtual_machine_object_table_item *eden);
int virtual_machine_write_barrier_list_remove_item(struct virtual_machine_write_barrier_list *write_barrier_list, \
    struct virtual_machine_object_table_item *item);
       

enum 
{
    VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE = 0,
    VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE = 1,
    VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE = 2,
};
#define VIRTUAL_MACHINE_RESOURCE_SOURCE_COUNT 3

/* Main Data Structure of Resource Management */
struct virtual_machine_resource
{
    /* Objects in VM */
    struct virtual_machine_object_table *objects;

    /* Sources */
    struct virtual_machine_resource_source *sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_COUNT];
};

struct virtual_machine_resource *virtual_machine_resource_new(struct virtual_machine_startup *startup);
int virtual_machine_resource_destroy(struct virtual_machine_resource *resource);

/* Memory allocation for virtual machine infrastructure */
void *virtual_machine_resource_malloc(struct virtual_machine_resource *resource, size_t size);
int virtual_machine_resource_free(struct virtual_machine_resource *resource, void *ptr);

/* Memory allocation for primitive data type */
void *virtual_machine_resource_malloc_primitive(struct virtual_machine_resource *resource, size_t size);
int virtual_machine_resource_free_primitive(struct virtual_machine_resource *resource, void *ptr);

/* Memory allocation for reference objects */
void *virtual_machine_resource_malloc_reference(struct virtual_machine_resource *resource, size_t size);
int virtual_machine_resource_free_reference(struct virtual_machine_resource *resource, void *ptr);

/* GC Interface */
int virtual_machine_resource_lack(struct virtual_machine_resource *resource);
int virtual_machine_resource_feedback(struct virtual_machine_resource *resource);

#endif

