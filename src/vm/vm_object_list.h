/* List Objects
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

#ifndef _VM_OBJECT_LIST_H_
#define _VM_OBJECT_LIST_H_

#include <stdio.h>
#include <stdint.h>

#include "vm_infrastructure.h"
#include "vm_err.h"

struct virtual_machine_object;

/* Internal Part */

#define LIST_INTERNAL_INDEX_CREATE_THRESHOLD 10

struct virtual_machine_object_list_internal_index
{
    struct virtual_machine_object_list_internal_node **arr;
    size_t size;
};

struct virtual_machine_object_list_internal_node
{
    struct virtual_machine_object *ptr;
    struct virtual_machine_object_list_internal_node *next;
    struct virtual_machine_object_list_internal_node *prev;
};

struct virtual_machine_object_list_internal
{
    struct virtual_machine_object_list_internal_node *begin;
    struct virtual_machine_object_list_internal_node *end;
    size_t size;

    struct virtual_machine_object_list_internal_index *index;
    size_t ref_by_idx_count;

    struct virtual_machine *vm;
};

/* Internal Marker */
int virtual_machine_object_list_internal_marker(void *object_internal);
/* Internal Collector */
int virtual_machine_object_list_internal_collector(void *object_internal, int *confirm);


/* Shell Part */

struct virtual_machine_object_list
{
    /* Pointer to internal (the kernel part) */
    struct virtual_machine_object_list_internal *ptr_internal;
};

/* Basic */
struct virtual_machine_object *virtual_machine_object_list_new( \
        struct virtual_machine *vm);
int virtual_machine_object_list_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_list_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_list_print(const struct virtual_machine_object *object);

/* Size */
int virtual_machine_object_list_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src);
/* Make */
#define VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_DEFAULT 0
#define VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_REVERSE 1
int virtual_machine_object_list_make( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_top, const size_t count, int order, \
        struct virtual_machine_running_stack_frame *target_frame);

/* Index */
int virtual_machine_object_list_ref_get( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, const struct virtual_machine_object *object_idx);
int virtual_machine_object_list_ref_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, const struct virtual_machine_object *object_idx, \
        const struct virtual_machine_object *object_value);
int virtual_machine_object_list_car( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);
int virtual_machine_object_list_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);

int virtual_machine_object_list_cdr_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_member);

/* Append */
int virtual_machine_object_list_append( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, struct virtual_machine_object *object_new_sub);
int virtual_machine_object_list_append_to_head( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, struct virtual_machine_object *object_new_sub);

/* Unpack */
#define VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_DEFAULT 0
#define VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_REVERSE 1
int virtual_machine_object_list_unpack( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, \
        int order);

/* eq, ne */
int virtual_machine_object_list_binary_equality( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, const uint32_t opcode);

#endif


