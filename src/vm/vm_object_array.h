/* Array Objects
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

#ifndef _VM_OBJECT_ARRAY_H_
#define _VM_OBJECT_ARRAY_H_

#include <stdio.h>
#include <stdint.h>

#include "vm_infrastructure.h"
#include "vm_err.h"

struct virtual_machine_object;

/* Internal Part */

struct virtual_machine_object_array_internal_node
{
    struct virtual_machine_object *ptr;
};

struct virtual_machine_object_array_internal
{
    struct virtual_machine_object_array_internal_node *nodes;
    size_t pos;
    size_t size;
    size_t capacity;

    struct virtual_machine *vm;
};

/* Internal Marker */
int virtual_machine_object_array_internal_marker(void *object_internal);
/* Internal Collector */
int virtual_machine_object_array_internal_collector(void *object_internal, int *confirm);


/* Shell Part */
struct virtual_machine_object_array
{
    /* Pointer to internal (the kernel part) */
    struct virtual_machine_object_array_internal *ptr_internal;
};

/* Basic */
struct virtual_machine_object *virtual_machine_object_array_new( \
        struct virtual_machine *vm);
int virtual_machine_object_array_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_array_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_array_print(const struct virtual_machine_object *object);

/* Size */
int virtual_machine_object_array_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src);
/* Make */
#define VIRTUAL_MACHINE_OBJECT_ARRAY_MAKE_ORDER_DEFAULT 0
#define VIRTUAL_MACHINE_OBJECT_ARRAY_MAKE_ORDER_REVERSE 1
int virtual_machine_object_array_make( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_top, const size_t count, int order, \
        struct virtual_machine_running_stack_frame *target_frame);

/* Index */
int virtual_machine_object_array_ref_get( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, const struct virtual_machine_object *object_idx);
int virtual_machine_object_array_ref_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, const struct virtual_machine_object *object_idx, \
        const struct virtual_machine_object *object_value);
int virtual_machine_object_array_car( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);
int virtual_machine_object_array_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);

/* Append */
int virtual_machine_object_array_append( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, struct virtual_machine_object *object_new_sub);

/* eq, ne */
int virtual_machine_object_array_binary_equality( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, const uint32_t opcode);

#endif

